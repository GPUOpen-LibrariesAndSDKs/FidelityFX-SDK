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

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif  // _WIN32

#include <vulkan/vulkan.h>

#include <atomic>
#include <cstdint>

#include "FrameInterpolationSwapchainVK_Helpers.h"

#include "../../../backend/vk/ffx_vk.h"
//#include <FidelityFX/host/ffx_fsr3.h>

#include "../../include/ffx_framegeneration.h"
#include "../../fsr3/include/ffx_frameinterpolation.h"
#include "../../include/ffx_framegeneration_api_types.h"

// NOTES regarding using win32 objects:
//   - On Windows, critical section and events are faster than their std counterparts
//   - using Win32 threads to set the priorities
//   - this needs to be ported to standard C++ or other platform if necessary
#include <Windows.h>

#define FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT            6
#define FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_ACQUIRE_SEMAPHORE_COUNT 8
#define FFX_FRAME_INTERPOLATION_SWAP_CHAIN_INTERPOLATION_OUTPUTS_COUNT 2

typedef struct PacingData
{
    FfxApiPresentCallbackFunc presentCallback        = nullptr;
    void*                     presentCallbackContext = nullptr;
    FfxApiResource            uiSurface;

    VkPresentModeKHR presentMode;
    bool             usePremulAlphaComposite;
    bool             drawDebugPacingLines;

    uint64_t gameSemaphoreValue;
    uint64_t replacementBufferSemaphoreSignal;
    uint64_t numFramesSentForPresentationBase;
    uint32_t numFramesToPresent;
    UINT64   currentFrameID;

    typedef enum FrameType
    {
        Interpolated_1,
        Real,
        Count
    } FrameType;

    struct FrameInfo
    {
        uint32_t       realImageIndex;
        bool           doPresent;
        FfxApiResource resource;
        uint64_t       interpolationCompletedSemaphoreValue;
        uint64_t       presentIndex;
        uint64_t       presentQpcDelta;
    };

    FrameInfo frames[FrameType::Count];

    void invalidate()
    {
        memset(this, 0, sizeof(PacingData));
    }
} PacingData;

typedef struct ReplacementResource
{
    VkImage                image                      = VK_NULL_HANDLE;
    VkDeviceMemory         memory                     = VK_NULL_HANDLE;
    VkDeviceSize           allocationSize             = 0;
    uint64_t               availabilitySemaphoreValue = 0;
    FfxApiResourceDescription description                = {};
} ReplacementResource;

enum class FGSwapchainCompositionMode
{
    eNone,
    eComposeOnPresentQueue,  ///< optimal behavior
    eComposeOnGameQueue      ///< legacy behavior
};

struct FrameinterpolationPresentInfo
{
    VulkanCommandPool<3, 32> commandPool; // at most 3 families: game, asyncCompute, present

    PacingData scheduledInterpolations;
    PacingData scheduledPresents;

    std::atomic<VkResult> lastPresentResult = VK_SUCCESS;

    FfxApiResource currentUiSurface   = {};
    uint32_t    uiCompositionFlags = 0;

    VkDevice         device         = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSwapchainKHR   realSwapchain  = VK_NULL_HANDLE;

    uint32_t               realSwapchainImageCount = 0;
    VkImage                realSwapchainImages[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT];
    VkSemaphore            frameRenderedSemaphores[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT];
    FfxApiResourceDescription realSwapchainImageDescription;

    ReplacementResource compositionOutput;

    VulkanQueue interpolationQueue = {};
    VulkanQueue asyncComputeQueue  = {};
    VulkanQueue gameQueue          = {};
    VulkanQueue presentQueue       = {};

    VkSemaphore gameSemaphore                                                                = VK_NULL_HANDLE;
    VkSemaphore interpolationSemaphore                                                       = VK_NULL_HANDLE;
    VkSemaphore presentSemaphore                                                             = VK_NULL_HANDLE;  // TODO: probably delete this one
    VkSemaphore replacementBufferSemaphore                                                   = VK_NULL_HANDLE;
    VkSemaphore compositionSemaphore                                                         = VK_NULL_HANDLE;

    uint64_t    lastPresentSemaphoreValue = 0;

    VkSemaphore acquireSemaphores[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_ACQUIRE_SEMAPHORE_COUNT] = {};
    uint32_t    nextAcquireSemaphoreIndex                                                         = 0;

    uint64_t realPresentCount = 0;

    // using win32 threads to set the priorities
    HANDLE           presenterThreadHandle         = NULL;
    CRITICAL_SECTION scheduledFrameCriticalSection = {};
    HANDLE           presentEvent                  = NULL;
    HANDLE           interpolationEvent            = NULL;
    HANDLE           pacerEvent                    = NULL;
    CRITICAL_SECTION swapchainCriticalSection;

    FGSwapchainCompositionMode compositionMode = FGSwapchainCompositionMode::eNone;
    volatile bool              resetTimer      = false;
    volatile bool              shutdown        = false;

