// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

/// @defgroup SDKSample Samples
/// The FidelityFX SDK Samples Reference Documentation
///

/// @defgroup SDKEffects Effect Samples
/// Effect Samples Reference Documentation
///
/// @ingroup SDKSample

#include "core/framework.h"

#ifdef FFX_API_CAULDRON
#include "ffx_cauldron.h"
#include "render/device.h"
#elif FFX_API_DX12
#include "dx12/ffx_dx12.h"
#include "render/dx12/device_dx12.h"
#include "render/dx12/commandlist_dx12.h"
#include "render/dx12/gpuresource_dx12.h"
#elif FFX_API_VK
#include "vk/ffx_vk.h"
#include "render/vk/device_vk.h"
#include "render/vk/commandlist_vk.h"
#include "render/vk/gpuresource_vk.h"
#endif  // FFX_API_CAULDRON

#include "render/texture.h"

// Make sure this is in sync with FfxGetSurfaceFormatCauldron in ffx_cauldron.cpp
inline FfxSurfaceFormat GetFfxSurfaceFormat(cauldron::ResourceFormat format)
{
    switch (format)
    {
    case (cauldron::ResourceFormat::RGBA32_TYPELESS):
        return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
    case (cauldron::ResourceFormat::RGBA32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case (cauldron::ResourceFormat::RGBA16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case (cauldron::ResourceFormat::RG32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32_FLOAT;
    case (cauldron::ResourceFormat::R8_UINT):
        return FFX_SURFACE_FORMAT_R8_UINT;
    case (cauldron::ResourceFormat::R32_UINT):
        return FFX_SURFACE_FORMAT_R32_UINT;
    case (cauldron::ResourceFormat::RGBA8_TYPELESS):
        return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
    case (cauldron::ResourceFormat::RGBA8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (cauldron::ResourceFormat::RGBA8_SRGB):
        return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case (cauldron::ResourceFormat::RG11B10_FLOAT):
        return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
    case (cauldron::ResourceFormat::RG16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case (cauldron::ResourceFormat::RG16_UINT):
        return FFX_SURFACE_FORMAT_R16G16_UINT;
    case (cauldron::ResourceFormat::RG16_SINT):
        return FFX_SURFACE_FORMAT_R16G16_SINT;
    case (cauldron::ResourceFormat::R16_FLOAT):
        return FFX_SURFACE_FORMAT_R16_FLOAT;
    case (cauldron::ResourceFormat::R16_UINT):
        return FFX_SURFACE_FORMAT_R16_UINT;
    case (cauldron::ResourceFormat::R16_UNORM):
        return FFX_SURFACE_FORMAT_R16_UNORM;
    case (cauldron::ResourceFormat::R16_SNORM):
        return FFX_SURFACE_FORMAT_R16_SNORM;
    case (cauldron::ResourceFormat::R8_UNORM):
        return FFX_SURFACE_FORMAT_R8_UNORM;
    case cauldron::ResourceFormat::RG8_UNORM:
        return FFX_SURFACE_FORMAT_R8G8_UNORM;
    case cauldron::ResourceFormat::R32_FLOAT:
    case cauldron::ResourceFormat::D32_FLOAT:
        return FFX_SURFACE_FORMAT_R32_FLOAT;
    case (cauldron::ResourceFormat::Unknown):
        return FFX_SURFACE_FORMAT_UNKNOWN;
    case cauldron::ResourceFormat::RGBA32_UINT:
        return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
    case cauldron::ResourceFormat::RGB10A2_UNORM:
        return FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
    default:
        FFX_ASSERT_MESSAGE(false, "ValidationRemap: Unsupported format requested. Please implement.");
        return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

inline FfxResourceDescription GetFfxResourceDescription(const cauldron::GPUResource* pResource, FfxResourceUsage additionalUsages)
{
    FfxResourceDescription resourceDescription = { };

    // This is valid
    if (!pResource)
        return resourceDescription;

    if (pResource->IsBuffer())
    {
        const cauldron::BufferDesc& bufDesc = pResource->GetBufferResource()->GetDesc();

        resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;
        resourceDescription.usage  = FFX_RESOURCE_USAGE_UAV;
        resourceDescription.width = bufDesc.Size;
        resourceDescription.height = bufDesc.Stride;
        resourceDescription.format = GetFfxSurfaceFormat(cauldron::ResourceFormat::Unknown);

        // Does not apply to buffers
        resourceDescription.depth = 0;
        resourceDescription.mipCount = 0;

        // Set the type
        resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
    }
    else
    {
        const cauldron::TextureDesc& texDesc = pResource->GetTextureResource()->GetDesc();

        // Set flags properly for resource registration
        resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;
        resourceDescription.usage = IsDepth(texDesc.Format) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
        if (static_cast<bool>(texDesc.Flags & cauldron::ResourceFlags::AllowUnorderedAccess))
            resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

        resourceDescription.width = texDesc.Width;
        resourceDescription.height = texDesc.Height;
        resourceDescription.depth = texDesc.DepthOrArraySize;
        resourceDescription.mipCount = texDesc.MipLevels;
        resourceDescription.format = GetFfxSurfaceFormat(texDesc.Format);

        resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | additionalUsages);

        switch (texDesc.Dimension)
        {
        case cauldron::TextureDimension::Texture1D:
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            break;
        case cauldron::TextureDimension::Texture2D:
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            break;
        case cauldron::TextureDimension::CubeMap:
            // 2D array access to cube map resources
            if (FFX_CONTAINS_FLAG(resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW))
                resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            else
                resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE_CUBE;
            break;
        case cauldron::TextureDimension::Texture3D:
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            break;
        default:
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported texture dimension requested. Please implement.");
            break;
        }
    }

    return resourceDescription;
}

inline size_t ffxGetScratchMemorySize(size_t maxContexts)
{
#ifdef FFX_API_CAULDRON
    return ffxGetScratchMemorySizeCauldron(maxContexts);
#elif FFX_API_DX12
    return ffxGetScratchMemorySizeDX12(maxContexts);
#elif FFX_API_VK
    return ffxGetScratchMemorySizeVK(cauldron::GetDevice()->GetImpl()->VKPhysicalDevice(), maxContexts);
#endif  // FFX_API_CAULDRON
}

inline FfxErrorCode ffxGetInterface(FfxInterface* backendInterface, cauldron::Device* device, void* scratchBuffer, size_t scratchBufferSize, size_t maxContexts)
{
#ifdef FFX_API_CAULDRON
    return ffxGetInterfaceCauldron(backendInterface, ffxGetDeviceCauldron(device), scratchBuffer, scratchBufferSize, maxContexts);
#elif FFX_API_DX12
    return ffxGetInterfaceDX12(backendInterface, ffxGetDeviceDX12(device->GetImpl()->DX12Device()), scratchBuffer, scratchBufferSize, (uint32_t)maxContexts);
#elif FFX_API_VK
    VkDeviceContext vkDeviceContext = { device->GetImpl()->VKDevice(), device->GetImpl()->VKPhysicalDevice(), vkGetDeviceProcAddr };
    return ffxGetInterfaceVK(backendInterface, ffxGetDeviceVK(&vkDeviceContext), scratchBuffer, scratchBufferSize, maxContexts);
#endif  // FFX_API_CAULDRON
}

inline FfxCommandList ffxGetCommandList(cauldron::CommandList* cauldronCmdList)
{
#ifdef FFX_API_CAULDRON
    return ffxGetCommandListCauldron(cauldronCmdList);
#elif FFX_API_DX12
    return ffxGetCommandListDX12(cauldronCmdList->GetImpl()->DX12CmdList());
#elif FFX_API_VK
    return ffxGetCommandListVK(cauldronCmdList->GetImpl()->VKCmdBuffer());
#endif  // FFX_API_CAULDRON
}

inline FfxResource ffxGetResource(const cauldron::GPUResource* cauldronResource,
                           wchar_t*                     name                   = nullptr,
                           FfxResourceStates            state                  = FFX_RESOURCE_STATE_COMPUTE_READ,
                           FfxResourceUsage             additionalUsages       = (FfxResourceUsage)0)
{
#ifdef FFX_API_CAULDRON
    return ffxGetResourceCauldron(cauldronResource, GetFfxResourceDescription(cauldronResource, additionalUsages), name, state);
#elif FFX_API_DX12
    return ffxGetResourceDX12(cauldronResource ? const_cast<ID3D12Resource*>(cauldronResource->GetImpl()->DX12Resource()) : 
        nullptr, GetFfxResourceDescription(cauldronResource, additionalUsages), name, state);
#elif FFX_API_VK
    return ffxGetResourceVK(!cauldronResource ? nullptr : 
        (cauldronResource->IsBuffer() ? (void*)cauldronResource->GetImpl()->GetBuffer() : 
            (void*)cauldronResource->GetImpl()->GetImage()), GetFfxResourceDescription(cauldronResource, additionalUsages), name, state);
#endif  // FFX_API_CAULDRON

    cauldron::CauldronCritical(L"Unsupported API or Platform for FFX Validation Remap");
    return FfxResource();   // Error
}

inline FfxErrorCode ffxReplaceSwapchainForFrameinterpolation(FfxCommandQueue gameQueue, FfxSwapchain& gameSwapChain)
{
#ifdef FFX_API_CAULDRON
    FFX_ASSERT_FAIL("FSR3 is not implemented for Cauldron backend, yet. Please use the native DX12 backend to test FSR3.");
#elif FFX_API_DX12
    return ffxReplaceSwapchainForFrameinterpolationDX12(gameQueue, gameSwapChain);
#elif FFX_API_VK
    FFX_ASSERT(false && "Not Implemented!");
#endif  // FFX_API_CAULDRON

    cauldron::CauldronCritical(L"Unsupported API or Platform for FFX Validation Remap");
    return FFX_ERROR_BACKEND_API_ERROR;  // Error
}

inline FfxErrorCode ffxRegisterFrameinterpolationUiResource(FfxSwapchain gameSwapChain, FfxResource uiResource)
{
#ifdef FFX_API_CAULDRON
    FFX_ASSERT(false && "Not Implemented!");
#elif FFX_API_DX12
    return ffxRegisterFrameinterpolationUiResourceDX12(gameSwapChain, uiResource);
#elif FFX_API_VK
    FFX_ASSERT(false && "Not Implemented!");
#endif  // FFX_API_CAULDRON

    cauldron::CauldronCritical(L"Unsupported API or Platform for FFX Validation Remap");
    return FFX_ERROR_BACKEND_API_ERROR;  // Error
}

inline FfxErrorCode ffxGetInterpolationCommandlist(FfxSwapchain gameSwapChain, FfxCommandList& gameCommandlist)
{
#ifdef FFX_API_CAULDRON
    FFX_ASSERT(false && "Not Implemented!");
#elif FFX_API_DX12
    return ffxGetFrameinterpolationCommandlistDX12(gameSwapChain, gameCommandlist);
#elif FFX_API_VK
    FFX_ASSERT(false && "Not Implemented!");
#endif  // FFX_API_CAULDRON

    cauldron::CauldronCritical(L"Unsupported API or Platform for FFX Validation Remap");
    return FFX_ERROR_BACKEND_API_ERROR;  // Error
}
