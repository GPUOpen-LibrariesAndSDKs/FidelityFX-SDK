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

#include "render/profiler.h"

#include <string>
#include <agilitysdk/include/d3d12.h>
#include <wrl.h>

namespace cauldron
{
    class ProfilerInternal final : public Profiler
    {
    protected:
        void EndFrameGPU(CommandList* pCmdList) override;

        void BeginEvent(CommandList* pCmdList, const wchar_t* label) override;
        void EndEvent(CommandList* pCmdList) override;
        bool InsertTimeStamp(CommandList* pCmdList) override;
        uint32_t RetrieveTimeStamps(CommandList* pCmdList, uint64_t* pQueries, size_t maxCount, uint32_t numTimeStamps) override;

    private:
        friend class Profiler;
        ProfilerInternal(bool enableCPUProfiling, bool enableGPUProfiling);
        virtual ~ProfilerInternal() = default;

        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_pQueryHeap = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource>  m_pBuffer    = nullptr;
    };

} // namespace cauldron

#endif // #if defined(_DX12)