    VkResult acquireNextRealImage(uint32_t& imageIndex, VkSemaphore& acquireSemaphore);

    // small helpers for queue ownership transfer
    VkImageMemoryBarrier queueFamilyOwnershipTransferGameToPresent(FfxApiResource resource) const;

    volatile double            safetyMarginInSec = 0.0001; //0.1ms
    volatile double            varianceFactor    = 0.1;

    FfxWaitCallbackFunc waitCallback               = nullptr;
};

// This interface class only contain virtual functions, no data members, vtable layout is fixed, so it's ABI stable.
// Implementation lives in cpp
class IFrameInterpolationSwapChainVK
{
public:
    virtual ~IFrameInterpolationSwapChainVK() {}

    virtual VkResult acquireNextImage(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) = 0;

    virtual VkResult init(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkFrameInterpolationInfoFFX* pFrameInterpolationInfo) = 0;

    virtual void destroySwapchain(VkDevice device, const VkAllocationCallbacks* pAllocator) = 0;

    virtual VkResult getSwapchainImages(VkDevice device, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) = 0;

    virtual VkResult queuePresent(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) = 0;

    virtual void setHdrMetadata(VkDevice device, const VkHdrMetadataEXT* pMetadata) = 0;

    virtual uint64_t getLastPresentCount() = 0;

    virtual void            setFrameGenerationConfig(FfxFrameGenerationConfig const* config)                = 0;
    virtual void            setFramePacingTuning(const FfxApiSwapchainFramePacingTuning* framePacingTuning) = 0;
    virtual bool            waitForPresents()                                                               = 0;
    virtual FfxApiResource  interpolationOutput(int index = 0)                                              = 0;
    virtual VkCommandBuffer getInterpolationCommandList()                                                   = 0;
    virtual void            registerUiResource(FfxApiResource uiResource, uint32_t flags)                   = 0;
    virtual void            setWaitCallback(FfxWaitCallbackFunc waitCallbackFunc)                           = 0;
    virtual void            getGpuMemoryUsage(FfxApiEffectMemoryUsage* vramUsage)                           = 0;

    virtual bool spawnPresenterThread() = 0;
    virtual bool killPresenterThread()  = 0;

    virtual VkResult presentInterpolated(const VkPresentInfoKHR* pPresentInfo, uint32_t currentBackBufferIndex, bool needUICopy) = 0;

    virtual void dispatchInterpolationCommands(uint32_t              currentBackBufferIndex,
                                               FfxApiResource*       pInterpolatedFrame,
                                               FfxApiResource*       pRealFrame,
                                               SubmissionSemaphores& semaphoresToWait) = 0;
    virtual void discardOutstandingInterpolationCommandLists()                         = 0;

    virtual bool                 verifyUiDuplicateResource()                                                                                     = 0;
    virtual VkImageMemoryBarrier copyUiResource(VkCommandBuffer commandBuffer, SubmissionSemaphores& gameQueueWait, bool transferToPresentQueue) = 0;

    // NOTE: append-only. New methods MUST be added at the end to keep the vtable layout (ABI) stable.
    // Recreates the internal (real) VkSwapchainKHR in place - e.g. when toggling vsync - without
    // destroying the frame interpolation context or its resources.
    virtual VkResult recreateSwapchainInPlace(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator) = 0;
};

//////////////////////////////////////////////
/// FrameInterpolationSwapChainVK
//////////////////////////////////////////////
class FrameInterpolationSwapChainVK : public IFrameInterpolationSwapChainVK
{
public:
    FrameInterpolationSwapChainVK();
    virtual ~FrameInterpolationSwapChainVK();

public:  // vulkan functions reimplementation
    VkResult acquireNextImage(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);

    VkResult init(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkFrameInterpolationInfoFFX* pFrameInterpolationInfo);

    void destroySwapchain(VkDevice device, const VkAllocationCallbacks* pAllocator);

    VkResult recreateSwapchainInPlace(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator);

    VkResult getSwapchainImages(VkDevice device, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages);

    VkResult queuePresent(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);

    void setHdrMetadata(VkDevice device, const VkHdrMetadataEXT* pMetadata);

    uint64_t getLastPresentCount();

public:
    void            setFrameGenerationConfig(FfxFrameGenerationConfig const* config);
    void            setFramePacingTuning(const FfxApiSwapchainFramePacingTuning* framePacingTuning);
    bool            waitForPresents();
    FfxApiResource  interpolationOutput(int index = 0);
    VkCommandBuffer getInterpolationCommandList();
    void            registerUiResource(FfxApiResource uiResource, uint32_t flags);
    void            setWaitCallback(FfxWaitCallbackFunc waitCallbackFunc);
    void            getGpuMemoryUsage(FfxApiEffectMemoryUsage* vramUsage);
    FfxErrorCode    setKeyValue(uint32_t key, uint64_t value, void* ptr);

