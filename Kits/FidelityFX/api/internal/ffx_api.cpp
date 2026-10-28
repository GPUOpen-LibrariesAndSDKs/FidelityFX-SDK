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

#include "../include/ffx_api.h"
#include "../internal/ffx_api_helper.h"
#include "../internal/ffx_internal_types.h"
#include "../internal/ffx_error.h"
#include "../internal/ffx_provider.h"
#include "../internal/ffx_backends.h"
#include "../internal/ffx_message.h"

static uint64_t GetVersionOverride(const ffxApiHeader* header)
{
    for (auto it = header; it; it = it->pNext)
    {
        if (auto versionDesc = ffx::DynamicCast<ffxOverrideVersion>(it))
        {
            return versionDesc->versionId;
        }
    }
    return 0;
}

FFX_API_ENTRY ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc, const ffxAllocationCallbacks* memCb)
{
    VERIFY(desc != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    *context = nullptr;

    Allocator alloc{memCb};
    std::optional<ffxProviderExternal> extProviderSlot;
    ffxProvider* provider = GetProvider(desc->type, GetVersionOverride(desc), GetDevice(desc), extProviderSlot);
    VERIFY(provider != nullptr, FFX_API_RETURN_NO_PROVIDER);

    if (extProviderSlot && &*extProviderSlot == provider)
    {
        // external provider was selected, need to move to heap allocation.
        provider = alloc.construct<ffxProviderExternal>(std::move(*extProviderSlot));
    }

    auto retCode = provider->CreateContext(context, desc, alloc);
    if (retCode != FFX_API_RETURN_OK && provider->GetRefCount() == 0)
    {
        provider->~ffxProvider();
        alloc.dealloc(provider);
    }
    return retCode;
}

FFX_API_ENTRY ffxReturnCode_t ffxDestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb)
{
    VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context != nullptr, FFX_API_RETURN_OK);

    Allocator alloc{memCb};
    ffxProvider* provider = GetAssociatedProvider(*context);
    auto retCode = provider->DestroyContext(context, alloc);

    if (provider->GetRefCount() == 0)
    {
        provider->~ffxProvider();
        alloc.dealloc(provider);
    }
    return retCode;
}

FFX_API_ENTRY ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* header)
{
    VERIFY(header != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    if (auto desc = ffx::DynamicCast< ffxConfigureDescGlobalDebug >(header))
    {
        ffxSetPrintMessageCallback(reinterpret_cast< ffxMessageCallback >(desc->fpMessage),
                                   desc->debugLevel);
        return FFX_API_RETURN_OK;
    }
    #pragma FFX_PRAGMA_WARNING_PUSH
    #pragma FFX_PRAGMA_WARNING_DISABLE_DEPRECATIONS
    else if (auto desc1 = ffx::DynamicCast< ffxConfigureDescGlobalDebug1 >(header))
    {
        ffxSetPrintMessageCallback(reinterpret_cast< ffxMessageCallback >(desc1->fpMessage),
                                   desc1->debugLevel);
        return FFX_API_RETURN_OK;
    }
    #pragma FFX_PRAGMA_WARNING_POP
    else
    {
        VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

        return GetAssociatedProvider(*context)->Configure(context, header);
    }
}

FFX_API_ENTRY ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* header)
{
    VERIFY(header != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    ffxReturnCode_t retCode;
    std::optional<ffxProviderExternal> extProviderSlot;
    if (context == nullptr)
    {
        if (auto desc = ffx::DynamicCast<ffxQueryDescGetVersions>(header))
        {
            const uint32_t effectIndex = ((desc->createDescType & FFX_API_EFFECT_MASK) >> 16u) - 1u;
            if (effectIndex >= FFX_API_EFFECT_COUNT)
            {
                if (desc->outputCount)
                    *desc->outputCount = 0;
                FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_ERROR, L"ffxQueryDescGetVersions::createDescType contains an invalid effect ID.");
                return FFX_API_RETURN_ERROR_PARAMETER;
            }

            // if output count is zero or no other pointer passed, count providers only
            if (desc->outputCount && (*desc->outputCount == 0 || (!desc->versionIds && !desc->versionNames)))
            {
                *desc->outputCount = GetProviderCount(desc->createDescType, desc->device, extProviderSlot);
            }
            else if (desc->outputCount && *desc->outputCount > 0)
            {
                uint64_t capacity = *desc->outputCount;
                *desc->outputCount = GetProviderVersions(desc->createDescType, desc->device, capacity, desc->versionIds, desc->versionNames, extProviderSlot);
            }
            return FFX_API_RETURN_OK;
        }
        else if (auto provider = GetProvider(header->type, GetVersionOverride(header), GetDevice(header), extProviderSlot))
        {
            retCode = provider->Query(nullptr, header);
        }
        else
        {
            retCode = FFX_API_RETURN_NO_PROVIDER;
            if (GetDevice(header) == nullptr)
            {
                const wchar_t* effectName = L"UNKNOWN_EFFECT";
                uint32_t effectId = static_cast<uint32_t>(header->type & FFX_API_EFFECT_MASK);
                uint32_t subId = static_cast<uint32_t>(header->type & ~FFX_API_EFFECT_MASK & ~FFX_API_BACKEND_MASK);
                switch (effectId)
                {
                case FFX_API_EFFECT_ID_UPSCALE:                    effectName = L"UPSCALE"; break;
                case FFX_API_EFFECT_ID_FRAMEGENERATION:            effectName = L"FRAMEGENERATION"; break;
                case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN:   effectName = L"FRAMEGENERATIONSWAPCHAIN"; break;
                }
                wchar_t msgBuf[512];
                swprintf_s(msgBuf, 512,
                    L"ffxQuery with null context returned FFX_API_RETURN_NO_PROVIDER (4) for descriptor type 0x%08llx (%s sub 0x%02x). "
                    L"No device found in descriptor chain. For null-context queries to reach driver/external providers, "
                    L"chain an ffxCreateBackendDX12Desc (with device set) via header.pNext.",
                    static_cast<unsigned long long>(header->type), effectName, subId);
                FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_ERROR, msgBuf);
            }
        }
    }
    else
    {
        VERIFY(*context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

        auto provider = GetAssociatedProvider(*context);
        if (provider)
        {
            retCode = provider->Query(context, header);
        }
        else
        {
            retCode = FFX_API_RETURN_NO_PROVIDER;
        }
    }

    return ffxQueryFallback(context, header, retCode);
}

FFX_API_ENTRY ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc)
{
    VERIFY(desc != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    return GetAssociatedProvider(*context)->Dispatch(context, desc);
}
