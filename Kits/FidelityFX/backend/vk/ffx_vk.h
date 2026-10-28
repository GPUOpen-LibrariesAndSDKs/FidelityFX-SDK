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

/// @defgroup VKBackend Vulkan Backend
/// FidelityFX SDK native backend implementation for Vulkan.
/// 
/// @ingroup Backends

#pragma once

#include <vulkan/vulkan.h>
#include "../../api/internal/ffx_interface.h"
#include "../../api/include/vk/ffx_api_vk.h"

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

/// FFX specific callback type when submitting a command buffer to a queue.
typedef VkResult (*PFN_vkQueueSubmitFFX)(uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);

typedef struct VkQueueInfoFFX
{
    VkQueue              queue;
    uint32_t             familyIndex;
    PFN_vkQueueSubmitFFX submitFunc;
} VkQueueInfoFFX;


typedef enum VkCompositonModeFFX
{
    VK_COMPOSITION_MODE_NOT_FORCED_FFX,
    VK_COMPOSITION_MODE_GAME_QUEUE_FFX,
    VK_COMPOSITION_MODE_PRESENT_QUEUE_FFX,
} VkCompositonModeFFX;

/// Structure holding additional information to effectively replace the game swapchain by the frame interpolation one.
/// Some notes on the queues:
///   - please pass the queue, its family (for queue family ownership transfer purposes) and an optional function if you want to control concurrent submissions
///   - game queue: the queue where the replacement of vkQueuePresentKHR is called. This queue should have Graphics and Compute capabilities (Transfer is implied as per Vulkan specification).
///                  It can be shared with the engine. No Submit function is necessary.
///                 The code assumes that the UI texture is owned by that queue family when present is called.
///   - async compute queue: optional queue with Compute capability (Transfer is implied as per Vulkan specification). If used by the engine, prefer not to enable the async compute path of FSR3 Frame interpolation.
///   - present queue: queue with Graphics, Compute or Transfer capability, and Present support. This queue cannot be used by the engine. Otherwise, some deadlock can occur.
///   - image acquire queue: this one doesn't need any capability. Strongly prefer a queue not used by the engine. The main graphics queue can work too but it might delay the signaling of the semaphore/fence when acquiring a new image, negatively impacting the performance.
typedef struct VkFrameInterpolationInfoFFX
{
    VkPhysicalDevice             physicalDevice;
    VkDevice                     device;
    VkQueueInfoFFX               gameQueue;
    VkQueueInfoFFX               asyncComputeQueue;
    VkQueueInfoFFX               presentQueue;
    VkQueueInfoFFX               imageAcquireQueue;
    VkCompositonModeFFX          compositionMode;
    const VkAllocationCallbacks* pAllocator;
} VkFrameInterpolationInfoFFX;

/// Query how much memory is required for the Vulkan backend's scratch buffer.
/// 
/// @param [in] physicalDevice              A pointer to the VkPhysicalDevice device.
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
///
/// @returns
/// The size (in bytes) of the required scratch memory buffer for the VK backend.
/// 
/// @ingroup VKBackend
FFX_API size_t ffxGetScratchMemorySizeVK(VkPhysicalDevice physicalDevice, size_t maxContexts);

/// Create a <c><i>FfxDevice</i></c> from a <c><i>VkDevice</i></c>.
///
/// @param [in] vkDeviceContext             A pointer to a VKDeviceContext that holds all needed information
///
/// @returns
/// An abstract FidelityFX device.
///
/// @ingroup VKBackend
FFX_API FfxDevice ffxGetDeviceVK(VkDeviceContext* vkDeviceContext);

/// Populate an interface with pointers for the VK backend.
///
/// @param [out] backendInterface           A pointer to a <c><i>FfxInterface</i></c> structure to populate with pointers.
/// @param [in] device                      A pointer to the VkDevice device.
/// @param [in] scratchBuffer               A pointer to a buffer of memory which can be used by the DirectX(R)12 backend.
/// @param [in] scratchBufferSize           The size (in bytes) of the buffer pointed to by <c><i>scratchBuffer</i></c>.
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
///
/// @retval
/// FFX_OK                                  The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_INVALID_POINTER          The <c><i>interface</i></c> pointer was <c><i>NULL</i></c>.
///
/// @ingroup VKBackend
FFX_API FfxErrorCode ffxGetInterfaceVK(
    FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize, 
    size_t maxContexts);

