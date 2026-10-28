// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <cstdlib>
#include <cstring>
#include <utility>

#include "../include/ffx_api.h"
#if defined(FFX_UPSCALER)
#include "../../upscalers/include/ffx_upscale.h"
#endif //#if defined(FFX_UPSCALER)
#if defined(FFX_FRAMEGENERATION)
#include "../../framegeneration/include/ffx_framegeneration.h"
#if defined(FFX_BACKEND_DX12)
#include "../../framegeneration/include/dx12/ffx_api_framegeneration_dx12.h"
#elif defined(FFX_BACKEND_VK)
#include "../../framegeneration/include/vk/ffx_api_framegeneration_vk.h"
#elif defined(FFX_BACKEND_XBOX)
#include "../../framegeneration/include/xbox/ffx_api_framegeneration_xbox.h"
#else
#error "Include the right backend framegeneration file"
#endif // defined(FFX_BACKEND_DX12)
#endif //#if defined(FFX_FRAMEGENERATION)
#if defined(FFX_DENOISER)
#include "../../denoisers/include/ffx_denoiser.h"
#endif //#if defined(FFX_DENOISER)
#if defined(FFX_RADIANCECACHE)
#include "../../radiancecache/include/ffx_radiancecache.h"
#endif //#if defined(FFX_RADIANCECACHE)

#define VERIFY(_cond, _retcode) \
    if (!(_cond)) return _retcode

#define VERIFY_MESSAGE(_cond, _retcode, _errcode, _msg) \
    do { if (!(_cond)) { FFX_PRINT_MESSAGE(_errcode, _msg); return _retcode; } } while(0)

#define TRY(_expr) \
    if (ffxReturnCode_t _rc = (_expr); _rc != FFX_API_RETURN_OK) return _rc

#define TRY2(_expr) \
    if (FFX_OK != (_expr)) return FFX_API_RETURN_ERROR_RUNTIME_ERROR

template<typename T>
static inline T ConvertEnum(uint32_t enumVal)
{
    return static_cast<T>(enumVal);
}

template<typename T>
static inline uint32_t ReverseConvertEnum(T enumVal)
{
    return static_cast<T>(enumVal);
}

struct Allocator
{
    const ffxAllocationCallbacks* cb;

    void* alloc(size_t sz, const bool zeroOutMemory = true)
    {
        if (cb)
        {
            void* ptr = cb->alloc(cb->pUserData, static_cast<uint64_t>(sz));
            if (zeroOutMemory)
                memset(ptr, 0, sz);
            return ptr;
        }
        else
        {
            if (zeroOutMemory)
            {
                return calloc(1, sz);
            }
            else
            {
                return malloc(sz);
            }
        }
    }

    void dealloc(void* ptr)
    {
        if (cb)
            cb->dealloc(cb->pUserData, ptr);
        else
            free(ptr);
    }

    template<typename T, typename... Args>
    T* construct(Args&&... args)
    {
        void* addr = alloc(sizeof(T));
        return ::new(addr) T(std::forward<Args>(args)...);
    }

    template<typename T>
	void destruct(T* ptr)
	{
		if (!ptr)
			return;

		ptr->~T();
		dealloc(ptr);
	}
};

ffxReturnCode_t ffxQueryFallback(ffxContext* context, ffxQueryDescHeader* header, ffxReturnCode_t retCode);
