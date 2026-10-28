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

#include "../../include/dx12/ffx_api_framegeneration_dx12.hpp"
#include "../../../backend/dx12/ffx_dx12.h"
#include "../../include/ffx_framegeneration.hpp"
#include "../include/ffx_provider_fsr3framegenerationswapchain.h"
#include "../../../framegeneration/include/ffx_framegeneration.h"

#include <stdlib.h>

#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR (3)
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR (1)
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH (7)

bool ffxProvider_Fsr3FrameGenerationSwapChain::CanProvide(uint64_t type) const
{
    return ((type & FFX_API_EFFECT_MASK) == FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN &&
            (type & FFX_API_BACKEND_MASK) == FFX_API_BACKEND_ID_DX12);
}

struct InternalFgScContext
{
    InternalContextHeader header;
    IDXGISwapChain4* fiSwapChain;
    FfxUInt32 version;
};

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X) 
#define MAKE_VERSION_STRING(major, minor, patch) STRINGIFY major "." STRINGIFY minor "." STRINGIFY patch

// If you change versioning in Id and VersionName, you must also update the DLL
// version defines in framegeneration/dx12/resource/resource.h
ffxProvider_Fsr3FrameGenerationSwapChain::ffxProvider_Fsr3FrameGenerationSwapChain()
    : ffxProvider(0xF65C'DD12ULL << 32 | (FFX_SDK_MAKE_VERSION(FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH) & 0xFFFF'FFFF),
              FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN,
              MAKE_VERSION_STRING(FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH))
{
}

