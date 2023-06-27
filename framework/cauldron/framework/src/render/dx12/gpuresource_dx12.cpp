// AMD Cauldron code
//
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#if defined(_DX12)
#include "render/dx12/device_dx12.h"
#include "render/dx12/gpuresource_dx12.h"

#include "core/framework.h"
#include "misc/assert.h"

namespace cauldron
{
    GPUResource* GPUResource::CreateGPUResource(const wchar_t* resourceName, void* pOwner, ResourceState initialState, void* pInitParams, bool resizable /* = false */)
    {
        GPUResourceInitParams* pParams = (GPUResourceInitParams*)pInitParams;
        switch (pParams->type)
        {
        case GPUResourceType::Texture:
        case GPUResourceType::Buffer:
            return new GPUResourceInternal(pParams->resourceDesc, pParams->heapType, initialState, resourceName, pOwner, resizable);
        case GPUResourceType::Swapchain:
            return new GPUResourceInternal(pParams->pResource, resourceName, initialState, resizable);
        default:
            CauldronCritical(L"Unsupported GPUResourceType creation requested");
            break;
        }

        return nullptr;
    }

    GPUResourceInternal::GPUResourceInternal(ID3D12Resource* pResource, const wchar_t* resourceName, ResourceState initialState, bool resizable) :
        GPUResource(resourceName, nullptr, initialState, resizable),
        m_pResource(pResource),
        m_ResourceDesc(pResource->GetDesc())
    {
        // Set the name on the existing resource (externally allocated)
        m_pResource->SetName(resourceName);

        // Setup sub-resource states
        InitSubResourceCount(m_ResourceDesc.DepthOrArraySize * m_ResourceDesc.MipLevels);
    }

