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


#include "../../../framegeneration/include/vk/ffx_api_framegeneration_vk.hpp"
#include "../../../backend/vk/ffx_vk.h"
#include "../../include/ffx_framegeneration.hpp"
#include "../include/ffx_provider_fsr3framegenerationswapchain.h"
#include "FrameInterpolationSwapchainVK.h"

struct InternalFgScContext
{
    InternalContextHeader            header;
    VkFrameInterpolationInfoFFX      frameInterpolationInfo;
    VkSwapchainKHR                   fiSwapChain;
    FfxUInt32                        version;
};

//////////////////////////////////////////////
/// Vulkan FFX API overridden functions
//////////////////////////////////////////////

///////////////////////////////////////////////////////////////
/// ffxProvider_Fsr3FrameGenerationSwapChain implementation
///////////////////////////////////////////////////////////////

#define STRINGIFY_(X)                            #X
#define STRINGIFY(X)                             STRINGIFY_(X)
#define MAKE_VERSION_STRING(major, minor, patch) STRINGIFY major "." STRINGIFY minor "." STRINGIFY patch

// Provider version stays on the FSR3 swapchain ABI. The public contract is
// FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION, matching the DX12 split.
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR (3)
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR (1)
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH (7)

ffxProvider_Fsr3FrameGenerationSwapChain::ffxProvider_Fsr3FrameGenerationSwapChain()
    : ffxProvider(0xF65D'564Bui64 << 32 | (FFX_SDK_MAKE_VERSION(FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH) & 0xFFFF'FFFF),
                  FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK,
                  MAKE_VERSION_STRING(FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH))
{
}

static ffxReturnCode_t Fsr3FrameGenerationSwapChainParseVersion(ffxCreateContextDescHeader* header, FfxUInt32& versionPtr)
{
    // Assume the 3.1.5 API version unless otherwise specified - this is the last version prior to ABI stability.
    // We can't assume callers will supply this (due to tacit support for DLL swapping)
    uint32_t version = FFX_FRAMEGENERATION_SWAPCHAIN_VK_MAKE_VERSION(3, 1, 5);
    bool bFoundVersion = false;

    for (auto it = header; it; it = it->pNext)
    {
        if (auto versionDesc = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainVersionVK>(it))
        {
            version = versionDesc->version;
            bFoundVersion = true;
            break;
        }
    }

    if (version < FFX_FRAMEGENERATION_SWAPCHAIN_VK_MAKE_VERSION(3, 1, 5) || version > FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION)
    {
        FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_ERROR, L"ffxCreateContextDescFrameGenerationSwapChainVersionVK version is invalid.");
        return FFX_API_RETURN_ERROR_PARAMETER;
    }

    if (!bFoundVersion)
    {
        FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_ERROR, L"An instance of ffxCreateContextDescFrameGenerationSwapChainVersionVK must be attached to ffxCreateContextDescFrameGenerationSwapChainVK and specify a valid version to access new API functions.");
    }

    versionPtr = version;

    return FFX_OK;
}

bool ffxProvider_Fsr3FrameGenerationSwapChain::CanProvide(uint64_t type) const
{
    return (type & FFX_API_EFFECT_MASK) == FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK;
}

