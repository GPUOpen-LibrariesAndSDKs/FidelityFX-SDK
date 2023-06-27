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

#include "misc/helpers.h"
#include "render/renderdefines.h"

namespace cauldron
{
    class GPUResource;

    /**
     * @class CopyResource
     *
     * Copy resources are used to prime GPU resources with data (copied over via the Copy queue).
     *
     * @ingroup CauldronRender
     */
    class CopyResource
    {
    public:

        /**
         * @brief   CopyResource instance creation function. Implemented per api/platform to return the correct
         *          internal resource type.
         */
        static CopyResource* CreateCopyResource(const GPUResource* pDest, void* pInitData, uint64_t dataSize, ResourceState initialState);

        /**
         * @brief   Destruction.
         */
        virtual ~CopyResource();

        /**
         * @brief   Gets the backing <c><i>GPUResource</i></c>.
         */
        GPUResource* GetResource() { return m_pResource; }
        const GPUResource* GetResource() const { return m_pResource; }

    private:
        // No copy, No move
        NO_COPY(CopyResource)
        NO_MOVE(CopyResource)

    protected:
        CopyResource(void* pInitData, uint64_t dataSize) {}
        CopyResource() = delete;

        GPUResource* m_pResource = nullptr;
    };

} // namespace cauldron