/// Create a <c><i>FfxCommandList</i></c> from a <c><i>VkCommandBuffer</i></c>.
///
/// @param [in] cmdBuf                      A pointer to the Vulkan command buffer.
///
/// @returns
/// An abstract FidelityFX command list.
///
/// @ingroup VKBackend
FFX_API FfxCommandList ffxGetCommandListVK(VkCommandBuffer cmdBuf);

/// Create a <c><i>FfxPipeline</i></c> from a <c><i>VkPipeline</i></c>.
///
/// @param [in] pipeline                    A pointer to the Vulkan pipeline.
///
/// @returns
/// An abstract FidelityFX pipeline.
///
/// @ingroup VKBackend
FFX_API FfxPipeline ffxGetPipelineVK(VkPipeline pipeline);

/// Fetch a <c><i>FfxApiResource</i></c> from a <c><i>GPUResource</i></c>.
///
/// @param [in] vkResource                  A pointer to the (agnostic) VK resource.
/// @param [in] ffxResDescription           An <c><i>FfxApiResourceDescription</i></c> for the resource representation.
/// @param [in] ffxResName                  (optional) A name string to identify the resource in debug mode.
/// @param [in] state                       The state the resource is currently in.
///
/// @returns
/// An abstract FidelityFX resources.
///
/// @ingroup VKBackend
FFX_API FfxApiResource ffxGetResourceVK(void*  vkResource,
    FfxApiResourceDescription                  ffxResDescription,
    const wchar_t*                             ffxResName,
    FfxApiResourceState                        state = FFX_API_RESOURCE_STATE_COMPUTE_READ);

/// Fetch a <c><i>FfxApiSurfaceFormat</i></c> from a VkFormat.
///
/// @param [in] format              The VkFormat to convert to <c><i>FfxApiSurfaceFormat</i></c>.
///
/// @returns
/// An <c><i>FfxApiSurfaceFormat</i></c>.
///
/// @ingroup VKBackend
FFX_API FfxApiSurfaceFormat ffxGetSurfaceFormatVK(VkFormat format);

/// Fetch a <c><i>FfxApiResourceDescription</i></c> from an existing VkBuffer.
///
/// @param [in] buffer              The VkBuffer resource to create a <c><i>FfxApiResourceDescription</i></c> for.
/// @param [in] createInfo          The VkBufferCreateInfo of the buffer
/// @param [in] additionalUsages    Optional <c><i>FfxApiResourceUsage</i></c> flags needed for select resource mapping.
///
/// @returns
/// An <c><i>FfxApiResourceDescription</i></c>.
///
/// @ingroup VKBackend
FFX_API FfxApiResourceDescription ffxGetBufferResourceDescriptionVK(const VkBuffer           buffer,
                                                                 const VkBufferCreateInfo createInfo,
    FfxApiResourceUsage         additionalUsages = FFX_API_RESOURCE_USAGE_READ_ONLY);

/// Fetch a <c><i>FfxApiResourceDescription</i></c> from an existing VkImage.
///
/// @param [in] image               The VkImage resource to create a <c><i>FfxApiResourceDescription</i></c> for.
/// @param [in] createInfo          The VkImageCreateInfo of the buffer
/// @param [in] additionalUsages    Optional <c><i>FfxApiResourceUsage</i></c> flags needed for select resource mapping.
///
/// @returns
/// An <c><i>FfxApiResourceDescription</i></c>.
///
/// @ingroup VKBackend
FFX_API FfxApiResourceDescription ffxGetImageResourceDescriptionVK(const VkImage           image,
                                                                const VkImageCreateInfo createInfo,
                                                                FfxApiResourceUsage        additionalUsages = FFX_API_RESOURCE_USAGE_READ_ONLY);

/// Fetch a <c><i>FfxCommandQueue</i></c> from an existing VkQueue.
///
/// @param [in] commandQueue       The VkQueue to create a <c><i>FfxCommandQueue</i></c> from.
///
/// @returns
/// An <c><i>FfxCommandQueue</i></c>.
///
/// @ingroup VKBackend
FFX_API FfxCommandQueue ffxGetCommandQueueVK(VkQueue commandQueue);

/// Fetch a <c><i>FfxSwapchain</i></c> from an existing VkSwapchainKHR.
///
/// @param [in] pSwapchain          The VkSwapchainKHR to create a <c><i>FfxSwapchain</i></c> from.
///
/// @returns
/// An <c><i>FfxSwapchain</i></c>.
///
/// @ingroup VKBackend
FFX_API FfxSwapchain ffxGetSwapchainVK(VkSwapchainKHR swapchain);