inline VkQueueInfoFFX convertQueueInfo(VkQueueInfoFFXAPI queueInfo)
{
    VkQueueInfoFFX info;
    info.queue       = queueInfo.queue;
    info.familyIndex = queueInfo.familyIndex;
    info.submitFunc  = queueInfo.submitFunc;
    return         info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Swapchain replacement functions
///
/// These are free functions (not bound member functions), so they convert to the plain C function
/// pointers in ffxQueryDescSwapchainReplacementFunctionsVK. The VkSwapchainKHR handle (and the
/// create/destroy pContext) IS the IFrameInterpolationSwapChainVK proxy - the cast to the concrete
/// type happens here, inside the SDK, so the caller (engine) only depends on this function-pointer
/// table and never on the internal C++ type. This keeps the ABI boundary stable.
//////////////////////////////////////////////////////////////////////////////////////////////////

static VkResult ffxApiCreateSwapchainVK(VkDevice                        device,
                                        const VkSwapchainCreateInfoKHR* pCreateInfo,
                                        const VkAllocationCallbacks*    pAllocator,
                                        VkSwapchainKHR*                 pSwapchain,
                                        void*                           pContext)
{
    (void)device;

    IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(pContext);
    if (pSwapChainVK == nullptr) 
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult res = pSwapChainVK->recreateSwapchainInPlace(pCreateInfo, pAllocator);

    if (pSwapchain != nullptr) 
    {
        *pSwapchain = reinterpret_cast<VkSwapchainKHR>(pContext);
    }

    return res;
}

static void ffxApiDestroySwapchainVK(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator, void* pContext)
{
    IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(swapchain);
    if (pSwapChainVK == nullptr)
    {
        return;
    }

    (void)pContext;
    pSwapChainVK->destroySwapchain(device, pAllocator);
    delete pSwapChainVK;
}

static VkResult ffxApiGetSwapchainImagesVK(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
    IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(swapchain);
    return pSwapChainVK->getSwapchainImages(device, pSwapchainImageCount, pSwapchainImages);
}

static VkResult ffxApiAcquireNextImageVK(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
    IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(swapchain);
    return pSwapChainVK->acquireNextImage(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

static VkResult ffxApiQueuePresentVK(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    if (pPresentInfo == nullptr || pPresentInfo->swapchainCount == 0) 
    {
        return VK_SUCCESS;
    }

    // Only a single frame interpolation swapchain is supported per present: there is no way to tell
    // which of several swapchains is the frame interpolation one, so we assume the first is.
    IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(pPresentInfo->pSwapchains[0]);
    return pSwapChainVK->queuePresent(queue, pPresentInfo);
}

static void ffxApiSetHdrMetadataVK(VkDevice device, uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata)
{
    for (uint32_t i = 0; i < swapchainCount; ++i)
    {
        IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(pSwapchains[i]);
        pSwapChainVK->setHdrMetadata(device, &pMetadata[i]);
    }
}

static uint64_t ffxApiGetLastPresentCountVK(VkSwapchainKHR swapchain)
{
    IFrameInterpolationSwapChainVK* pSwapChainVK = reinterpret_cast<IFrameInterpolationSwapChainVK*>(swapchain);
    return pSwapChainVK->getLastPresentCount();
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::CreateContext(ffxContext* context, ffxCreateContextDescHeader* header, Allocator& alloc)
{
    if (auto desc = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainVK>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(Fsr3FrameGenerationSwapChainParseVersion(header, version) == FFX_OK, FFX_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, FFX_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;
        internal_context->version = version;

        internal_context->frameInterpolationInfo.physicalDevice    = desc->physicalDevice;
        internal_context->frameInterpolationInfo.device            = desc->device;
        internal_context->frameInterpolationInfo.gameQueue         = convertQueueInfo(desc->gameQueue);
        internal_context->frameInterpolationInfo.asyncComputeQueue = convertQueueInfo(desc->asyncComputeQueue);
        internal_context->frameInterpolationInfo.presentQueue      = convertQueueInfo(desc->presentQueue);
        internal_context->frameInterpolationInfo.imageAcquireQueue = convertQueueInfo(desc->imageAcquireQueue);
        internal_context->frameInterpolationInfo.pAllocator        = desc->allocator;

        // set the default values
        internal_context->frameInterpolationInfo.compositionMode = VK_COMPOSITION_MODE_NOT_FORCED_FFX;

        // get the extensions
        for (auto it = header->pNext; it != nullptr; it = it->pNext)
        {
            if (auto mode = ffx::DynamicCast<ffxCreateContextDescFrameGenerationSwapChainModeVK>(it))
            {
                if (mode->composeOnPresentQueue)
                    internal_context->frameInterpolationInfo.compositionMode = VK_COMPOSITION_MODE_PRESENT_QUEUE_FFX;
                else
                    internal_context->frameInterpolationInfo.compositionMode = VK_COMPOSITION_MODE_GAME_QUEUE_FFX;
            }
        }

        FfxSwapchain swapChain = ffxGetSwapchainVK(*desc->swapchain);
        TRY2(ffxReplaceSwapchainForFrameinterpolationVK(desc->gameQueue.queue, swapChain, &desc->createInfo, &internal_context->frameInterpolationInfo));
        internal_context->fiSwapChain = *desc->swapchain = ffxGetVKSwapchain(swapChain);

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

    // The swapchain outlives the context; the app destroys it through pOutDestroySwapchainFFXAPI.
    alloc.dealloc(internal_context);

    return FFX_API_RETURN_OK;
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::Configure(ffxContext* context, const ffxConfigureDescHeader* header) const
{
    VERIFY(header, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);

    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceVK>(header))
    {
        TRY2(ffxRegisterFrameinterpolationUiResourceVK(ffxGetSwapchainVK(internal_context->fiSwapChain), desc->uiResource, desc->flags));

        return FFX_API_RETURN_OK;
    }
    else if (auto descKey = ffx::DynamicCast<ffxConfigureDescFrameGenerationSwapChainKeyValueVK>(header))
    {
        TRY2(ffxConfigureFrameInterpolationSwapchainVK(ffxGetSwapchainVK(internal_context->fiSwapChain), static_cast<FfxFrameInterpolationSwapchainConfigureKey>(descKey->key), descKey->ptr));

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

    if (auto descMemoryV2 = ffx::DynamicCast<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2>(header))
    {
        const FfxErrorCode error = ffxFrameInterpolationSwapchainGetGpuMemoryUsageVKV2(descMemoryV2);
        if (error == FFX_ERROR_INVALID_ARGUMENT)
            return FFX_API_RETURN_ERROR_PARAMETER;
        if (error != FFX_OK)
            return FFX_API_RETURN_ERROR_RUNTIME_ERROR;
        return FFX_API_RETURN_OK;
    }

    VERIFY(context, FFX_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);

    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<ffxQueryDescFrameGenerationSwapChainInterpolationCommandListVK>(header))
    {
        FfxCommandList outCommandList{};
        TRY2(ffxGetFrameinterpolationCommandlistVK(ffxGetSwapchainVK(internal_context->fiSwapChain), outCommandList));
        *desc->pOutCommandList = outCommandList;

        return FFX_API_RETURN_OK;
    }
    else if (auto descTexture = ffx::DynamicCast<ffxQueryDescFrameGenerationSwapChainInterpolationTextureVK>(header))
    {
        *descTexture->pOutTexture = ffxGetFrameinterpolationTextureVK(ffxGetSwapchainVK(internal_context->fiSwapChain));
        
        return FFX_API_RETURN_OK;
    }
    else if (auto descMemory = ffx::DynamicCast<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVK>(header))
    {
        TRY2(ffxFrameInterpolationSwapchainGetGpuMemoryUsageVK(ffxGetSwapchainVK(internal_context->fiSwapChain), descMemory->gpuMemoryUsageFrameGenerationSwapchain));
        return FFX_API_RETURN_OK;
    }
    else if (auto descFunctions = ffx::DynamicCast<ffxQueryDescSwapchainReplacementFunctionsVK>(header))
    {
        descFunctions->pOutCreateSwapchainFFXAPI     = ffxApiCreateSwapchainVK;
        descFunctions->pOutDestroySwapchainFFXAPI    = ffxApiDestroySwapchainVK;
        descFunctions->pOutGetSwapchainImagesKHR     = ffxApiGetSwapchainImagesVK;
        descFunctions->pOutAcquireNextImageKHR       = ffxApiAcquireNextImageVK;
        descFunctions->pOutQueuePresentKHR           = ffxApiQueuePresentVK;
        descFunctions->pOutSetHdrMetadataEXT         = ffxApiSetHdrMetadataVK;
        descFunctions->pOutGetLastPresentCountFFXAPI = ffxApiGetLastPresentCountVK;
        return FFX_API_RETURN_OK;
    }
    else
    {
        return FFX_API_RETURN_ERROR_PARAMETER;
    }
}

ffxReturnCode_t ffxProvider_Fsr3FrameGenerationSwapChain::Dispatch(ffxContext* context, const ffxDispatchDescHeader* header) const
{
    VERIFY(*context, FFX_API_RETURN_ERROR_PARAMETER);
    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<ffxDispatchDescFrameGenerationSwapChainWaitForPresentsVK>(header))
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