    GPUResourceInternal::GPUResourceInternal(D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType, ResourceState initialState, const wchar_t* resourceName,  void* pOwner, bool resizable) :
        GPUResource(resourceName, pOwner, initialState, resizable),
        m_ResourceDesc(resourceDesc)
    {
        // Allocate using D3D12MA
        CreateResourceInternal(heapType, initialState);

        // What type of resource is this?
        if (m_pOwner)
        {
            if (heapType == D3D12_HEAP_TYPE_UPLOAD)
                m_OwnerType = OwnerType::Memory;
            else
            {
                if (m_ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
                    m_OwnerType = OwnerType::Buffer;
                else if (m_ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN) {
                    
                    m_OwnerType = OwnerType::Texture;
                    
                    // Update the texture desc after creation (as some parameters can auto-generate info (i.e. mip levels)
                    m_ResourceDesc.MipLevels = DX12Desc().MipLevels;
                }

                    
            }
        }
        // Setup sub-resource states
        InitSubResourceCount(m_ResourceDesc.DepthOrArraySize * m_ResourceDesc.MipLevels);
    }

    GPUResourceInternal::~GPUResourceInternal()
    {
        // Only release the resource if we have an allocation (swapchain resources are backed by swapchain)
        if (m_pAllocation)
        {
            m_pAllocation->Release();
        }
        m_pResource->Release();
    }

    void GPUResourceInternal::SetOwner(void* pOwner)
    {
        m_pOwner = pOwner;

        // What type of resource is this?
        if (m_pOwner)
        {
            if (m_pAllocation && m_pAllocation->GetHeap()->GetDesc().Properties.Type == D3D12_HEAP_TYPE_UPLOAD)
                m_OwnerType = OwnerType::Memory;

            else
            {
                if (m_ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
                    m_OwnerType = OwnerType::Buffer;

                else if (m_ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN)
                    m_OwnerType = OwnerType::Texture;
            }
        }
    }

    void GPUResourceInternal::RecreateResource(D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType, ResourceState initialState)
    {
        CauldronAssert(ASSERT_ERROR, m_Resizable, L"Cannot recreate a resource that isn't resizable");
        if(m_pResource)
        {
            m_pResource->Release();
            m_pResource = nullptr;
        }
        if(m_pAllocation)
        {
            m_pAllocation->Release();
            m_pAllocation = nullptr;
        }
        m_ResourceDesc = resourceDesc;
        // Setup sub-resource states
        InitSubResourceCount(m_ResourceDesc.DepthOrArraySize * m_ResourceDesc.MipLevels);
        CreateResourceInternal(heapType, initialState);
    }

    void GPUResourceInternal::CreateResourceInternal(D3D12_HEAP_TYPE heapType, ResourceState initialState)
    {
        CauldronAssert(ASSERT_ERROR, m_pAllocation == nullptr && m_pResource == nullptr, L"GPU resource was not freed before recreation.");

        // Allocate resource via D3D12 memory allocator
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = heapType;

        static bool s_InvertedDepth = GetConfig()->InvertedDepth;
        D3D12_CLEAR_VALUE* pClearValue = NULL;
        D3D12_CLEAR_VALUE clearValue;
        if (m_ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        {
            clearValue.Format = m_ResourceDesc.Format;
            clearValue.DepthStencil.Depth = s_InvertedDepth ? 0.f : 1.0f;
            clearValue.DepthStencil.Stencil = 0;
            pClearValue = &clearValue;
        }
        else if (m_ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
        {
            clearValue.Format = m_ResourceDesc.Format;
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 0.0f;
            pClearValue = &clearValue;
        }

        CauldronThrowOnFail(GetDevice()->GetImpl()->GetD3D12MemoryAllocator()->CreateResource(&allocationDesc, &m_ResourceDesc, GetDXResourceState(initialState),
            pClearValue, &m_pAllocation, IID_PPV_ARGS(&m_pResource)));

        // And set a resource name
        m_pAllocation->SetName(GetName());
        m_pResource->SetName(GetName());
    }

    DXGI_FORMAT GetDXGIFormat(ResourceFormat format)
    {
        switch (format)
        {
        case ResourceFormat::Unknown:
            return DXGI_FORMAT_UNKNOWN;

            // 8-bit
        case ResourceFormat::R8_SINT:
            return DXGI_FORMAT_R8_SINT;
        case ResourceFormat::R8_UINT:
            return DXGI_FORMAT_R8_UINT;
        case ResourceFormat::R8_UNORM:
            return DXGI_FORMAT_R8_UNORM;

            // 16-bit
        case ResourceFormat::R16_SINT:
            return DXGI_FORMAT_R16_SINT;
        case ResourceFormat::R16_UINT:
            return DXGI_FORMAT_R16_UINT;
        case ResourceFormat::R16_FLOAT:
            return DXGI_FORMAT_R16_FLOAT;
        case ResourceFormat::R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case ResourceFormat::R16_SNORM:
            return DXGI_FORMAT_R16_SNORM;
        case ResourceFormat::RG8_SINT:
            return DXGI_FORMAT_R8G8_SINT;
        case ResourceFormat::RG8_UINT:
            return DXGI_FORMAT_R8G8_UINT;
        case ResourceFormat::RG8_UNORM:
            return DXGI_FORMAT_R8G8_UNORM;

            // 32-bit
        case ResourceFormat::R32_SINT:
            return DXGI_FORMAT_R32_SINT;
        case ResourceFormat::R32_UINT:
            return DXGI_FORMAT_R32_UINT;
        case ResourceFormat::RGBA8_SINT:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case ResourceFormat::RGBA8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case ResourceFormat::RGBA8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case ResourceFormat::RGBA8_SNORM:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case ResourceFormat::RGBA8_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case ResourceFormat::RGBA8_TYPELESS:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case ResourceFormat::RGB10A2_UNORM:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case ResourceFormat::RG11B10_FLOAT:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case ResourceFormat::RG16_SINT:
            return DXGI_FORMAT_R16G16_SINT;
        case ResourceFormat::RG16_UINT:
            return DXGI_FORMAT_R16G16_UINT;
        case ResourceFormat::RG16_FLOAT:
            return DXGI_FORMAT_R16G16_FLOAT;
        case ResourceFormat::R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;

            // 64-bit
        case ResourceFormat::RGBA16_SINT:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case ResourceFormat::RGBA16_UINT:
        case ResourceFormat::RGBA16_UNORM:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case ResourceFormat::RGBA16_SNORM:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case ResourceFormat::RGBA16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case ResourceFormat::RG32_SINT:
            return DXGI_FORMAT_R32G32_SINT;
        case ResourceFormat::RG32_UINT:
            return DXGI_FORMAT_R32G32_UINT;
        case ResourceFormat::RG32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;

            // 96-bit
        case ResourceFormat::RGB32_SINT:
            return DXGI_FORMAT_R32G32B32_SINT;
        case ResourceFormat::RGB32_UINT:
            return DXGI_FORMAT_R32G32B32_UINT;
        case ResourceFormat::RGB32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;


            // 128-bit
        case ResourceFormat::RGBA32_SINT:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case ResourceFormat::RGBA32_UINT:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case ResourceFormat::RGBA32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case ResourceFormat::RGBA32_TYPELESS:
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;

            // Depth
        case ResourceFormat::D16_UNORM:
            return DXGI_FORMAT_D16_UNORM;
        case ResourceFormat::D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;

            // Compressed
        case ResourceFormat::BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;
        case ResourceFormat::BC1_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case ResourceFormat::BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM;
        case ResourceFormat::BC2_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case ResourceFormat::BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;
        case ResourceFormat::BC3_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case ResourceFormat::BC4_UNORM:
            return DXGI_FORMAT_BC4_UNORM;
        case ResourceFormat::BC4_SNORM:
            return DXGI_FORMAT_BC4_SNORM;
        case ResourceFormat::BC5_UNORM:
            return DXGI_FORMAT_BC5_UNORM;
        case ResourceFormat::BC5_SNORM:
            return DXGI_FORMAT_BC5_SNORM;
        case ResourceFormat::BC6_UNSIGNED:
            return DXGI_FORMAT_BC6H_UF16;
        case ResourceFormat::BC6_SIGNED:
            return DXGI_FORMAT_BC6H_SF16;
        case ResourceFormat::BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM;
        case ResourceFormat::BC7_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:
            CauldronCritical(L"Unsupported Format conversion requested.");
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    DXGI_FORMAT DXGIToGamma(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
        }

        return format;
    }

    DXGI_FORMAT DXGIFromGamma(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM;
        }

        return format;
    }

    uint32_t GetDXGIFormatStride(ResourceFormat format)
    {
        switch (format)
        {
            // 8-bit
        case ResourceFormat::R8_UNORM:
            return 1;

            // 16-bit
        case ResourceFormat::R16_FLOAT:
        case ResourceFormat::R16_SINT:
        case ResourceFormat::R16_UINT:
        case ResourceFormat::R16_UNORM:
        case ResourceFormat::R16_SNORM:
        case ResourceFormat::D16_UNORM:
            return 2;

            // 32-bit
        case ResourceFormat::RGBA8_UNORM:
        case ResourceFormat::RGBA8_SNORM:
        case ResourceFormat::RGBA8_SRGB:
        case ResourceFormat::RGBA8_TYPELESS:
        case ResourceFormat::RGB10A2_UNORM:
        case ResourceFormat::RG11B10_FLOAT:
        case ResourceFormat::RG16_FLOAT:
        case ResourceFormat::R32_UINT:
        case ResourceFormat::R32_FLOAT:
        case ResourceFormat::D32_FLOAT:
            return 4;

            // 64-bit
        case ResourceFormat::RGBA16_UNORM:
        case ResourceFormat::RGBA16_SNORM:
        case ResourceFormat::RGBA16_FLOAT:
        case ResourceFormat::RG32_FLOAT:
            // Compressed - 64 bits per block
        case ResourceFormat::BC1_UNORM:
        case ResourceFormat::BC1_SRGB:
        case ResourceFormat::BC4_UNORM:
        case ResourceFormat::BC4_SNORM:
            return 8;

            // 128-bit
        case ResourceFormat::RGBA32_SINT:
        case ResourceFormat::RGBA32_UINT:
        case ResourceFormat::RGBA32_FLOAT:
        case ResourceFormat::RGBA32_TYPELESS:
            // Compressed - 128 bits per block
        case ResourceFormat::BC2_UNORM:
        case ResourceFormat::BC2_SRGB:
        case ResourceFormat::BC3_UNORM:
        case ResourceFormat::BC3_SRGB:
        case ResourceFormat::BC5_UNORM:
        case ResourceFormat::BC5_SNORM:
        case ResourceFormat::BC7_UNORM:
        case ResourceFormat::BC7_SRGB:
            return 16;

        default:
            CauldronAssert(ASSERT_ERROR, 0, L"Requesting format stride of unsupported format. Please add it");
            return 0;
        }
    }

    D3D12_RESOURCE_STATES GetDXResourceState(ResourceState state)
    {
        switch (state)
        {
        case ResourceState::CommonResource:
            return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::VertexBufferResource:
        case ResourceState::ConstantBufferResource:
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case ResourceState::IndexBufferResource:
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case ResourceState::RenderTargetResource:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceState::UnorderedAccess:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceState::DepthWrite:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ResourceState::DepthRead:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ResourceState::DepthShaderResource:
            return D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        case ResourceState::NonPixelShaderResource:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ResourceState::PixelShaderResource:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceState::ShaderResource:
            return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        case ResourceState::IndirectArgument:
            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case ResourceState::CopyDest:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceState::CopySource:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::ResolveDest:
            return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case ResourceState::ResolveSource:
            return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        case ResourceState::RTAccelerationStruct:
            return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        case ResourceState::ShadingRateSource:
            return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        case ResourceState::GenericRead:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case ResourceState::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        default:
            CauldronError(L"Unsupported Resource State conversion requested, returning D3D12_RESOURCE_STATE_COMMON.");
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    D3D12_RESOURCE_FLAGS GetDXResourceFlags(ResourceFlags flags)
    {
        D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

        if (static_cast<bool>(flags & ResourceFlags::AllowRenderTarget))
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (static_cast<bool>(flags & ResourceFlags::AllowDepthStencil))
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (static_cast<bool>(flags & ResourceFlags::AllowUnorderedAccess))
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (static_cast<bool>(flags & ResourceFlags::DenyShaderResource))
            resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        if (static_cast<bool>(flags & ResourceFlags::AllowSimultaneousAccess))
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

        return resourceFlags;
    }

} // namespace cauldron

#endif // #if defined(_DX12)
