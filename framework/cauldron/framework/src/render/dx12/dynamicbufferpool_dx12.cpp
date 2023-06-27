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

#include "render/dx12/gpuresource_dx12.h"
#include "render/dx12/dynamicbufferpool_dx12.h"

#include "misc/assert.h"
#include "dxheaders/include/directx/d3dx12.h"

namespace cauldron
{
    DynamicBufferPool* DynamicBufferPool::CreateDynamicBufferPool()
    {
        return new DynamicBufferPoolInternal();
    }

    DynamicBufferPoolInternal::DynamicBufferPoolInternal() : DynamicBufferPool()
    {
        // Init the d3d12 resource backing the dynamic buffer pool
        GPUResourceInitParams initParams = {};
        initParams.heapType = D3D12_HEAP_TYPE_UPLOAD;
        initParams.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_TotalSize);
        initParams.type = GPUResourceType::Buffer;

        m_pResource = GPUResource::CreateGPUResource(L"Cauldron dynamic buffer pool", nullptr, ResourceState::GenericRead, &initParams);

        // Map the memory
        CauldronThrowOnFail(m_pResource->GetImpl()->DX12Resource()->Map(0, nullptr, (void**)&m_pData));
    }
    
    DynamicBufferPoolInternal::~DynamicBufferPoolInternal()
    {
        m_pResource->GetImpl()->DX12Resource()->Unmap(0, nullptr);
    }

    BufferAddressInfo DynamicBufferPoolInternal::AllocConstantBuffer(uint32_t size, const void* pInitData)
    {
        uint32_t alignedSize = AlignUp(size, 256u);

        uint32_t offset;
        CauldronAssert(ASSERT_CRITICAL, m_RingBuffer.Alloc(alignedSize, &offset), L"DynamicBufferPool has run out of memory. Please increase the allocation size.");
        
        // Copy the data in
        void* pBuffer = (void*)(m_pData + offset);
        memcpy(pBuffer, pInitData, size);

        BufferAddressInfo bufferInfo = {};
        BufferAddressInfoInternal* pInfo = (BufferAddressInfoInternal*)(&bufferInfo);
        pInfo->GPUBufferView = m_pResource->GetImpl()->DX12Resource()->GetGPUVirtualAddress() + offset;

        return bufferInfo;
    }

    BufferAddressInfo DynamicBufferPoolInternal::AllocVertexBuffer(uint32_t vertexCount, uint32_t vertexStride, void** pBuffer)
    {
        uint32_t size = AlignUp(vertexCount * vertexStride, 256u);

        uint32_t offset;
        CauldronAssert(ASSERT_CRITICAL, m_RingBuffer.Alloc(size, &offset), L"DynamicBufferPool has run out of memory. Please increase the allocation size.");

        // Set the buffer data pointer
        *pBuffer = (void*)(m_pData + offset);

        // Fill in the buffer address info struct
        BufferAddressInfo bufferInfo = {};
        BufferAddressInfoInternal* pInfo = (BufferAddressInfoInternal*)(&bufferInfo);
        pInfo->GPUBufferView    = m_pResource->GetImpl()->DX12Resource()->GetGPUVirtualAddress() + offset;
        pInfo->SizeInBytes      = size;
        pInfo->StrideInBytes    = vertexStride;

        return bufferInfo;
    }

    BufferAddressInfo DynamicBufferPoolInternal::AllocIndexBuffer(uint32_t indexCount, uint32_t indexStride, void** pBuffer)
    {
        CauldronAssert(ASSERT_CRITICAL, indexStride == 2 || indexStride == 4, L"Requesting allocation of index buffer with an invalid index size.");
        uint32_t size = AlignUp(indexCount * indexStride, 256u);

        uint32_t offset;
        CauldronAssert(ASSERT_CRITICAL, m_RingBuffer.Alloc(size, &offset), L"DynamicBufferPool has run out of memory. Please increase the allocation size.");

        // Set the buffer data pointer
        *pBuffer = (void*)(m_pData + offset);

        // Fill in the buffer address info struct
        BufferAddressInfo bufferInfo = {};
        BufferAddressInfoInternal* pInfo = (BufferAddressInfoInternal*)(&bufferInfo);
        pInfo->GPUBufferView    = m_pResource->GetImpl()->DX12Resource()->GetGPUVirtualAddress() + offset;
        pInfo->SizeInBytes      = size;
        pInfo->Format           = (indexStride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

        return bufferInfo;
    }

} // namespace cauldron

#endif // #if defined(_DX12)
