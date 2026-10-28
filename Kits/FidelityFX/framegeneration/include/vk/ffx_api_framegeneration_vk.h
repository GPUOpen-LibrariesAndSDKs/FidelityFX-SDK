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
#include "../../../api/include/vk/ffx_api_vk.h"

#define FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION_MAJOR 4
#define FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION_MINOR 0
#define FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION_PATCH 0

#define FFX_FRAMEGENERATION_SWAPCHAIN_VK_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION FFX_FRAMEGENERATION_SWAPCHAIN_VK_MAKE_VERSION(FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION_MAJOR, FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION_MINOR, FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION_PATCH)

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FGSWAPCHAIN_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x01)
struct ffxCreateContextDescFrameGenerationSwapChainVK
{
    ffxCreateContextDescHeader header;
    VkPhysicalDevice           physicalDevice;     ///< the physicak device used by the program.
    VkDevice                   device;             ///< the logical device used by the program.
    VkSwapchainKHR*            swapchain;          ///< the current swapchain to be replaced. Will be destroyed when the context is created. This can be VK_NULL_HANDLE. Will contain the new swapchain on return.
    VkAllocationCallbacks*     allocator;          ///< optional allocation callbacks.
    VkSwapchainCreateInfoKHR   createInfo;         ///< the description of the desired swapchain. If its VkSwapchainCreateInfoKHR::oldSwapchain field isn't VK_NULL_HANDLE, it should be the same as the ffxCreateContextDescFrameGenerationSwapChainVK::swapchain field above.
    VkQueueInfoFFXAPI          gameQueue;          ///< the main graphics queue, where Present is called.
    VkQueueInfoFFXAPI          asyncComputeQueue;  ///< A queue with Compute capability.
    VkQueueInfoFFXAPI          presentQueue;       ///< A queue with Transfer and Present capabilities.
    VkQueueInfoFFXAPI          imageAcquireQueue;  ///< A queue with no capability required.
};

#define FFX_API_CONFIGURE_DESC_TYPE_FGSWAPCHAIN_REGISTERUIRESOURCE_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x02)
struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceVK
{
    ffxConfigureDescHeader header;
    struct FfxApiResource  uiResource;  ///< Resource containing user interface for composition. May be empty.
    uint32_t               flags;       ///< Zero or combination of values from FfxApiUiCompositionFlags.
};

#define FFX_API_QUERY_DESC_TYPE_FGSWAPCHAIN_INTERPOLATIONCOMMANDLIST_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x03)
struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListVK
{
    ffxQueryDescHeader header;
    void** pOutCommandList;  ///< Output command nuffer (VkCommandBuffer) to be used for frame generation dispatch.
};

#define FFX_API_QUERY_DESC_TYPE_FGSWAPCHAIN_INTERPOLATIONTEXTURE_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x04)
struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureVK
{
    ffxQueryDescHeader     header;
    struct FfxApiResource* pOutTexture;  ///< Output resource in which the frame interpolation result should be placed.
};

#define FFX_API_DISPATCH_DESC_TYPE_FGSWAPCHAIN_WAIT_FOR_PRESENTS_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x07)
struct ffxDispatchDescFrameGenerationSwapChainWaitForPresentsVK
{
    ffxDispatchDescHeader header;
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x08)
struct ffxConfigureDescFrameGenerationSwapChainKeyValueVK
{
    ffxConfigureDescHeader header;
    uint64_t               key;        ///< Configuration key, member of the FfxApiConfigureFrameGenerationSwapChainKeyVK enumeration.
    uint64_t               u64;        ///< Integer value or enum value to set.
    void* ptr;        ///< Pointer to set or pointer to value to set.
};

//enum value matches enum FfxFrameInterpolationSwapchainConfigureKey
enum FfxApiConfigureFrameGenerationSwapChainKeyVK
{
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK = 0,                     ///< Sets FfxWaitCallbackFunc
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING = 2,                ///< Sets FfxApiSwapchainFramePacingTuning casted from ptr
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x09)
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVK
{
    ffxQueryDescHeader header;
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;  ///< Output pointer. Requires an existing swapchain context.
};

