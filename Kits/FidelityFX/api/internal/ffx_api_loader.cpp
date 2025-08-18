// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
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
#include "../include/ffx_api_types.h"
#include "../internal/ffx_error.h"
#include "../../upscalers/include/ffx_upscale.hpp"
#include "../../framegeneration/include/ffx_framegeneration.hpp"

#ifdef FFX_BACKEND_DX12
#include "../include/dx12/ffx_api_dx12.hpp"
#include "../../framegeneration/include/dx12/ffx_api_framegeneration_dx12.hpp"
#elif defined(FFX_BACKEND_VK)
#include "../include/vk/ffx_api_vk.hpp"
#include "../../framegeneration/include/vk/ffx_api_framegeneration_vk.hpp"
#elif FFX_BACKEND_XBOX
#include "../include/xbox/ffx_api_xbox.hpp"
#include "../../framegeneration/include/xbox/ffx_api_framegeneration_xbox.hpp"
#endif // FFX_BACKEND_DX12

#include "ffx_provider.h"
#include "../internal/ffx_backends.h"
#include "../include/ffx_api_loader.h"
#include "../include/ffx_api-helper.h"

typedef struct ClientModuleType
{
    LPCSTR       moduleName;
    HMODULE      dll;
    ffxFunctions functions;

} ClientModuleType;

static ClientModuleType modules[] = {
#ifdef _DEBUG
#ifdef FFX_BACKEND_DX12
    {"amd_fidelityfx_upscaler_dx12d.dll"},
    {"amd_fidelityfx_framegeneration_dx12d.dll"},
#elif defined(FFX_BACKEND_VK)
    {"amd_fidelityfx_upscaler_vkd.dll"},
    {"amd_fidelityfx_framegeneration_vkd.dll"},
#elif FFX_BACKEND_XBOX
    {"amd_fidelityfx_upscaler_xboxd.dll"},
    {"amd_fidelityfx_framegeneration_xboxd.dll"},
#endif
#elif PROFILE
#if FFX_BACKEND_XBOX
    {"amd_fidelityfx_upscaler_xboxp.dll"},
    {"amd_fidelityfx_framegeneration_xboxp.dll"},
#endif // _GAMING_XBOX
#else
#ifdef FFX_BACKEND_DX12
    {"amd_fidelityfx_upscaler_dx12.dll"},
    {"amd_fidelityfx_framegeneration_dx12.dll"},
#elif defined(FFX_BACKEND_VK)
    {"amd_fidelityfx_upscaler_vk.dll"},
    {"amd_fidelityfx_framegeneration_vk.dll"},
#elif FFX_BACKEND_XBOX
    {"amd_fidelityfx_upscaler_xbox.dll"},
    {"amd_fidelityfx_framegeneration_xbox.dll"},
#endif
#endif // _DEBUG
};
static constexpr size_t moduleCount = _countof(modules);

void LoadChildModules(uint64_t type)
{
    int32_t idx = -1;
    switch (type)
    {
    case FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE:
        idx = 0;
        break;

    case FFX_API_EFFECT_ID_FRAMEGENERATION:
#ifdef FFX_BACKEND_DX12
    case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_DX12:
#elif defined(FFX_BACKEND_VK)
    case FFX_API_EFFECT_ID_FGSC_VK:
#elif FFX_BACKEND_XBOX
    case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_XBOX:
#endif
        idx = 1;
        break;

    default: 
        // skip trying to load DLL if unknown effect type
        return;
    }

    // try to load DLL if it's not loaded yet
    if (modules[idx].dll == nullptr)
    {
        modules[idx].dll = GetModuleHandleA(modules[idx].moduleName);
        if (modules[idx].dll == nullptr)
            modules[idx].dll = LoadLibraryA(modules[idx].moduleName);

        if (modules[idx].dll != nullptr)
            ffxLoadFunctions(&modules[idx].functions, modules[idx].dll);
    }
}

#include "ffx_provider.h"

const ffxProvider* GetAssociatedProvider(ffxContext* context)
{
    const InternalContextHeader* hdr      = (const InternalContextHeader*)(*context);
    const ffxProvider*           provider = hdr->provider;
    return provider;
}

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

    LoadChildModules(desc->type & FFX_API_EFFECT_MASK);

    *context = nullptr;

    switch (desc->type & FFX_API_EFFECT_MASK)
    {
    case FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE:
        if (modules[0].dll)
            return modules[0].functions.CreateContext(context, desc, memCb);
        break;
    case FFX_API_EFFECT_ID_FRAMEGENERATION:
#ifdef FFX_BACKEND_DX12
    case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_DX12:
#elif defined(FFX_BACKEND_VK)
    case FFX_API_EFFECT_ID_FGSC_VK:
#elif FFX_BACKEND_XBOX
    case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_XBOX:
#endif // FFX_BACKEND_DX12
        if (modules[1].dll)
            return modules[1].functions.CreateContext(context, desc, memCb);
        break;
    default:
        break;
    }
    return FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE;
}

FFX_API_ENTRY ffxReturnCode_t ffxDestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb)
{
    VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    Allocator alloc{memCb};
    return GetAssociatedProvider(context)->DestroyContext(context, alloc);
}

FFX_API_ENTRY ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc)
{
    VERIFY(desc != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    return GetAssociatedProvider(context)->Configure(context, desc);
}

FFX_API_ENTRY ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* header)
{
    VERIFY(header != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    ffxStructType_t effectType = header->type & FFX_API_EFFECT_MASK;
    if (header->type == FFX_API_QUERY_DESC_TYPE_GET_VERSIONS)
    {
        auto desc   = reinterpret_cast<ffxQueryDescGetVersions*>(header);
        effectType  = desc->createDescType & FFX_API_EFFECT_MASK;
    }

    LoadChildModules(effectType);
    ffxReturnCode_t retCode;

    if (nullptr == context)
    {
        switch (effectType)
        {
            case FFX_API_EFFECT_ID_UPSCALE:
            {
                if (modules[0].dll)
                    retCode = modules[0].functions.Query(nullptr, header);
                else
                    retCode = FFX_API_RETURN_NO_PROVIDER;
                break;
            }
            case FFX_API_EFFECT_ID_FRAMEGENERATION:
#ifdef FFX_BACKEND_DX12
            case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_DX12:
#elif defined(FFX_BACKEND_VK)
            case FFX_API_EFFECT_ID_FGSC_VK:
#elif FFX_BACKEND_XBOX
            case FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_XBOX:
#endif // FFX_BACKEND_DX12
            {
                if (modules[1].dll)
                    retCode = modules[1].functions.Query(nullptr, header);
                else
                    retCode = FFX_API_RETURN_NO_PROVIDER;
                break;
            }
            default:
            {
                retCode = FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE;
                break;
            }
        }
        return ffxQueryFallback(context, header, retCode);
    }
    else
    {
        auto provider = GetAssociatedProvider(context);
        if (provider)
        {
            retCode = provider->Query(context, header);
        }
        else
        {
            retCode = FFX_API_RETURN_NO_PROVIDER;
        }
        return ffxQueryFallback(context, header, retCode);
    }
}

FFX_API_ENTRY ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc)
{
    VERIFY(desc != nullptr, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, FFX_API_RETURN_ERROR_PARAMETER);

    return GetAssociatedProvider(context)->Dispatch(context, desc);
}