/// Fetch a VkSwapchainKHR from an existing <c><i>FfxSwapchain</i></c>.
///
/// @param [in] ffxSwapchain          The <c><i>FfxSwapchain</i></c> to fetch an VkSwapchainKHR from.
///
/// @returns
/// An VkSwapchainKHR object.
///
/// @ingroup VKBackend
FFX_API VkSwapchainKHR ffxGetVKSwapchain(FfxSwapchain ffxSwapchain);

/// Replaces the current swapchain with the provided <c><i>FfxSwapchain</i></c>.
///
/// @param [in]     gameQueue               The <c><i>FfxCommandQueue</i></c> presentation will occur on.
/// @param [in,out] gameSwapChain           The current <c><i>FfxSwapchain</i></c> to replace, optional. If not NULL, the swapchain will be destroyed. On return, it will hold the <c><i>FfxSwapchain</i></c> to use for frame interpolation presentation.
/// @param [in]     swapchainCreateInfo     The <c><i>VkSwapchainCreateInfoKHR</i></c> of the current swapchain. Its oldSwapchain member should be VK_NULL_HANDLE or the same as gameSwapChain.
/// @param [in]     frameInterpolationInfo  The <c><i>VkFrameInterpolationInfoFFX</i></c> containing additional information for swapchain replacement.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          One of the parameters is invalid. If the returned <c><i>gameSwapChain</i></c> is NULL, the old swapchain has been destroyed.
/// @retval
/// FFX_ERROR_BACKEND_API_ERROR         Internal generic error. If the returned <c><i>gameSwapChain</i></c> is NULL, the old swapchain has been destroyed.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxReplaceSwapchainForFrameinterpolationVK(FfxCommandQueue                    gameQueue,
                                                                FfxSwapchain&                      gameSwapChain,
                                                                const VkSwapchainCreateInfoKHR*    swapchainCreateInfo,
                                                                const VkFrameInterpolationInfoFFX* frameInterpolationInfo);

/// Waits for the <c><i>FfxSwapchain</i></c> to complete presentation.
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to wait on.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxWaitForPresents(FfxSwapchain gameSwapChain);

/// Registers a <c><i>FfxApiResource</i></c> to use for UI with the provided <c><i>FfxSwapchain</i></c>.
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to to register the UI resource with.
/// @param [in] uiResource              The <c><i>FfxApiResource</i></c> representing the UI resource.
/// @param [in] flags                   A set of <c><i>FfxUiCompositionFlags</i></c>.
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxRegisterFrameinterpolationUiResourceVK(FfxSwapchain gameSwapChain, FfxApiResource uiResource, uint32_t flags);

/// Fetches a <c><i>FfxCommandList</i></c> from the <c><i>FfxSwapchain</i></c>.
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to get a <c><i>FfxCommandList</i></c> from.
/// @param [out] gameCommandlist        The <c><i>FfxCommandList</i></c> from the provided <c><i>FfxSwapchain</i></c>.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxGetFrameinterpolationCommandlistVK(FfxSwapchain gameSwapChain, FfxCommandList& gameCommandlist);

/// Fetches a <c><i>FfxApiResource</i></c>  representing the backbuffer from the <c><i>FfxSwapchain</i></c>.
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to get a <c><i>FfxApiResource</i></c> backbuffer from.
///
/// @returns
/// An abstract FidelityFX resources for the swapchain backbuffer.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxApiResource ffxGetFrameinterpolationTextureVK(FfxSwapchain gameSwapChain);

/// Sets a <c><i>FfxFrameGenerationConfig</i></c> to the internal FrameInterpolationSwapChain (in the backend).
///
/// @param [in] config                  The <c><i>FfxFrameGenerationConfig</i></c> to set.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxSetFrameGenerationConfigToSwapchainVK(FfxFrameGenerationConfig const* config);

/// Returns an estimate of the maximum number of in-flight frames for an FSR3 frame
/// interpolation swapchain proxy. Returns 0 if the latency could not be determined.
/// Native VkSwapchainKHR handles are not queried; callers retain their default latency.
FFX_API uint32_t ffxGetMaximumFrameLatencyVK(VkDevice device, FfxSwapchain gameSwapChain);

//enum values should match enum FfxApiConfigureFrameGenerationSwapChainKeyVK
typedef enum FfxFrameInterpolationSwapchainConfigureKey
{
    FFX_FI_SWAPCHAIN_CONFIGURE_KEY_WAITCALLBACK = 0,
    FFX_FI_SWAPCHAIN_CONFIGURE_KEY_FRAMEPACINGTUNING = 2,
} FfxFrameInterpolationSwapchainConfigureKey;