    bool spawnPresenterThread();
    bool killPresenterThread();

    VkResult presentInterpolated(const VkPresentInfoKHR* pPresentInfo, uint32_t currentBackBufferIndex, bool needUICopy);

    void     dispatchInterpolationCommands(uint32_t              currentBackBufferIndex,
                                           FfxApiResource*       pInterpolatedFrame,
                                           FfxApiResource*       pRealFrame,
                                           SubmissionSemaphores& semaphoresToWait);
    void     discardOutstandingInterpolationCommandLists();
    VkResult submitCompositionOnGameQueue(const PacingData& entry);

    bool                 verifyUiDuplicateResource();
    VkImageMemoryBarrier copyUiResource(VkCommandBuffer commandBuffer, SubmissionSemaphores& gameQueueWait, bool transferToPresentQueue);

private:
    VkResult presentNonInterpolatedWithUiCompositionOnGameQueue(uint32_t              imageIndex,
                                                                SubmissionSemaphores& gameQueueWait,
                                                                SubmissionSemaphores& gameQueueSignal,
                                                                SubmissionSemaphores& presentQueueWait,
                                                                bool                  needUICopy);
    VkResult presentNonInterpolatedWithUiCompositionOnPresentQueue(uint32_t              imageIndex,
                                                                   SubmissionSemaphores& gameQueueWait,
                                                                   SubmissionSemaphores& gameQueueSignal,
                                                                   SubmissionSemaphores& presentQueueWait,
                                                                   bool                  needUICopy);
    VkResult queuePresentNonInterpolated(VkCommands* pCommands, uint32_t imageIndex, SubmissionSemaphores& semaphoresToWait);

    VkResult createImage(ReplacementResource&                    resource,
                         VkImageCreateInfo&                      info,
                         FfxApiSurfaceFormat                     format,
                         const char*                             name,
                         const VkPhysicalDeviceMemoryProperties& memProperties,
                         const VkAllocationCallbacks*            pAllocator);
    VkResult createImage(ReplacementResource&                    resource,
                         VkImageCreateInfo&                      info,
                         FfxApiSurfaceFormat                     format,
                         const char*                             name,
                         uint32_t                                index,
                         const VkPhysicalDeviceMemoryProperties& memProperties,
                         const VkAllocationCallbacks*            pAllocator);
    void     destroyImage(ReplacementResource& resource, const VkAllocationCallbacks* pAllocator);

private:
    // swapchain settings
    VkPresentModeKHR presentMode       = VK_PRESENT_MODE_IMMEDIATE_KHR;
    VulkanQueue      imageAcquireQueue = {};

    // for framepacing
    FrameinterpolationPresentInfo presentInfo               = {};
    FfxFrameGenerationConfig      nextFrameGenerationConfig = {};

    uint64_t     interpolationSemaphoreValue      = 0;
    uint64_t     gameSemaphoreValue               = 0;
    bool         frameInterpolationResetCondition = false;
    FfxApiRect2D interpolationRect;

    uint32_t            gameBufferCount                                                                      = 0;
    ReplacementResource replacementSwapBuffers[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT]          = {};
    ReplacementResource interpolationOutputs[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_INTERPOLATION_OUTPUTS_COUNT] = {};
    ReplacementResource uiReplacementBuffer                                                                  = {};
    uint32_t            replacementSwapBufferIndex                                                           = 0;
    uint32_t            interpolationBufferIndex                                                             = 0;
    uint64_t            presentCount                                                                         = 0;
    uint64_t            acquiredCount                                                                        = 0;

    VkDeviceSize totalUsageInBytes     = 0;
    VkDeviceSize aliasableUsageInBytes = 0;

    FfxFrameInterpolationDispatchFlags configFlags = {};

    bool tearingSupported             = false;
    bool interpolationEnabled         = false;
    bool presentInterpolatedOnly      = false;
    bool previousFrameWasInterpolated = false;
    bool drawDebugPacingLines         = false;

    UINT64 currentFrameID = 0;

    LARGE_INTEGER lastTimestamp = {};
    LARGE_INTEGER currTimestamp = {};
    double        perfCountFreq = 0.0;

    uint64_t framesSentForPresentation = 0;

    CRITICAL_SECTION criticalSection             = {};
    CRITICAL_SECTION criticalSectionUpdateConfig = {};
    HANDLE           interpolationThreadHandle   = NULL;

    FfxApiPresentCallbackFunc         presentCallback                = nullptr;
    void*                             presentCallbackContext         = nullptr;
    FfxApiFrameGenerationDispatchFunc frameGenerationCallback        = nullptr;
    void*                             frameGenerationCallbackContext = nullptr;

    uint32_t    backBufferTransferFunction                                                               = 0;
    float       minLuminance                                                                             = 0.0f;
    float       maxLuminance                                                                             = 0.0f;
    VkCommands* registeredInterpolationCommandLists[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT] = {};

    // extension functions
    PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXTProc = nullptr;
};