static ffxReturnCode_t Fsr3FrameGenerationSwapChainParseVersion(ffxCreateContextDescHeader* header, FfxUInt32& versionPtr)
{
    // Assume the 3.1.5 API version unless otherwise specified - this is the last version prior to ABI stability.
    // We can't assume callers will supply this (due to tacit support for DLL swapping)
    uint32_t version = FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(3, 1, 5);
    bool bFoundVersion = false;

    for (auto it = header; it; it = it->pNext)
    {
        if (auto versionDesc = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainVersionDX12>(it))
        {
            version = versionDesc->version;
            bFoundVersion = true;
            break;
        }
    }

    if (version < FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(3, 1, 5) || version > FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION)
    {
        FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_ERROR, L"ffxCreateContextDescFrameGenerationSwapChainVersionDX12 version is invalid.");
        return FFX_API_RETURN_ERROR_PARAMETER;
    }

    if (!bFoundVersion)
    {
        FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_ERROR, L"An instance of ffxCreateContextDescFrameGenerationSwapChainVersionDX12 must be attached to ffxCreateContextDescFrameGenerationSwapChain* and specify a valid version to access new API functions.");
    }

    versionPtr = version;

    return FFX_OK;
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::CreateContext(ffxContext* context, ffxCreateContextDescHeader* header, Allocator& alloc)
{
    if (auto desc = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainWrapDX12>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(Fsr3FrameGenerationSwapChainParseVersion(header, version) == FFX_OK, FFX_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, FFX_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;

        internal_context->version = version;

        FfxSwapchain swapChain = ffxGetSwapchainDX12(*desc->swapchain);
        TRY2(ffxReplaceSwapchainForFrameinterpolationDX12(desc->gameQueue, swapChain));
        internal_context->fiSwapChain = *desc->swapchain = ffxGetDX12SwapchainPtr(swapChain);

        // reference tracked by internal_context
        internal_context->fiSwapChain->AddRef();

        *context = internal_context;
        return FFX_API_RETURN_OK;
    }
    else if (auto descNew = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainNewDX12>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(Fsr3FrameGenerationSwapChainParseVersion(header, version) == FFX_OK, FFX_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, FFX_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;

        internal_context->version = version;

        FfxSwapchain swapChain;
        TRY2(ffxCreateFrameinterpolationSwapchainDX12(descNew->desc, descNew->gameQueue, descNew->dxgiFactory, swapChain));
        internal_context->fiSwapChain = *descNew->swapchain = ffxGetDX12SwapchainPtr(swapChain);

        // reference tracked by internal_context
        internal_context->fiSwapChain->AddRef();

        *context = internal_context;
        return FFX_API_RETURN_OK;
    }
    else if (auto descHwnd = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainForHwndDX12>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(Fsr3FrameGenerationSwapChainParseVersion(header, version) == FFX_OK, FFX_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, FFX_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;

        internal_context->version = version;

        FfxSwapchain swapChain;
        TRY2(ffxCreateFrameinterpolationSwapchainForHwndDX12(descHwnd->hwnd, descHwnd->desc, descHwnd->fullscreenDesc, descHwnd->gameQueue, descHwnd->dxgiFactory, swapChain));
        internal_context->fiSwapChain = *descHwnd->swapchain = ffxGetDX12SwapchainPtr(swapChain);

        // reference tracked by internal_context
        internal_context->fiSwapChain->AddRef();

        *context = internal_context;
        return FFX_API_RETURN_OK;
    }
    else
    {
        return FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE;
    }
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::DestroyContext(ffxContext* context, Allocator& alloc)
{
    VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);

    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);

    internal_context->fiSwapChain->Release();

    alloc.dealloc(internal_context);

    return FFX_API_RETURN_OK;
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::Configure(ffxContext* context, const ffxConfigureDescHeader* header) const
{
    VERIFY(header, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);

    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12>(header))
    {
        TRY2(ffxRegisterFrameinterpolationUiResourceDX12(ffxGetSwapchainDX12(internal_context->fiSwapChain), desc->uiResource, desc->flags));

        return FFX_API_RETURN_OK;
    }
    else if (auto descKey = ffx::DynamicCast<ffxConfigureDescFrameGenerationSwapChainKeyValueDX12>(header))
    {
        TRY2(ffxConfigureFrameInterpolationSwapchainDX12(ffxGetSwapchainDX12(internal_context->fiSwapChain), static_cast<FfxFrameInterpolationSwapchainConfigureKey>(descKey->key), descKey->ptr));

        return FFX_API_RETURN_OK;
    }
    else
    {
        return FFX_API_RETURN_ERROR_PARAMETER;
    }
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::Query(ffxContext* context, ffxQueryDescHeader* header) const
{
    VERIFY(header, FFX_API_RETURN_ERROR_PARAMETER);

    
    if (auto desc = ffx::DynamicCast<ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12>(header))
    {
        VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
        FfxCommandList outCommandList{};
        TRY2(ffxGetFrameinterpolationCommandlistDX12(ffxGetSwapchainDX12(internal_context->fiSwapChain), outCommandList));
        *desc->pOutCommandList = outCommandList;

        return FFX_API_RETURN_OK;
    }
    else if (auto descTexture = ffx::DynamicCast<ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12>(header))
    {
        VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
        *descTexture->pOutTexture = ffxGetFrameinterpolationTextureDX12(ffxGetSwapchainDX12(internal_context->fiSwapChain));
        
        return FFX_API_RETURN_OK;
    }
    else if (auto descMemory = ffx::DynamicCast<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12>(header))
    {
        VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);
        VERIFY(descMemory->gpuMemoryUsageFrameGenerationSwapchain, FFX_API_RETURN_ERROR_PARAMETER);

        memset(descMemory->gpuMemoryUsageFrameGenerationSwapchain, 0, sizeof(FfxApiEffectMemoryUsage));

        InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
        TRY2(ffxFrameInterpolationSwapchainGetGpuMemoryUsageDX12(ffxGetSwapchainDX12(internal_context->fiSwapChain), descMemory->gpuMemoryUsageFrameGenerationSwapchain));
        return FFX_API_RETURN_OK;
    }
    else if (auto descMemoryV2 = ffx::DynamicCast<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2>(header))
    {
        VERIFY(descMemoryV2->gpuMemoryUsageFrameGenerationSwapchain, FFX_API_RETURN_ERROR_PARAMETER);

        memset(descMemoryV2->gpuMemoryUsageFrameGenerationSwapchain, 0, sizeof(FfxApiEffectMemoryUsage));

        TRY2(ffxFrameInterpolationSwapchainGetGpuMemoryUsageDX12V2(
            static_cast<FfxDevice>(descMemoryV2->device),
            &(descMemoryV2->displaySize),
            (FfxApiSurfaceFormat) descMemoryV2->backBufferFormat,
            descMemoryV2->backBufferCount,
            &(descMemoryV2->uiResourceSize),
            (FfxApiSurfaceFormat) descMemoryV2->uiResourceFormat,
            descMemoryV2->flags,
            descMemoryV2->gpuMemoryUsageFrameGenerationSwapchain));
        return FFX_API_RETURN_OK;
    }
    else
    {
        return FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE;
    }
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::Dispatch(ffxContext* context, const ffxDispatchDescHeader* header) const
{
    VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);
    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12>(header))
    {
        ffxWaitForPresents(internal_context->fiSwapChain);
        return FFX_API_RETURN_OK;
    }
    else
    {
        return FFX_API_RETURN_ERROR;
    }
}

ffxProvider_Fsr3FrameGenerationSwapChain& ffxProvider_Fsr3FrameGenerationSwapChain::GetInstance()
{
    static ffxProvider_Fsr3FrameGenerationSwapChain instance;
    return instance;
}