/// Configures <c><i>FfxSwapchain</i></c> via KeyValue API post <c><i>FfxSwapchain</i></c> context creation
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to configure via KeyValue API
/// @param [in] key                     The <c><i>FfxFrameInterpolationSwapchainConfigureKey</i></c> is key
/// @param [in] valuePtr                The <c><i><void *></i></c> pointer to value. What this pointer deference to depends on key.
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxConfigureFrameInterpolationSwapchainVK(FfxSwapchain gameSwapChain, FfxFrameInterpolationSwapchainConfigureKey key, void* valuePtr);

/// Query how much GPU memory created by <c><i>FfxSwapchain</i></c>. This excludes GPU memory created by the VkSwapchain (ie. size of backbuffers).
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to configure via KeyValue API
/// @param [in out] vramUsage           The <c><i>FfxApiEffectMemoryUsage</i></c> is the GPU memory created by FrameInterpolationSwapchain
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup VKFrameInterpolation
FFX_API FfxErrorCode ffxFrameInterpolationSwapchainGetGpuMemoryUsageVK(FfxSwapchain gameSwapChain, FfxApiEffectMemoryUsage* vramUsage);

/// Query how much GPU memory a Vulkan frame-generation swapchain will create before the swapchain context exists.
///
/// @param [in] desc                    The <c><i>ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2</i></c> description.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          The description is missing required fields or is out of range.
///
/// @ingroup VKFrameInterpolation
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2;
FFX_API FfxErrorCode ffxFrameInterpolationSwapchainGetGpuMemoryUsageVKV2(const struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2* desc);

typedef VkResult (*PFN_vkCreateSwapchainFFX)(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain, const VkFrameInterpolationInfoFFX* pFrameInterpolationInfo);

/// Function to get he number of presents. This is useful when using frame interpolation
typedef uint64_t (*PFN_getLastPresentCountFFX)(VkSwapchainKHR);

/// Query estimated Gpu memory size of a resource description
///
/// @param [in] device                  The <c><i>FfxDevice</i></c>
/// @param [in] resourceDesc            The <c><i>FfxCreateResourceDescription</i></c>
/// @param [out] sizeInBytes            The <c><i>uint64_t</i></c>
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query <c><i>ID3D12Device</i></c> from <c><i>FfxDevice</i></c> to call 
///
/// @ingroup DX12Backend
FfxErrorCode ffxGetResourceSizeFromDescriptionVK(FfxDevice device, const FfxCreateResourceDescription* createResourceDescription, uint64_t* sizeInBytes, uint64_t* alignment = nullptr);

/// Query the fixed GPU memory overhead of a backend context for a context-less memory query.
///
/// @param [in] device                  The <c><i>FfxDevice</i></c>
/// @param [in] contextCount            The number of effect contexts the backend is sized for
/// @param [out] byteCount              The backend GPU memory usage expressed in bytes
///
/// @retval
/// FFX_OK                              The operation completed successfully.
///
/// @ingroup VKBackend
FfxErrorCode ffxGetBackendContextGpuMemoryUsageVK(FfxDevice device, size_t contextCount, uint64_t* byteCount);

/// Query The ABI version used by the swapchain
///
/// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c>
///
/// @retval
/// FFX_ABI_INVALID The ABI is Invalid or an error occurred
/// @retval
/// FFX_ABI_OLD The ABI predates FSR 3.1.4 and only supports upscaler replacement
/// @retval
/// FFX_ABI_1_1_4 The FSR 3.1.4 release - first one to support frame-gen replacement
/// @retval
/// FFX_ABI_1_1_5 The ABI is for a patch release for specific titles
/// @retval
/// FFX_ABI_2_0_0 The FSR4 release
/// @retval
/// FFX_ABI_VALID The latest stable ABI, currently a synonym for FFX_ABI_2_0_0
///
/// @ingroup DX12Backend
//FFX_API FfxABIVersion ffxGetSwapchainABIVK(FfxSwapchain gameSwapChain);

FFX_API void ffxRegisterConstantBufferAllocatorVK(FfxApiConstantBufferAllocator fpConstantAllocator);
FFX_API void ffxRegisterResourceAllocatorVK(PfnFfxResourceAllocatorFunc fpResourceAllocator);
FFX_API void ffxRegisterResourceDeallocatorVK(PfnFfxResourceDeallocatorFunc fpResourceDeallocator);

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)
