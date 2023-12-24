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
#pragma once

#if defined(_DX12)

#include "render/gpuresource.h"

#include "memoryallocator/D3D12MemAlloc.h"
#include <agilitysdk/include/d3d12.h>

namespace cauldron
{
    DXGI_FORMAT GetDXGIFormat(ResourceFormat format);
    DXGI_FORMAT DXGIToGamma(DXGI_FORMAT format);
    DXGI_FORMAT DXGIFromGamma(DXGI_FORMAT format);
    uint32_t GetDXGIFormatStride(ResourceFormat format);
    D3D12_RESOURCE_STATES GetDXResourceState(ResourceState state);
    D3D12_RESOURCE_FLAGS GetDXResourceFlags(ResourceFlags flags);

    struct GPUResourceInitParams
    {
        D3D12_RESOURCE_DESC     resourceDesc;
        D3D12_HEAP_TYPE         heapType;
        ID3D12Resource*         pResource;
        GPUResourceType         type = GPUResourceType::Texture;
    };

    class GPUResourceInternal final : public GPUResource
    {
    public:
        virtual ~GPUResourceInternal();

        ID3D12Resource* DX12Resource() { return m_pResource; }
        const ID3D12Resource* DX12Resource() const { return m_pResource; }

        D3D12_RESOURCE_DESC DX12Desc() { return m_pResource->GetDesc(); }
        const D3D12_RESOURCE_DESC DX12Desc() const { return m_pResource->GetDesc(); }

        void RecreateResource(D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType, ResourceState initialState);
        void SetOwner(void* pOwner) override;

        virtual const GPUResourceInternal* GetImpl() const override { return this; }
        virtual GPUResourceInternal* GetImpl() override { return this; }

    private:
        friend class GPUResource;
        friend class FSR3RenderModule;
        GPUResourceInternal() = delete;
        GPUResourceInternal(ID3D12Resource* pResource, const wchar_t* resourceName, ResourceState initialState, bool resizable = false);
        GPUResourceInternal(D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType, ResourceState initialState, const wchar_t* resourceName, void* pOwner, bool resizable = false);

        void CreateResourceInternal(D3D12_HEAP_TYPE heapType, ResourceState initialState);

        D3D12MA::Allocation*   m_pAllocation  = nullptr;
        ID3D12Resource*        m_pResource    = nullptr;
        D3D12_RESOURCE_DESC    m_ResourceDesc = { };
    };

} // namespace cauldron

#endif // #if defined(_DX12)