/// Pre-context GPU memory estimate for a Vulkan frame-generation swapchain.
/// Does not require an ffxContext. Pass the device and the swapchain configuration the application intends to create.
#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_VK_V2 FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x0b)
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2
{
    ffxQueryDescHeader              header;                                  ///< Description header for the pre-context GPU memory query.
    VkDevice                        device;                                  ///< Logical device used to size images. Required.
    VkPhysicalDevice                physicalDevice;                          ///< Physical device that owns the logical device. Required. Used to select an external provider.
    struct FfxApiDimensions2D       displaySize;                             ///< Resolution of rendered and generated frames. Required.
    uint32_t                        backBufferFormat;                        ///< Backbuffer format. One of the values from FfxApiSurfaceFormat. Required.
    uint32_t                        backBufferCount;                         ///< Number of application backbuffers, from 1 through 6. Required.
    struct FfxApiDimensions2D       uiResourceSize;                          ///< UI resource resolution. Required when flags requests internal UI double buffering, otherwise ignored.
    uint32_t                        uiResourceFormat;                        ///< UI resource format. One of the values from FfxApiSurfaceFormat. Required when flags requests internal UI double buffering.
    uint32_t                        flags;                                   ///< UI composition flags. Zero or a combination of values from FfxApiUiCompositionFlags.
    uint32_t                        composeOnGameQueue;                      ///< Nonzero when UI composition runs on the game queue. Adds the composition output image.
    uint32_t                        reserved;                                ///< Must be zero.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;  ///< Output. totalUsageInBytes is the sum of the swapchain images. aliasableUsageInBytes is zero because those images stay live together.
};

/// Function to get the number of presents. This is useful when using frame interpolation
typedef uint64_t(*PFN_getLastPresentCountFFXAPI)(VkSwapchainKHR);

/// FFX API specific functions to create and destroy a swapchain
typedef VkResult(*PFN_vkCreateSwapchainFFXAPI)(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain, void* pContext);
typedef void(*PFN_vkDestroySwapchainFFXAPI)(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator, void* pContext);

#define FFX_API_QUERY_DESC_TYPE_FGSWAPCHAIN_FUNCTIONS_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x05)
struct ffxQueryDescSwapchainReplacementFunctionsVK
{
    ffxQueryDescHeader header;
    PFN_vkCreateSwapchainFFXAPI   pOutCreateSwapchainFFXAPI;      ///< Replacement of vkCreateSwapchainKHR. Recreates the swapchain in place (for example on resize or vsync toggle); pContext and the returned handle are the existing frame generation swapchain.
    PFN_vkDestroySwapchainFFXAPI  pOutDestroySwapchainFFXAPI;     ///< Replacement of vkDestroySwapchainKHR. Must be called after the swapchain context is destroyed; pContext is the frame generation swapchain. The handle is invalid afterwards.
    PFN_vkGetSwapchainImagesKHR   pOutGetSwapchainImagesKHR;      ///< Replacement of vkGetSwapchainImagesKHR.
    PFN_vkAcquireNextImageKHR     pOutAcquireNextImageKHR;        ///< Replacement of vkAcquireNextImageKHR.
    PFN_vkQueuePresentKHR         pOutQueuePresentKHR;            ///< Replacement of vkQueuePresentKHR.
    PFN_vkSetHdrMetadataEXT       pOutSetHdrMetadataEXT;          ///< Replacement of vkSetHdrMetadataEXT.
    PFN_getLastPresentCountFFXAPI pOutGetLastPresentCountFFXAPI;  ///< Additional function to get the number of times present has been called since the swapchain creation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FGSWAPCHAIN_MODE_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x10)
struct ffxCreateContextDescFrameGenerationSwapChainModeVK
{
    ffxCreateContextDescHeader header;
    bool                       composeOnPresentQueue;  ///< flags indicating that composition will happen on the present queue
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_VK FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK, 0x0a)
struct ffxCreateContextDescFrameGenerationSwapChainVersionVK
{
    ffxCreateContextDescHeader header;   ///< Description header for frame generation swapchain version context creation.
    uint32_t                   version;  ///< The API version the application was built against. This must be set to FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION.
};
