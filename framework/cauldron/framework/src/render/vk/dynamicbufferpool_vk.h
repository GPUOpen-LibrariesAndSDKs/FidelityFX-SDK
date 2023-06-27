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

#if defined(_VK)

#include "render/dynamicbufferpool.h"
#include "render/vk/buffer_vk.h"
#include <vulkan/vulkan.h>

namespace cauldron
{
    class DynamicBufferPoolInternal final : public DynamicBufferPool
    {
    public:
        DynamicBufferPoolInternal();
        virtual ~DynamicBufferPoolInternal();

        virtual BufferAddressInfo AllocConstantBuffer(uint32_t size, const void* pInitData) override;
        virtual BufferAddressInfo AllocVertexBuffer(uint32_t vertexCount, uint32_t vertexStride, void** pBuffer) override;
        virtual BufferAddressInfo AllocIndexBuffer(uint32_t indexCount, uint32_t indexStride, void** pBuffer) override;
    };

} // namespace cauldron

#endif // #if defined(_VK)
