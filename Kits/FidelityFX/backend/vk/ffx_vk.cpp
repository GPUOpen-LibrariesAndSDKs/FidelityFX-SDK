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

#include "../../api/internal/ffx_interface.h"
#include "../../api/internal/ffx_util.h"
#include "../../api/internal/ffx_assert.h"
#include "../../api/internal/ffx_blob_decompress.h"
#include "ffx_vk.h"

#include <vector>


#ifdef _WIN32
#include <windows.h>
#else
#include <codecvt>  // this is deprecated so it's just a fallback solution
#endif  // _WIN32

#include <vulkan/vulkan.h>

// prototypes for functions in the interface
FfxErrorCode           GetEffectGpuMemoryUsageVK(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxApiEffectMemoryUsage* outVramUsage);
FfxErrorCode           CreateBackendContextVK(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId);
FfxErrorCode           GetDeviceCapabilitiesVK(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode           DestroyBackendContextVK(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode           CreateResourceVK(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode           DestroyResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId);
FfxErrorCode           CreateHeapVK(FfxInterface* backendInterface, const FfxCreateHeapDescription* createHeapDescription, FfxUInt32 effectContextId, FfxResourceHeap* outHeap);
FfxErrorCode           DestroyHeapVK(FfxInterface* backendInterface, FfxResourceHeap heap, FfxUInt32 effectContextId);
FfxErrorCode           MapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr);
FfxErrorCode           UnmapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           RegisterResourceVK(FfxInterface* backendInterface, const FfxApiResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxApiResource         GetResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           UnregisterResourcesVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxErrorCode           RegisterStaticResourceVK(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId);
FfxApiResourceDescription GetResourceDescriptionVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           StageConstantBufferDataVK(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer);
FfxErrorCode           CreatePipelineVK(FfxInterface* backendInterface, FfxShaderBlob* pShaderBlob, const FfxPipelineDescription* desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);

FfxErrorCode           DestroyPipelineVK(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode           ScheduleGpuJobVK(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode           ExecuteGpuJobsVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxErrorCode           QueryNextGpuJobDescVK(FfxInterface* backendInterface, FfxGpuJobDescription** outJobPtr);

static VkDeviceContext sVkDeviceContext = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

#define MAX_PIPELINE_USAGE_PER_FRAME      (10) // Required to make sure passes that are called more than once per-frame don't have their descriptors overwritten.
#define MAX_DESCRIPTOR_SET_LAYOUTS        (64)
#define FFX_MAX_BINDLESS_DESCRIPTOR_COUNT (65536)

// Constant buffer allocation callback
static FfxApiConstantBufferAllocator s_fpConstantAllocator = nullptr;
static PfnFfxResourceAllocatorFunc   s_fpResourceAllocator = nullptr;
static PfnFfxResourceDeallocatorFunc s_fpResourceDeallocator = nullptr;

// Offset the binding of samplers to avoid collisions
constexpr uint32_t SAMPLER_BINDING_SHIFT = 320;

typedef struct BackendContext_VK {

    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        char                    resourceName[64] = {};
#endif
        union {
            VkImage             imageResource;
            VkBuffer            bufferResource;
        };

        FfxApiResourceDescription  resourceDescription;
        FfxApiResourceState       initialState;
        FfxApiResourceState       currentState;
        int32_t                 srvViewIndex;
        int32_t                 uavViewIndex;
        uint32_t                uavViewCount;

        VkDeviceMemory          deviceMemory;
        VkDeviceSize            allocationSize;
        VkMemoryPropertyFlags   memoryProperties;

        bool                    undefined;
        bool                    dynamic;

    } Resource;

    typedef struct PipelineLayout {

        VkSampler               samplers[FFX_MAX_SAMPLERS];
        VkDescriptorSetLayout   descriptorSetLayout;
        VkDescriptorSet         descriptorSets[FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME];
        uint32_t                descriptorSetIndex;
        VkPipelineLayout        pipelineLayout;
        int32_t                 staticTextureSrvSet;
        int32_t                 staticBufferSrvSet;
        int32_t                 staticTextureUavSet;
        int32_t                 staticBufferUavSet;
    } PipelineLayout;

    typedef struct VKFunctionTable
    {
        PFN_vkGetDeviceProcAddr                 vkGetDeviceProcAddr = 0;
        PFN_vkSetDebugUtilsObjectNameEXT        vkSetDebugUtilsObjectNameEXT = 0;
        PFN_vkCreateDescriptorPool              vkCreateDescriptorPool = 0;
        PFN_vkCreateSampler                     vkCreateSampler = 0;
        PFN_vkCreateDescriptorSetLayout         vkCreateDescriptorSetLayout = 0;
        PFN_vkCreateBuffer                      vkCreateBuffer = 0;
        PFN_vkCreateBufferView                  vkCreateBufferView = 0;
        PFN_vkCreateImage                       vkCreateImage = 0;
        PFN_vkCreateImageView                   vkCreateImageView = 0;
        PFN_vkCreateShaderModule                vkCreateShaderModule = 0;
        PFN_vkCreatePipelineLayout              vkCreatePipelineLayout = 0;
        PFN_vkCreateComputePipelines            vkCreateComputePipelines = 0;
        PFN_vkDestroyPipelineLayout             vkDestroyPipelineLayout = 0;
        PFN_vkDestroyPipeline                   vkDestroyPipeline = 0;
        PFN_vkDestroyImage                      vkDestroyImage = 0;
        PFN_vkDestroyImageView                  vkDestroyImageView = 0;
        PFN_vkDestroyBuffer                     vkDestroyBuffer = 0;
        PFN_vkDestroyBufferView                 vkDestroyBufferView = 0;
        PFN_vkDestroyDescriptorSetLayout        vkDestroyDescriptorSetLayout = 0;
        PFN_vkDestroyDescriptorPool             vkDestroyDescriptorPool = 0;
        PFN_vkDestroySampler                    vkDestroySampler = 0;
        PFN_vkDestroyShaderModule               vkDestroyShaderModule = 0;
        PFN_vkGetBufferMemoryRequirements       vkGetBufferMemoryRequirements = 0;
        PFN_vkGetBufferMemoryRequirements2KHR   vkGetBufferMemoryRequirements2KHR = 0;
        PFN_vkGetImageMemoryRequirements        vkGetImageMemoryRequirements = 0;
        PFN_vkAllocateDescriptorSets            vkAllocateDescriptorSets = 0;
        PFN_vkFreeDescriptorSets                vkFreeDescriptorSets = 0;
        PFN_vkAllocateMemory                    vkAllocateMemory = 0;
        PFN_vkFreeMemory                        vkFreeMemory = 0;
        PFN_vkMapMemory                         vkMapMemory = 0;
        PFN_vkUnmapMemory                       vkUnmapMemory = 0;
        PFN_vkBindBufferMemory                  vkBindBufferMemory = 0;
        PFN_vkBindImageMemory                   vkBindImageMemory = 0;
        PFN_vkUpdateDescriptorSets              vkUpdateDescriptorSets = 0;
        PFN_vkFlushMappedMemoryRanges           vkFlushMappedMemoryRanges = 0;
        PFN_vkCmdPipelineBarrier                vkCmdPipelineBarrier = 0;
        PFN_vkCmdBindPipeline                   vkCmdBindPipeline = 0;
        PFN_vkCmdBindDescriptorSets             vkCmdBindDescriptorSets = 0;
        PFN_vkCmdDispatch                       vkCmdDispatch = 0;
        PFN_vkCmdDispatchIndirect               vkCmdDispatchIndirect = 0;
        PFN_vkCmdCopyBuffer                     vkCmdCopyBuffer = 0;
        PFN_vkCmdCopyImage                      vkCmdCopyImage = 0;
        PFN_vkCmdCopyBufferToImage              vkCmdCopyBufferToImage = 0;
        PFN_vkCmdClearColorImage                vkCmdClearColorImage = 0;
        PFN_vkCmdFillBuffer                     vkCmdFillBuffer = 0;
        PFN_vkCmdWriteBufferMarkerAMD           vkCmdWriteBufferMarkerAMD = 0;
        PFN_vkCmdWriteBufferMarker2AMD          vkCmdWriteBufferMarker2AMD = 0;
        PFN_vkCmdBeginDebugUtilsLabelEXT        vkCmdBeginDebugUtilsLabelEXT = 0;
        PFN_vkCmdEndDebugUtilsLabelEXT          vkCmdEndDebugUtilsLabelEXT = 0;

    } VkFunctionTable;

    uint32_t refCount;
    uint32_t maxEffectContexts;

    VkDevice                device = VK_NULL_HANDLE;
    VkPhysicalDevice        physicalDevice = VK_NULL_HANDLE;
    VkFunctionTable         vkFunctionTable = {};

    FfxGpuJobDescription*   pGpuJobs;
    uint32_t                gpuJobCount = 0;

    typedef struct VkResourceView {
        VkImageView imageView;
    } VkResourceView;
    VkResourceView*         pResourceViews;

    uint8_t*                pStagingRingBuffer;
    uint32_t                stagingRingBufferBase = 0;

    PipelineLayout*         pPipelineLayouts;

    VkDescriptorPool        descriptorPool;
    uint32_t                bindlessBase;

    VkImageMemoryBarrier    imageMemoryBarriers[FFX_MAX_BARRIERS] = {};
    VkBufferMemoryBarrier   bufferMemoryBarriers[FFX_MAX_BARRIERS] = {};
    uint32_t                scheduledImageBarrierCount = 0;
    uint32_t                scheduledBufferBarrierCount = 0;
    VkPipelineStageFlags    srcStageMask = 0;
    VkPipelineStageFlags    dstStageMask = 0;

    typedef struct alignas(32) EffectContext {

        // Effect identifier -- used for various resource callbacks to application
        FfxEffect           effectId;

        // Resource allocation
        uint32_t              nextStaticResource;
        uint32_t              nextDynamicResource;

        // UAV offsets
        uint32_t              nextStaticResourceView;
        uint32_t              nextDynamicResourceView[FFX_MAX_QUEUED_FRAMES];

        // Bindless descriptors
        uint32_t              bindlessTextureSrvHeapStart;
        uint32_t              bindlessTextureSrvHeapSize;
        uint32_t              bindlessBufferSrvHeapSize;
        uint32_t              bindlessTextureUavHeapStart;
        uint32_t              bindlessTextureUavHeapSize;
        uint32_t              bindlessBufferUavHeapSize;
        VkDescriptorPool      bindlessDescriptorPool;
        VkDescriptorSetLayout bindlessTextureSrvDescriptorSetLayout;
        VkDescriptorSetLayout bindlessBufferSrvDescriptorSetLayout;
        VkDescriptorSetLayout bindlessTextureUavDescriptorSetLayout;
        VkDescriptorSetLayout bindlessBufferUavDescriptorSetLayout;
        VkDescriptorSet       bindlessTextureSrvDescriptorSet;
        VkDescriptorSet       bindlessBufferSrvDescriptorSet;
        VkDescriptorSet       bindlessTextureUavDescriptorSet;
        VkDescriptorSet       bindlessBufferUavDescriptorSet;

        // Pipeline layout
        uint32_t              nextPipelineLayout;

        // the frame index for the context
        uint32_t              frameIndex;

        // Usage
        bool                  active;

        // VRAM usage
        FfxApiEffectMemoryUsage vramUsage;

    } EffectContext;

    Resource*               pResources;
    EffectContext*          pEffectContexts;

     // Allocation defaults
    FfxApiConstantBufferAllocation FallbackConstantAllocator(void* data, FfxUInt64 dataSize);
    VkDeviceMemory        uniformBufferMemory = VK_NULL_HANDLE;
    VkMemoryPropertyFlags uniformBufferMemoryProperties;
    VkDeviceSize          uniformBufferAlignment = 0;
    void*                 uniformBufferMem       = nullptr;
    VkBuffer              uniformBuffer          = VK_NULL_HANDLE;
    VkDeviceSize          uniformBufferSize      = 0;
    VkDeviceSize          uniformBufferOffset    = 0;
    std::mutex            uniformBufferMutex;

    uint32_t                numDeviceExtensions = 0;
    VkExtensionProperties*  extensionProperties = nullptr;
} BackendContext_VK;

FFX_API size_t ffxGetScratchMemorySizeVK(VkPhysicalDevice physicalDevice, size_t maxContexts)
{
    uint32_t numExtensions = 0;

    if (physicalDevice)
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, nullptr);

    uint32_t extensionPropArraySize = sizeof(VkExtensionProperties) * numExtensions;
    uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
    uint32_t resourceViewArraySize = FFX_ALIGN_UP(((maxContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2) + FFX_MAX_BINDLESS_DESCRIPTOR_COUNT) * sizeof(BackendContext_VK::VkResourceView), sizeof(uint32_t));
    uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(maxContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
    uint32_t pipelineArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(uint32_t));
    uint32_t resourceArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(uint32_t));
    uint32_t contextArraySize = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_VK::EffectContext), sizeof(uint32_t));
    
    return FFX_ALIGN_UP(sizeof(BackendContext_VK) + extensionPropArraySize + gpuJobDescArraySize + resourceViewArraySize + stagingRingBufferArraySize +
                            pipelineArraySize + resourceArraySize + contextArraySize,
                        sizeof(uint64_t));
}

// Create a FfxDevice from a VkDevice
FfxDevice ffxGetDeviceVK(VkDeviceContext* vkDeviceContext)
{
    sVkDeviceContext = *vkDeviceContext;
    return reinterpret_cast<FfxDevice>(&sVkDeviceContext);
}

FfxErrorCode ffxGetInterfaceVK(
    FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    size_t maxContexts)
{
    FFX_RETURN_ON_ERROR(
        backendInterface,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBuffer,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBufferSize >= ffxGetScratchMemorySizeVK(((VkDeviceContext*)device)->vkPhysicalDevice, maxContexts),
        FFX_ERROR_INSUFFICIENT_MEMORY);

    backendInterface->fpGetEffectGpuMemoryUsage = GetEffectGpuMemoryUsageVK;
    backendInterface->fpCreateBackendContext = CreateBackendContextVK;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesVK;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextVK;
    backendInterface->fpCreateResource = CreateResourceVK;
    backendInterface->fpDestroyResource = DestroyResourceVK;
    backendInterface->fpCreateHeap = CreateHeapVK;
    backendInterface->fpDestroyHeap = DestroyHeapVK;
    backendInterface->fpMapResource = MapResourceVK;
    backendInterface->fpUnmapResource = UnmapResourceVK;
    backendInterface->fpRegisterResource = RegisterResourceVK;
    backendInterface->fpGetResource = GetResourceVK;
    backendInterface->fpUnregisterResources = UnregisterResourcesVK;
    backendInterface->fpRegisterStaticResource = RegisterStaticResourceVK;
    backendInterface->fpGetResourceDescription = GetResourceDescriptionVK;
    backendInterface->fpStageConstantBufferDataFunc = StageConstantBufferDataVK;
    backendInterface->fpCreatePipeline = CreatePipelineVK;
    backendInterface->fpDestroyPipeline = DestroyPipelineVK;

    backendInterface->fpScheduleGpuJob = ScheduleGpuJobVK;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsVK;

    backendInterface->fpQueryNextGpuJobDesc = QueryNextGpuJobDescVK;

#if defined(FFX_FRAMEGENERATION)
    backendInterface->fpSwapChainConfigureFrameGeneration = ffxSetFrameGenerationConfigToSwapchainVK;
    backendInterface->fpGetSwapchainABI = nullptr; // ffxGetSwapchainABIVK;
#else
    backendInterface->fpSwapChainConfigureFrameGeneration = nullptr;
    backendInterface->fpGetSwapchainABI = nullptr;
#endif // defined(FFX_FRAMEGENERATION)

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFX_RETURN_ON_ERROR(!backendContext->refCount, FFX_ERROR_BACKEND_API_ERROR);

    // Map the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    backendContext->maxEffectContexts = (uint32_t)maxContexts;

    return FFX_OK;
}

FfxCommandList ffxGetCommandListVK(VkCommandBuffer cmdBuf)
{
    FFX_ASSERT(NULL != cmdBuf);
    return reinterpret_cast<FfxCommandList>(cmdBuf);
}

FfxPipeline ffxGetPipelineVK(VkPipeline pipeline)
{
    FFX_ASSERT(NULL != pipeline);
    return reinterpret_cast<FfxPipeline>(pipeline);
}

FfxApiResource ffxGetResourceVK(void* vkResource,
                                FfxApiResourceDescription ffxResDescription,
                                const wchar_t* ffxResName,
                                FfxApiResourceState state /*=FFX_API_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxApiResource resource = {};
    resource.resource = vkResource;
    resource.state = state;
    resource.description = ffxResDescription;

    UNREFERENCED_PARAMETER(ffxResName);

    return resource;
}

uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags requestedProperties, VkMemoryPropertyFlags& outProperties)
{
    FFX_ASSERT(NULL != physicalDevice);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t bestCandidate = UINT32_MAX;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & requestedProperties)) {

            // if just device-local memory is requested, make sure this is the invisible heap to prevent over-subscribing the local heap
            if (requestedProperties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
                continue;

            bestCandidate = i;
            outProperties = memProperties.memoryTypes[i].propertyFlags;

            // if host-visible memory is requested, check for host coherency as well and if available, return immediately
            if ((requestedProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                return bestCandidate;
        }
    }

    return bestCandidate;
}

VkBufferUsageFlags ffxGetVKBufferUsageFlagsFromResourceUsage(FfxApiResourceUsage flags)
{
    VkBufferUsageFlags indirectBit = 0;

    if (FFX_CONTAINS_FLAG(flags, FFX_API_RESOURCE_USAGE_INDIRECT))
        indirectBit = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    // FFX buffers can be copy src/dst (init-upload staging, readback, FFX_GPU_JOB_COPY),
    // so always allow transfer usage. Fixes VUID-vkCmdCopyBuffer-srcBuffer-00118.
    const VkBufferUsageFlags transferBits = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (FFX_CONTAINS_FLAG(flags, FFX_API_RESOURCE_USAGE_UAV))
        return indirectBit | transferBits | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else
        // A read-only (non-UAV) buffer is a STORAGE_BUFFER when bound as a StructuredBuffer SRV, 
        // or a UNIFORM_BUFFER when bound as a cbuffer, so provide both. Fixes VUID-VkWriteDescriptorSet-descriptorType-00331.
        return indirectBit | transferBits | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

VkImageType ffxGetVKImageTypeFromResourceType(FfxApiResourceType type)
{
    switch (type) {
    case(FFX_API_RESOURCE_TYPE_TEXTURE1D):
        return VK_IMAGE_TYPE_1D;
    case(FFX_API_RESOURCE_TYPE_TEXTURE2D):
        return VK_IMAGE_TYPE_2D;
    case(FFX_API_RESOURCE_TYPE_TEXTURE_CUBE):
    case(FFX_API_RESOURCE_TYPE_TEXTURE3D):
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

bool ffxIsSurfaceFormatSRGB(FfxApiSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB):
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return true;
    case (FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
    case (FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT):
    case (FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R32G32B32_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R32G32_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R32G32_UINT):
    case (FFX_API_SURFACE_FORMAT_R8_UINT):
    case (FFX_API_SURFACE_FORMAT_R32_UINT):
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM):
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM):
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM):
    case (FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM):
    case (FFX_API_SURFACE_FORMAT_R16G16_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R16G16_UINT):
    case (FFX_API_SURFACE_FORMAT_R16G16_SINT):
    case (FFX_API_SURFACE_FORMAT_R16_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R16_UINT):
    case (FFX_API_SURFACE_FORMAT_R16_UNORM):
    case (FFX_API_SURFACE_FORMAT_R16_SNORM):
    case (FFX_API_SURFACE_FORMAT_R8_UNORM):
    case (FFX_API_SURFACE_FORMAT_R8G8_UNORM):
    case (FFX_API_SURFACE_FORMAT_R8G8_UINT):
    case (FFX_API_SURFACE_FORMAT_R32_FLOAT):
    case (FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
    case (FFX_API_SURFACE_FORMAT_UNKNOWN):
        return false;
    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return false;
    }
}

FfxApiSurfaceFormat ffxGetSurfaceFormatFromGamma(FfxApiSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
    default:
        return fmt;
    }
}

FfxApiSurfaceFormat ffxGetSurfaceFormatToGamma(FfxApiSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB;
    default:
        return fmt;
    }
}

VkFormat ffxGetVkFormatFromSurfaceFormat(FfxApiSurfaceFormat fmt)
{
    switch (fmt)
    {
    case(FFX_API_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;
    case(FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT):
        return VK_FORMAT_R32G32B32A32_UINT;
    case(FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case (FFX_API_SURFACE_FORMAT_R32G32B32_FLOAT):
        return VK_FORMAT_R32G32B32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32_UINT):
        return VK_FORMAT_R32G32_UINT;
    case(FFX_API_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFX_API_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case (FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_SRGB;
    case(FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return VK_FORMAT_B8G8R8A8_SRGB;
    case(FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case (FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM):
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case(FFX_API_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFX_API_SURFACE_FORMAT_R16G16_SINT):
        return VK_FORMAT_R16G16_SINT;
    case(FFX_API_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFX_API_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFX_API_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFX_API_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8_UINT):
        return VK_FORMAT_R8G8_UINT;
    case(FFX_API_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

VkFormat ffxGetVKUAVFormatFromSurfaceFormat(FfxApiSurfaceFormat fmt)
{
    switch (fmt)
    {
    case(FFX_API_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;
    case(FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT):
        return VK_FORMAT_R32G32B32A32_UINT;
    case(FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32B32_FLOAT):
        return VK_FORMAT_R32G32B32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R32G32_UINT):
        return VK_FORMAT_R32G32_UINT;
    case(FFX_API_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFX_API_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM):
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
    case(FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM):
    case (FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case (FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM):
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case(FFX_API_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFX_API_SURFACE_FORMAT_R16G16_SINT):
        return VK_FORMAT_R16G16_SINT;
    case(FFX_API_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFX_API_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFX_API_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFX_API_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFX_API_SURFACE_FORMAT_R8G8_UINT):
        return VK_FORMAT_R8G8_UINT;
    case(FFX_API_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

FfxApiSurfaceFormat ffxGetSurfaceFormatVK(VkFormat fmt)
{
    switch (fmt)
    {
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case VK_FORMAT_R32G32_SFLOAT:
        return FFX_API_SURFACE_FORMAT_R32G32_FLOAT;
    case VK_FORMAT_R32G32_UINT:
        return FFX_API_SURFACE_FORMAT_R32G32_UINT;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return FFX_API_SURFACE_FORMAT_R32_UINT;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SNORM:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;
    case VK_FORMAT_R16G16_SFLOAT:
        return FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
    case VK_FORMAT_R16G16_UINT:
        return FFX_API_SURFACE_FORMAT_R16G16_UINT;
    case VK_FORMAT_R16G16_SINT:
        return FFX_API_SURFACE_FORMAT_R16G16_SINT;
    case VK_FORMAT_R16_SFLOAT:
        return FFX_API_SURFACE_FORMAT_R16_FLOAT;
    case VK_FORMAT_R16_UINT:
        return FFX_API_SURFACE_FORMAT_R16_UINT;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D16_UNORM_S8_UINT:
        return FFX_API_SURFACE_FORMAT_R16_UNORM;
    case VK_FORMAT_R16_SNORM:
        return FFX_API_SURFACE_FORMAT_R16_SNORM;
    case VK_FORMAT_R8_UNORM:
        return FFX_API_SURFACE_FORMAT_R8_UNORM;
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_S8_UINT:
        return FFX_API_SURFACE_FORMAT_R8_UINT;
    case VK_FORMAT_R8G8_UNORM:
        return FFX_API_SURFACE_FORMAT_R8G8_UNORM;
    case VK_FORMAT_R8G8_UINT:
        return FFX_API_SURFACE_FORMAT_R8G8_UINT;
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return FFX_API_SURFACE_FORMAT_R32_FLOAT;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;
    case VK_FORMAT_UNDEFINED:
        return FFX_API_SURFACE_FORMAT_UNKNOWN;

    default:
        // NOTE: we do not support typeless formats here
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return FFX_API_SURFACE_FORMAT_UNKNOWN;
    }
}

bool isDepthFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;

    default:
        return false;
    }
}

bool isStencilFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;

    default:
        return false;
    }
}

VkImageAspectFlags getImageAspect(FfxApiResourceUsage usage)
{
    // No VK_IMAGE_ASPECT_STENCIL_BIT support for now
    if (FFX_CONTAINS_FLAG(usage, FFX_API_RESOURCE_USAGE_DEPTHTARGET))
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkFormat getVkFormatFromSurfaceFormatAndUsage(FfxApiSurfaceFormat fmt, FfxApiResourceUsage usage)
{
    VkFormat format = ffxGetVkFormatFromSurfaceFormat(fmt);
    bool hasDepth = FFX_CONTAINS_FLAG(usage, FFX_API_RESOURCE_USAGE_DEPTHTARGET);
    bool hasStencil = FFX_CONTAINS_FLAG(usage, FFX_API_RESOURCE_USAGE_STENCILTARGET);
    if (hasDepth || hasStencil)
    {
        switch (fmt)
        {
        case FFX_API_SURFACE_FORMAT_R16_UNORM:
        {
            if (hasDepth && hasStencil)
                return VK_FORMAT_D16_UNORM_S8_UINT;
            else if (hasDepth)
                return VK_FORMAT_D16_UNORM;
            // stencil only doesn't exist
            break;
        }
        case FFX_API_SURFACE_FORMAT_R32_FLOAT:
        {
            if (hasDepth && hasStencil)
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            else if (hasDepth)
                return VK_FORMAT_D32_SFLOAT;
            // stencil only doesn't exist
            break;
        }
        case FFX_API_SURFACE_FORMAT_R32_UINT:
        {
            if (hasDepth && hasStencil)
                return VK_FORMAT_D24_UNORM_S8_UINT;
            else if (hasDepth)
                return VK_FORMAT_X8_D24_UNORM_PACK32;
            // stencil only doesn't exist
            break;
        }
        case FFX_API_SURFACE_FORMAT_R8_UINT:
        {
            if (hasStencil)
                return VK_FORMAT_S8_UINT;
            break;
        }
        default:
            break;
        }
    }

    return format;
}


VkImageUsageFlags getVKImageUsageFlagsFromResourceUsage(FfxApiResourceUsage flags)
{
    VkImageUsageFlags ret = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (FFX_CONTAINS_FLAG(flags, FFX_API_RESOURCE_USAGE_RENDERTARGET)) ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (FFX_CONTAINS_FLAG(flags, FFX_API_RESOURCE_USAGE_UAV)) ret |= (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    return ret;
}

FfxApiResourceDescription ffxGetBufferResourceDescriptionVK(const VkBuffer buffer, const VkBufferCreateInfo createInfo, FfxApiResourceUsage additionalUsages /*=FFX_API_RESOURCE_USAGE_READ_ONLY*/)
{
    FfxApiResourceDescription resourceDescription = {};

    // This is valid
    if (buffer == VK_NULL_HANDLE)
        return resourceDescription;

    resourceDescription.flags  = FFX_API_RESOURCE_FLAGS_NONE;
    resourceDescription.usage  = additionalUsages;
    resourceDescription.size   = (uint32_t)createInfo.size;
    resourceDescription.stride = 0;
    resourceDescription.format = FFX_API_SURFACE_FORMAT_UNKNOWN;

    if (FFX_CONTAINS_FLAG(createInfo.usage, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
        resourceDescription.usage = (FfxApiResourceUsage)(resourceDescription.usage | FFX_API_RESOURCE_USAGE_UAV);
    if (FFX_CONTAINS_FLAG(createInfo.usage, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
        resourceDescription.usage = (FfxApiResourceUsage)(resourceDescription.usage | FFX_API_RESOURCE_USAGE_INDIRECT);

    // What should we initialize this to?? No case for this yet
    resourceDescription.depth    = 0;
    resourceDescription.mipCount = 0;

    // Set the type
    resourceDescription.type = FFX_API_RESOURCE_TYPE_BUFFER;
    
    return resourceDescription;
}

FfxApiResourceDescription ffxGetImageResourceDescriptionVK(const VkImage           image,
                                                        const VkImageCreateInfo createInfo,
                                                        FfxApiResourceUsage        additionalUsages /*=FFX_API_RESOURCE_USAGE_READ_ONLY*/)
{
    FfxApiResourceDescription resourceDescription = {};

    // This is valid
    if (image == VK_NULL_HANDLE)
        return resourceDescription;

    // Set flags properly for resource registration
    resourceDescription.flags = FFX_API_RESOURCE_FLAGS_NONE;
    resourceDescription.usage = FFX_API_RESOURCE_USAGE_READ_ONLY;

    // Check for depth and stencil use
    if (isDepthFormat(createInfo.format))
        resourceDescription.usage = (FfxApiResourceUsage)(resourceDescription.usage | FFX_API_RESOURCE_USAGE_DEPTHTARGET);
    if (isStencilFormat(createInfo.format))
        resourceDescription.usage = (FfxApiResourceUsage)(resourceDescription.usage | FFX_API_RESOURCE_USAGE_STENCILTARGET);

    // Unordered access use
    if (FFX_CONTAINS_FLAG(createInfo.usage, VK_IMAGE_USAGE_STORAGE_BIT))
        resourceDescription.usage = (FfxApiResourceUsage)(resourceDescription.usage | FFX_API_RESOURCE_USAGE_UAV);

    // Resource-specific supplemental use flags
    resourceDescription.usage = (FfxApiResourceUsage)(resourceDescription.usage | additionalUsages);

    resourceDescription.width    = createInfo.extent.width;
    resourceDescription.height   = createInfo.extent.height;
    resourceDescription.mipCount = createInfo.mipLevels;
    resourceDescription.format   = ffxGetSurfaceFormatVK(createInfo.format);

    // if the mutable flag is present, assume that the real format is sRGB
    if (FFX_CONTAINS_FLAG(createInfo.flags, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
        resourceDescription.format = ffxGetSurfaceFormatToGamma((FfxApiSurfaceFormat)resourceDescription.format);
    
    switch (createInfo.imageType)
    {
    case VK_IMAGE_TYPE_1D:
        resourceDescription.type = FFX_API_RESOURCE_TYPE_TEXTURE1D;
        break;
    case VK_IMAGE_TYPE_2D:
        resourceDescription.depth = createInfo.arrayLayers;
        if (FFX_CONTAINS_FLAG(additionalUsages, FFX_API_RESOURCE_USAGE_ARRAYVIEW))
            resourceDescription.type = FFX_API_RESOURCE_TYPE_TEXTURE2D;
        else if (FFX_CONTAINS_FLAG(createInfo.flags, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
            resourceDescription.type = FFX_API_RESOURCE_TYPE_TEXTURE_CUBE;
        else
            resourceDescription.type  = FFX_API_RESOURCE_TYPE_TEXTURE2D;
        break;
    case VK_IMAGE_TYPE_3D:
        resourceDescription.depth = createInfo.extent.depth;
        resourceDescription.type = FFX_API_RESOURCE_TYPE_TEXTURE3D;
        break;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: VK: Unsupported texture dimension requested. Please implement.");
        break;
    }

    return resourceDescription;
}

FfxErrorCode allocateDeviceMemory(BackendContext_VK* backendContext, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags requiredMemoryProperties, BackendContext_VK::Resource* backendResource)
{
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(backendContext->physicalDevice, memRequirements, requiredMemoryProperties, backendResource->memoryProperties);

    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendResource->deviceMemory);

    if (result != VK_SUCCESS) {
        switch (result) {
        case(VK_ERROR_OUT_OF_HOST_MEMORY):
        case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
            return FFX_ERROR_OUT_OF_MEMORY;
        default:
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    return FFX_OK;
}

void setVKObjectName(BackendContext_VK::VKFunctionTable& vkFunctionTable, VkDevice device, VkObjectType objectType, uint64_t object, char* name)
{
    VkDebugUtilsObjectNameInfoEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objectType, object, name };

    if (vkFunctionTable.vkSetDebugUtilsObjectNameEXT)
        vkFunctionTable.vkSetDebugUtilsObjectNameEXT(device, &s);
}

uint32_t getDynamicResourcesStartIndex(uint32_t effectContextId)
{
    // dynamic resources are tracked from the max index
    return (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
}
uint32_t getDynamicResourceViewsStartIndex(uint32_t effectContextId, uint32_t frameIndex)
{
    // dynamic resource views are tracked from the max index
    return (effectContextId * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2) + (frameIndex * FFX_MAX_RESOURCE_COUNT * 2) + (FFX_MAX_RESOURCE_COUNT * 2) - 1;
}

void destroyDynamicViews(BackendContext_VK* backendContext, uint32_t effectContextId, uint32_t frameIndex)
{
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Release image views for dynamic resources
    const uint32_t dynamicResourceViewIndexStart = getDynamicResourceViewsStartIndex(effectContextId, frameIndex);
    for (uint32_t dynamicViewIndex = effectContext.nextDynamicResourceView[frameIndex] + 1; dynamicViewIndex <= dynamicResourceViewIndexStart;
         ++dynamicViewIndex)
    {
        backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[dynamicViewIndex].imageView, VK_NULL_HANDLE);
        backendContext->pResourceViews[dynamicViewIndex].imageView = VK_NULL_HANDLE;
    }
    effectContext.nextDynamicResourceView[frameIndex] = dynamicResourceViewIndexStart;
}

VkAccessFlags getVKAccessFlagsFromResourceState(FfxApiResourceState state)
{
    switch (state) {

    case(FFX_API_RESOURCE_STATE_COMMON):
        return VK_ACCESS_NONE;
    case(FFX_API_RESOURCE_STATE_GENERIC_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case(FFX_API_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case(FFX_API_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_API_RESOURCE_STATE_PIXEL_READ):
    case(FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case FFX_API_RESOURCE_STATE_COPY_SRC:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case FFX_API_RESOURCE_STATE_COPY_DEST:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case FFX_API_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    case FFX_API_RESOURCE_STATE_PRESENT:
        return VK_ACCESS_NONE;
    case FFX_API_RESOURCE_STATE_RENDER_TARGET:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    case FFX_API_RESOURCE_STATE_DEPTH_ATTACHMENT:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    default:
        FFX_ASSERT_MESSAGE(false, "State flag not yet supported");
        return VK_ACCESS_SHADER_READ_BIT;
    }
}

VkPipelineStageFlags getVKPipelineStageFlagsFromResourceState(FfxApiResourceState state)
{
    switch (state) {

    case(FFX_API_RESOURCE_STATE_COMMON):
    case(FFX_API_RESOURCE_STATE_GENERIC_READ):
    case(FFX_API_RESOURCE_STATE_UNORDERED_ACCESS):
    case(FFX_API_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_API_RESOURCE_STATE_PIXEL_READ):
    case(FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case(FFX_API_RESOURCE_STATE_INDIRECT_ARGUMENT):
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    case FFX_API_RESOURCE_STATE_COPY_SRC:
    case FFX_API_RESOURCE_STATE_COPY_DEST:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case FFX_API_RESOURCE_STATE_PRESENT:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    case FFX_API_RESOURCE_STATE_RENDER_TARGET:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case FFX_API_RESOURCE_STATE_DEPTH_ATTACHMENT:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    default:
        FFX_ASSERT_MESSAGE(false, "Pipeline stage flag not yet supported");
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
}

VkImageLayout getVKImageLayoutFromResourceState(FfxApiResourceState state)
{
    switch (state) {

    case(FFX_API_RESOURCE_STATE_COMMON):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_API_RESOURCE_STATE_GENERIC_READ):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_API_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_API_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ):
    case(FFX_API_RESOURCE_STATE_PIXEL_READ):
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case FFX_API_RESOURCE_STATE_COPY_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case FFX_API_RESOURCE_STATE_COPY_DEST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case FFX_API_RESOURCE_STATE_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case FFX_API_RESOURCE_STATE_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case FFX_API_RESOURCE_STATE_DEPTH_ATTACHMENT:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case FFX_API_RESOURCE_STATE_INDIRECT_ARGUMENT:
        // this case is for buffers
    default:
        FFX_ASSERT_MESSAGE(false, "Image layout flag not yet supported");
        return VK_IMAGE_LAYOUT_GENERAL;
    }
}

void addMutableViewForSRV(VkImageViewCreateInfo& imageViewCreateInfo, VkImageViewUsageCreateInfo& imageViewUsageCreateInfo, FfxApiResourceDescription resourceDescription)
{
    if (ffxIsSurfaceFormatSRGB((FfxApiSurfaceFormat)resourceDescription.format) && FFX_CONTAINS_FLAG(resourceDescription.usage, FFX_API_RESOURCE_USAGE_UAV))
    {
        // mutable is only for sRGB textures that will need a storage
        imageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
        imageViewUsageCreateInfo.pNext = nullptr;
        imageViewUsageCreateInfo.usage = getVKImageUsageFlagsFromResourceUsage(FFX_API_RESOURCE_USAGE_READ_ONLY);  // we can assume that SRV is enough
        imageViewCreateInfo.pNext      = &imageViewUsageCreateInfo;
    }
}

void copyResourceState(BackendContext_VK::Resource* backendResource, const FfxApiResource* inFfxResource)
{
    FfxApiResourceState state = (FfxApiResourceState)inFfxResource->state;

    // copy the new states
    backendResource->initialState = state;
    backendResource->currentState = state;
    backendResource->undefined    = false;
    backendResource->dynamic      = true;

    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so tag the resource as undefined so we can transition it accordingly.
    if (backendResource->resourceDescription.flags & FFX_API_RESOURCE_FLAGS_UNDEFINED)
    {
        backendResource->undefined                 = true;
        backendResource->resourceDescription.flags = (FfxApiResourceFlags)((int)backendResource->resourceDescription.flags & ~FFX_API_RESOURCE_FLAGS_UNDEFINED);
    }
}

#ifdef _WIN32
void ConvertUTF8ToUTF16(const char* inputName, wchar_t* outputBuffer, size_t outputLen)
{
    if (MultiByteToWideChar(CP_UTF8, 0, inputName, -1, outputBuffer, static_cast<int>(outputLen)) == 0)
    {
        memset(outputBuffer, 0, outputLen * sizeof(wchar_t));
    }
}
void ConvertUTF16ToUTF8(const wchar_t* inputName, char* outputBuffer, size_t outputLen)
{
    if (WideCharToMultiByte(CP_UTF8, 0, inputName, -1, outputBuffer, static_cast<int>(outputLen * sizeof(char)), NULL, NULL) == 0)
    {
        memset(outputBuffer, 0, outputLen * sizeof(char));
    }
}
#else
void ConvertUTF8ToUTF16(const char* inputName, wchar_t* outputBuffer, size_t outputLen)
{
    memset(outputBuffer, 0, outputLen * sizeof(wchar_t));
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    wcscpy_s(outputBuffer, outputLen, converter.from_bytes(inputName).c_str());
}
void ConvertUTF16ToUTF8(const wchar_t* inputName, char* outputBuffer, size_t outputLen)
{
    memset(outputBuffer, 0, outputLen * sizeof(char));
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    strcpy_s(outputBuffer, outputLen, converter.to_bytes(inputName).c_str());
}
#endif  // _WIN32

void beginMarkerVK(BackendContext_VK* backendContext, VkCommandBuffer commandBuffer, const wchar_t* label)
{
    if(backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT == nullptr)
    {
        return;
    }

    constexpr size_t strLen = 64;
    char strLabel[strLen];
    ConvertUTF16ToUTF8(label, strLabel, strLen);

    VkDebugUtilsLabelEXT debugLabel = {};
    debugLabel.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    debugLabel.pNext                = nullptr;
    debugLabel.pLabelName           = strLabel;

    // not to saturated red
    debugLabel.color[0] = 1.0f;
    debugLabel.color[1] = 0.14f;
    debugLabel.color[2] = 0.14f;
    debugLabel.color[3] = 1.0f;
    backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);
}

void endMarkerVK(BackendContext_VK* backendContext, VkCommandBuffer commandBuffer)
{
    if(backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT != nullptr)
    {
        backendContext->vkFunctionTable.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    }
}

void addBarrier(BackendContext_VK* backendContext, FfxResourceInternal* resource, FfxApiResourceState newState)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != resource);

    BackendContext_VK::Resource& ffxResource = backendContext->pResources[resource->internalIndex];

    if (ffxResource.resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResource = ffxResource.bufferResource;
        VkBufferMemoryBarrier* barrier = &backendContext->bufferMemoryBarriers[backendContext->scheduledBufferBarrierCount];

        FfxApiResourceState& curState = backendContext->pResources[resource->internalIndex].currentState;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->buffer = vkResource;
        barrier->offset = 0;
        barrier->size = VK_WHOLE_SIZE;

        backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
        backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

        curState = newState;

        ++backendContext->scheduledBufferBarrierCount;
    }
    else
    {
        VkImage vkResource = ffxResource.imageResource;

        FfxApiResourceState& curState   = backendContext->pResources[resource->internalIndex].currentState;

        VkImageSubresourceRange range;
        range.aspectMask     = getImageAspect((FfxApiResourceUsage) ffxResource.resourceDescription.usage);
        range.baseMipLevel   = 0;
        range.levelCount     = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        VkImageLayout oldLayout = ffxResource.undefined ? VK_IMAGE_LAYOUT_UNDEFINED : getVKImageLayoutFromResourceState(curState);
        VkImageLayout newLayout = getVKImageLayoutFromResourceState(newState);

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            // Add an intermediate transition to get rid of the validation warning:
            // we are transitioning from undefined state (which means the content of the texture is undefined) to a read state.
            // Vulkan triggers a warning in this case.
            // More case might exist. We will add them when we meet them.

            // add intermediate barrier
            VkImageMemoryBarrier* imageBarrier = &backendContext->imageMemoryBarriers[backendContext->scheduledImageBarrierCount];

            imageBarrier->sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier->pNext               = nullptr;
            imageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier->subresourceRange    = range;
            imageBarrier->image               = vkResource;

            imageBarrier->srcAccessMask = 0;
            imageBarrier->oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageUsageFlags usage = getVKImageUsageFlagsFromResourceUsage((FfxApiResourceUsage)ffxResource.resourceDescription.usage);
            if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0) 
            {
                imageBarrier->dstAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarrier->newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_RENDER_TARGET);
                // dstAccessMask is used as srcAccessMask in the next barrier, so srcStageMask need this stage for the next barrier
                backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_RENDER_TARGET);
            } 
            else if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) 
            {
                imageBarrier->dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                imageBarrier->newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_DEPTH_ATTACHMENT);
                backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_DEPTH_ATTACHMENT);
            } 
            else if ((usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0) 
            {
                imageBarrier->dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageBarrier->newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_COPY_DEST);
                backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_COPY_DEST);
            } 
            else if ((usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0) 
            {
                imageBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                imageBarrier->newLayout     = VK_IMAGE_LAYOUT_GENERAL;
                backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
                backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            } 
            else 
            {
                FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_WARNING, L"Unable to find an appropriate intermediate transition. Please support this case.");
            }

            ++backendContext->scheduledImageBarrierCount;

            // add final barrier
            VkImageMemoryBarrier* barrier = &backendContext->imageMemoryBarriers[backendContext->scheduledImageBarrierCount];
            barrier->sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier->pNext                = nullptr;
            barrier->srcAccessMask        = imageBarrier->dstAccessMask;
            barrier->dstAccessMask        = getVKAccessFlagsFromResourceState(newState);
            barrier->oldLayout            = imageBarrier->newLayout;
            barrier->newLayout            = getVKImageLayoutFromResourceState(newState);

            barrier->srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            barrier->dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            barrier->image                = vkResource;
            barrier->subresourceRange     = range;

            backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

            curState = newState;

            ++backendContext->scheduledImageBarrierCount;
        }
        else
        {
            VkImageMemoryBarrier* barrier = &backendContext->imageMemoryBarriers[backendContext->scheduledImageBarrierCount];

            barrier->sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier->pNext         = nullptr;
            barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
            barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
            barrier->oldLayout     = ffxResource.undefined ? VK_IMAGE_LAYOUT_UNDEFINED : getVKImageLayoutFromResourceState(curState);
            barrier->newLayout     = getVKImageLayoutFromResourceState(newState);

            barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier->image               = vkResource;
            barrier->subresourceRange    = range;

            backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
            backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

            curState = newState;

            ++backendContext->scheduledImageBarrierCount;
        }
    }

    if (ffxResource.undefined)
        ffxResource.undefined = false;
}

void flushBarriers(BackendContext_VK* backendContext, VkCommandBuffer vkCommandBuffer)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != vkCommandBuffer);

    if (backendContext->scheduledImageBarrierCount > 0 || backendContext->scheduledBufferBarrierCount > 0)
    {
        backendContext->vkFunctionTable.vkCmdPipelineBarrier(vkCommandBuffer, backendContext->srcStageMask, backendContext->dstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, backendContext->scheduledBufferBarrierCount, backendContext->bufferMemoryBarriers, backendContext->scheduledImageBarrierCount, backendContext->imageMemoryBarriers);
        backendContext->scheduledImageBarrierCount = 0;
        backendContext->scheduledBufferBarrierCount = 0;
        backendContext->srcStageMask = 0;
        backendContext->dstStageMask = 0;
    }
}

FfxApiConstantBufferAllocation BackendContext_VK::FallbackConstantAllocator(void* data, FfxUInt64 dataSize)
{
    FfxApiConstantBufferAllocation       allocation;
    std::lock_guard<std::mutex> cbLock{uniformBufferMutex};

    if (!uniformBufferMem)
    {
        // allocate dynamic uniform buffer

        // get alignment
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        uniformBufferAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

        uniformBufferSize = FFX_ALIGN_UP(FFX_BUFFER_SIZE, uniformBufferAlignment) * maxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = static_cast<VkDeviceSize>(uniformBufferSize);
        bufferInfo.usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VkResult res = vkFunctionTable.vkCreateBuffer(device, &bufferInfo, NULL, &uniformBuffer);
        FFX_ASSERT(res == VK_SUCCESS);

        VkMemoryAllocateInfo allocInfo = {};
        if (res == VK_SUCCESS)
        {
            // allocate memory block for all uniform buffers
            VkMemoryRequirements memRequirements = {};
            vkFunctionTable.vkGetBufferMemoryRequirements(device, uniformBuffer, &memRequirements);

            // this is the real alignment
            uniformBufferAlignment = memRequirements.alignment;

            VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize  = uniformBufferSize;
            allocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements, requiredMemoryProperties, uniformBufferMemoryProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX)
            {
                requiredMemoryProperties  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements, requiredMemoryProperties, uniformBufferMemoryProperties);

                if (allocInfo.memoryTypeIndex == UINT32_MAX)
                    res = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        if (res == VK_SUCCESS)
            res = vkFunctionTable.vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory);
        FFX_ASSERT(res == VK_SUCCESS);

        if (res == VK_SUCCESS)
            res = vkFunctionTable.vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
        FFX_ASSERT(res == VK_SUCCESS);

        // map the memory block
        if (res == VK_SUCCESS)
            res = vkFunctionTable.vkMapMemory(device, uniformBufferMemory, 0, uniformBufferSize, 0, &uniformBufferMem);
        FFX_ASSERT(res == VK_SUCCESS);
    }

    FFX_ASSERT(uniformBufferMem);

    allocation.resource.resource = uniformBuffer;
    allocation.handle            = 0;

    if (data)
    {
        if (uniformBufferOffset + dataSize >= uniformBufferSize)
            uniformBufferOffset = 0;

        allocation.handle = static_cast<FfxUInt64>(uniformBufferOffset);

        void* pBuffer = (void*)((uint8_t*)(uniformBufferMem) + uniformBufferOffset);
        memcpy(pBuffer, data, dataSize);

        // TODO: ensure that we aren't writing on some used memory

        // flush mapped range if memory type is not coherent
        if ((uniformBufferMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange memoryRange;
            memset(&memoryRange, 0, sizeof(memoryRange));

            memoryRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.memory = uniformBufferMemory;
            memoryRange.offset = uniformBufferOffset;
            memoryRange.size   = dataSize;

            vkFunctionTable.vkFlushMappedMemoryRanges(device, 1, &memoryRange);
        }

        uniformBufferOffset += FFX_ALIGN_UP(dataSize, uniformBufferAlignment);
        if (uniformBufferOffset > uniformBufferSize)
            uniformBufferOffset = 0;
    }

    return allocation;
}

void resetBackendContext(BackendContext_VK* backendContext)
{
    // reset the context except the maxEffectContexts in case the memory is reused for a new context
    uint32_t maxEffectContexts = backendContext->maxEffectContexts;

    memset(backendContext, 0, sizeof(BackendContext_VK));

    // restore the maxEffectContexts
    backendContext->maxEffectContexts = maxEffectContexts;
}

//////////////////////////////////////////////////////////////////////////
// VK back end implementation

FfxErrorCode GetEffectGpuMemoryUsageVK(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxApiEffectMemoryUsage* outVramUsage)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != outVramUsage);

    BackendContext_VK*                backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    *outVramUsage = effectContext.vramUsage;

    return FFX_OK;
}

FfxErrorCode ffxGetResourceSizeFromDescriptionVK(FfxDevice device, const FfxCreateResourceDescription* createResourceDescription, uint64_t* sizeInBytes, uint64_t* alignment)
{
    FFX_ASSERT(NULL != device);

    // Setup the resource description
    FfxApiResourceDescription resourceDesc = createResourceDescription->resourceDescription;

    if (resourceDesc.mipCount == 0) {
        resourceDesc.mipCount = (uint32_t)(1 + floor(log2(FFX_MAXIMUM(FFX_MAXIMUM(createResourceDescription->resourceDescription.width,
            createResourceDescription->resourceDescription.height), createResourceDescription->resourceDescription.depth))));
    }

    const auto& initData = createResourceDescription->initData;

    const FfxApiResourceState resourceState =
        ((initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED) && (createResourceDescription->heapInfo.heapType != FFX_HEAP_TYPE_UPLOAD))
        ? FFX_API_RESOURCE_STATE_COPY_DEST
        : createResourceDescription->initialState;

    VkMemoryRequirements memRequirements = {};
    VkDeviceContext* vkDeviceContext = reinterpret_cast<VkDeviceContext*>(device);
    if (!vkDeviceContext)
    {
        return FFX_ERROR_NULL_DEVICE;
    }

    VkDevice vkDevice = vkDeviceContext->vkDevice;

    switch (createResourceDescription->resourceDescription.type)
    {
    case FFX_API_RESOURCE_TYPE_BUFFER:
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = createResourceDescription->resourceDescription.width;
        bufferInfo.usage = ffxGetVKBufferUsageFlagsFromResourceUsage((FfxApiResourceUsage)resourceDesc.usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (resourceState == FFX_API_RESOURCE_STATE_COPY_SRC)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (resourceState == FFX_API_RESOURCE_STATE_COPY_DEST)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VkBuffer bufferResource;
        if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &bufferResource) != VK_SUCCESS) {
            return FFX_ERROR_INVALID_ARGUMENT;
        }

        vkGetBufferMemoryRequirements(vkDevice, bufferResource, &memRequirements);
        *sizeInBytes = memRequirements.size;
        if (alignment)
        {
            *alignment = memRequirements.alignment > 0 ? memRequirements.alignment : 1u;
        }

        vkDestroyBuffer(vkDevice, bufferResource, nullptr);

        break;
    }
    case FFX_API_RESOURCE_TYPE_TEXTURE1D:
    case FFX_API_RESOURCE_TYPE_TEXTURE2D:
    case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_API_RESOURCE_TYPE_TEXTURE3D:
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = ffxGetVKImageTypeFromResourceType((FfxApiResourceType)createResourceDescription->resourceDescription.type);
        imageInfo.extent.width = createResourceDescription->resourceDescription.width;
        imageInfo.extent.height = createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE1D ? 1 : createResourceDescription->resourceDescription.height;
        imageInfo.extent.depth = (createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE3D || createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE_CUBE) ?
            createResourceDescription->resourceDescription.depth : 1;
        imageInfo.mipLevels = resourceDesc.mipCount;
        imageInfo.arrayLayers = (createResourceDescription->resourceDescription.type ==
            FFX_API_RESOURCE_TYPE_TEXTURE1D || createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE2D)
            ? createResourceDescription->resourceDescription.depth : 1;
        imageInfo.format = getVkFormatFromSurfaceFormatAndUsage((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format, (FfxApiResourceUsage)resourceDesc.usage);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = getVKImageUsageFlagsFromResourceUsage((FfxApiResourceUsage)resourceDesc.usage);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (FFX_CONTAINS_FLAG(resourceDesc.usage, FFX_API_RESOURCE_USAGE_UAV) && ffxIsSurfaceFormatSRGB((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageInfo.format = ffxGetVkFormatFromSurfaceFormat(ffxGetSurfaceFormatFromGamma((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format));
        }

        VkImage imageResource;
        if (vkCreateImage(vkDevice, &imageInfo, nullptr, &imageResource) != VK_SUCCESS) {
            return FFX_ERROR_INVALID_ARGUMENT;
        }

        vkGetImageMemoryRequirements(vkDevice, imageResource, &memRequirements);
        *sizeInBytes = memRequirements.size;
        if (alignment)
        {
            *alignment = memRequirements.alignment > 0 ? memRequirements.alignment : 1u;
        }

        vkDestroyImage(vkDevice, imageResource, nullptr);

        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource type creation requested.");
        break;
    }

    if (*sizeInBytes != UINT64_MAX)
    {
        return FFX_OK;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}

FfxErrorCode ffxGetBackendContextGpuMemoryUsageVK(FfxDevice, size_t, uint64_t* byteCount)
{
    *byteCount = 0;
    return FFX_OK;
}

FfxErrorCode CreateBackendContextVK(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId)
{
    VkDeviceContext* vkDeviceContext = reinterpret_cast<VkDeviceContext*>(backendInterface->device);

    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != vkDeviceContext);
    FFX_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkDevice);
    FFX_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkPhysicalDevice);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!backendContext->refCount) {

        resetBackendContext(backendContext);

        new (&backendContext->uniformBufferMutex) std::mutex();

        // Map all of our pointers
        uint32_t gpuJobDescArraySize   = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint32_t resourceViewArraySize = FFX_ALIGN_UP(((backendContext->maxEffectContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2) + FFX_MAX_BINDLESS_DESCRIPTOR_COUNT) * sizeof(BackendContext_VK::VkResourceView), sizeof(uint32_t));
        uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
        uint32_t pipelineArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(uint32_t));
        uint32_t resourceArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(uint32_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * sizeof(BackendContext_VK::EffectContext), sizeof(uint32_t));
        uint8_t* pMem = (uint8_t*)((BackendContext_VK*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        pMem += gpuJobDescArraySize;

        // Map the resource view array
        backendContext->pResourceViews = (BackendContext_VK::VkResourceView*)(pMem);
        pMem += resourceViewArraySize;

        // Map the staging ring buffer array
        backendContext->pStagingRingBuffer = (uint8_t*)pMem;
        pMem += stagingRingBufferArraySize;

        // Map pipeline array
        backendContext->pPipelineLayouts = (BackendContext_VK::PipelineLayout*)pMem;
        pMem += pipelineArraySize;

        // Map resource array
        backendContext->pResources = (BackendContext_VK::Resource*)pMem;
        pMem += resourceArraySize;

        // Clear out all resource mappings
        for (uint32_t i = 0; i < backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT; ++i) {
            backendContext->pResources[i].uavViewIndex = backendContext->pResources[i].srvViewIndex = -1;
        }

        // Map context array
        backendContext->pEffectContexts = (BackendContext_VK::EffectContext*)pMem;
        pMem += contextArraySize;

        // Map extension array
        backendContext->extensionProperties = (VkExtensionProperties*)pMem;

        // if vkGetDeviceProcAddr is NULL, use the one from the vulkan header
        if (vkDeviceContext->vkDeviceProcAddr == NULL)
            vkDeviceContext->vkDeviceProcAddr = vkGetDeviceProcAddr;

        if (vkDeviceContext->vkDevice != VK_NULL_HANDLE) {
            backendContext->device = vkDeviceContext->vkDevice;
        }

        if (vkDeviceContext->vkPhysicalDevice != VK_NULL_HANDLE) {
            backendContext->physicalDevice = vkDeviceContext->vkPhysicalDevice;
        }

        // load vulkan functions
        backendContext->vkFunctionTable.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkSetDebugUtilsObjectNameEXT");
        backendContext->vkFunctionTable.vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFlushMappedMemoryRanges");
        backendContext->vkFunctionTable.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateDescriptorPool");
        backendContext->vkFunctionTable.vkCreateSampler = (PFN_vkCreateSampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateSampler");
        backendContext->vkFunctionTable.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateDescriptorSetLayout");
        backendContext->vkFunctionTable.vkCreateBuffer = (PFN_vkCreateBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateBuffer");
        backendContext->vkFunctionTable.vkCreateBufferView = (PFN_vkCreateBufferView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateBufferView");
        backendContext->vkFunctionTable.vkCreateImage = (PFN_vkCreateImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateImage");
        backendContext->vkFunctionTable.vkCreateImageView = (PFN_vkCreateImageView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateImageView");
        backendContext->vkFunctionTable.vkCreateShaderModule = (PFN_vkCreateShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateShaderModule");
        backendContext->vkFunctionTable.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreatePipelineLayout");
        backendContext->vkFunctionTable.vkCreateComputePipelines = (PFN_vkCreateComputePipelines)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateComputePipelines");
        backendContext->vkFunctionTable.vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyPipelineLayout");
        backendContext->vkFunctionTable.vkDestroyPipeline = (PFN_vkDestroyPipeline)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyPipeline");
        backendContext->vkFunctionTable.vkDestroyImage = (PFN_vkDestroyImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyImage");
        backendContext->vkFunctionTable.vkDestroyImageView = (PFN_vkDestroyImageView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyImageView");
        backendContext->vkFunctionTable.vkDestroyBuffer = (PFN_vkDestroyBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyBuffer");
        backendContext->vkFunctionTable.vkDestroyBufferView = (PFN_vkDestroyBufferView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyBufferView");
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorSetLayout");
        backendContext->vkFunctionTable.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorPool");
        backendContext->vkFunctionTable.vkDestroySampler = (PFN_vkDestroySampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroySampler");
        backendContext->vkFunctionTable.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyShaderModule");
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetBufferMemoryRequirements");
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetBufferMemoryRequirements2KHR");
        backendContext->vkFunctionTable.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetImageMemoryRequirements");
        backendContext->vkFunctionTable.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateDescriptorSets");
        backendContext->vkFunctionTable.vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFreeDescriptorSets");
        backendContext->vkFunctionTable.vkAllocateMemory = (PFN_vkAllocateMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateMemory");
        backendContext->vkFunctionTable.vkFreeMemory = (PFN_vkFreeMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFreeMemory");
        backendContext->vkFunctionTable.vkMapMemory = (PFN_vkMapMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkMapMemory");
        backendContext->vkFunctionTable.vkUnmapMemory = (PFN_vkUnmapMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkUnmapMemory");
        backendContext->vkFunctionTable.vkBindBufferMemory = (PFN_vkBindBufferMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkBindBufferMemory");
        backendContext->vkFunctionTable.vkBindImageMemory = (PFN_vkBindImageMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkBindImageMemory");
        backendContext->vkFunctionTable.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkUpdateDescriptorSets");
        backendContext->vkFunctionTable.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdPipelineBarrier");
        backendContext->vkFunctionTable.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBindPipeline");
        backendContext->vkFunctionTable.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBindDescriptorSets");
        backendContext->vkFunctionTable.vkCmdDispatch = (PFN_vkCmdDispatch)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDispatch");
        backendContext->vkFunctionTable.vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDispatchIndirect");
        backendContext->vkFunctionTable.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyBuffer");
        backendContext->vkFunctionTable.vkCmdCopyImage = (PFN_vkCmdCopyImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyImage");
        backendContext->vkFunctionTable.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyBufferToImage");
        backendContext->vkFunctionTable.vkCmdClearColorImage = (PFN_vkCmdClearColorImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdClearColorImage");
        backendContext->vkFunctionTable.vkCmdFillBuffer = (PFN_vkCmdFillBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdFillBuffer");
        backendContext->vkFunctionTable.vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdWriteBufferMarkerAMD");
        backendContext->vkFunctionTable.vkCmdWriteBufferMarker2AMD = (PFN_vkCmdWriteBufferMarker2AMD)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdWriteBufferMarker2AMD");
        backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBeginDebugUtilsLabelEXT");
        backendContext->vkFunctionTable.vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdEndDebugUtilsLabelEXT");

        // enumerate all the device extensions
        backendContext->numDeviceExtensions = 0;
        vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, nullptr);
        vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, backendContext->extensionProperties);

        // create a global descriptor pool to hold all descriptors we'll need
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_SAMPLER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
        };

        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(_countof(poolSizes));
        descriptorPoolCreateInfo.pPoolSizes = poolSizes;
        descriptorPoolCreateInfo.maxSets = backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * MAX_PIPELINE_USAGE_PER_FRAME * FFX_MAX_QUEUED_FRAMES;

        if (backendContext->vkFunctionTable.vkCreateDescriptorPool(backendContext->device, &descriptorPoolCreateInfo, nullptr, &backendContext->descriptorPool) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        // set bindless resource view to base
        backendContext->bindlessBase = (backendContext->maxEffectContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2);

        // allocate dynamic uniform buffer
        {
            // get alignment
            VkPhysicalDeviceProperties physicalDeviceProperties = {};
            vkGetPhysicalDeviceProperties(backendContext->physicalDevice, &physicalDeviceProperties);
            backendContext->uniformBufferAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

            backendContext->uniformBufferSize =
                FFX_ALIGN_UP(FFX_BUFFER_SIZE, backendContext->uniformBufferAlignment) * backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES;

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size               = static_cast<VkDeviceSize>(backendContext->uniformBufferSize);
            bufferInfo.usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

            if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &backendContext->uniformBuffer) != VK_SUCCESS)
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            // allocate memory block for all uniform buffers
            VkMemoryRequirements memRequirements = {};
            backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendContext->uniformBuffer, &memRequirements);

            // this is the real alignment
            backendContext->uniformBufferAlignment = memRequirements.alignment;

            VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = backendContext->uniformBufferSize;
            allocInfo.memoryTypeIndex =
                findMemoryTypeIndex(backendContext->physicalDevice, memRequirements, requiredMemoryProperties, backendContext->uniformBufferMemoryProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX)
            {
                requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.memoryTypeIndex = findMemoryTypeIndex(
                    backendContext->physicalDevice, memRequirements, requiredMemoryProperties, backendContext->uniformBufferMemoryProperties);

                if (allocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendContext->uniformBufferMemory);

            if (result != VK_SUCCESS)
            {
                switch (result)
                {
                case (VK_ERROR_OUT_OF_HOST_MEMORY):
                case (VK_ERROR_OUT_OF_DEVICE_MEMORY):
                    return FFX_ERROR_OUT_OF_MEMORY;
                default:
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            // map the memory block
            if (backendContext->vkFunctionTable.vkMapMemory(
                    backendContext->device, backendContext->uniformBufferMemory, 0, backendContext->uniformBufferSize, 0, &backendContext->uniformBufferMem) !=
                VK_SUCCESS)
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            if (backendContext->vkFunctionTable.vkBindBufferMemory(
                    backendContext->device, backendContext->uniformBuffer, backendContext->uniformBufferMemory, 0) != VK_SUCCESS)
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }
        }
    }

    // Increment the ref count
    ++backendContext->refCount;

    // Get an available context id
    for (uint32_t i = 0; i < backendContext->maxEffectContexts; ++i) {
        if (!backendContext->pEffectContexts[i].active) {
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.effectId = effect;

            effectContext.nextStaticResource = (i * FFX_MAX_RESOURCE_COUNT) + 1;
            effectContext.nextDynamicResource = getDynamicResourcesStartIndex(i);
            effectContext.nextStaticResourceView = (i * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2);
            for (uint32_t frameIndex = 0; frameIndex < FFX_MAX_QUEUED_FRAMES; ++frameIndex)
            {
                effectContext.nextDynamicResourceView[frameIndex] = getDynamicResourceViewsStartIndex(i, frameIndex);
            }
            effectContext.nextPipelineLayout = (i * FFX_MAX_PASS_COUNT);
            effectContext.frameIndex = 0;

            if (bindlessConfig)
            {
                effectContext.bindlessTextureSrvHeapStart = backendContext->bindlessBase;
                effectContext.bindlessTextureSrvHeapSize  = bindlessConfig->maxTextureSrvs;

                backendContext->bindlessBase += bindlessConfig->maxTextureSrvs;

                effectContext.bindlessBufferSrvHeapSize  = bindlessConfig->maxBufferSrvs;

                effectContext.bindlessTextureUavHeapStart = backendContext->bindlessBase;
                effectContext.bindlessTextureUavHeapSize  = bindlessConfig->maxTextureUavs;

                backendContext->bindlessBase += bindlessConfig->maxTextureUavs;

                effectContext.bindlessBufferUavHeapSize  = bindlessConfig->maxBufferUavs;

                
                // create a bindless descriptor pool local to the current effect
                VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
                VkDescriptorPoolSize       poolSizes[3];

                uint32_t poolSizeCount = 0;

                if (bindlessConfig->maxTextureSrvs > 0)
                {
                    VkDescriptorPoolSize& poolSize = poolSizes[poolSizeCount++];

                    poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    poolSize.descriptorCount = bindlessConfig->maxTextureSrvs;
                }

                if (bindlessConfig->maxTextureUavs > 0)
                {
                    VkDescriptorPoolSize& poolSize = poolSizes[poolSizeCount++];

                    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    poolSize.descriptorCount = bindlessConfig->maxTextureUavs;
                }

                if (bindlessConfig->maxBufferSrvs > 0 || bindlessConfig->maxBufferUavs > 0)
                {
                    VkDescriptorPoolSize& poolSize = poolSizes[poolSizeCount++];

                    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    poolSize.descriptorCount = bindlessConfig->maxBufferSrvs + bindlessConfig->maxBufferUavs;
                }

                descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                descriptorPoolCreateInfo.pNext         = nullptr;
                // The bindless sets live for the pool's lifetime and are freed collectively by
                // vkDestroyDescriptorPool at context teardown, so no FREE_DESCRIPTOR_SET_BIT is needed.
                descriptorPoolCreateInfo.flags         = 0;
                descriptorPoolCreateInfo.poolSizeCount = poolSizeCount;
                descriptorPoolCreateInfo.pPoolSizes    = poolSizes;
                descriptorPoolCreateInfo.maxSets       = poolSizeCount;

                if (backendContext->vkFunctionTable.vkCreateDescriptorPool(
                        backendContext->device, &descriptorPoolCreateInfo, nullptr, &effectContext.bindlessDescriptorPool) != VK_SUCCESS)
                {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }

                // create the descriptor layout for bindless texture srv buffers
                if (bindlessConfig->maxTextureSrvs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, bindlessConfig->maxTextureSrvs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessTextureSrvDescriptorSetLayout) != VK_SUCCESS)
                    {
                     
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessTextureSrvDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessTextureSrvDescriptorSet);
                }

                // create the descriptor layout for bindless buffer srv buffers
                if (bindlessConfig->maxBufferSrvs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindlessConfig->maxBufferSrvs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessBufferSrvDescriptorSetLayout) != VK_SUCCESS)
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessBufferSrvDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessBufferSrvDescriptorSet);
                }

                // create the descriptor layout for bindless texture uav buffers
                if (bindlessConfig->maxTextureUavs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bindlessConfig->maxTextureUavs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessTextureUavDescriptorSetLayout) != VK_SUCCESS)
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessTextureUavDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessTextureUavDescriptorSet);
                }

                // create the descriptor layout for bindless buffer uav buffers
                if (bindlessConfig->maxBufferUavs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindlessConfig->maxBufferUavs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessBufferUavDescriptorSetLayout) != VK_SUCCESS)
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessBufferUavDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessBufferUavDescriptorSet);
                }
            }
            else
            {
                effectContext.bindlessTextureSrvHeapStart = 0;
                effectContext.bindlessTextureSrvHeapSize  = 0;
                effectContext.bindlessBufferSrvHeapSize   = 0;
                effectContext.bindlessTextureUavHeapStart = 0;
                effectContext.bindlessTextureUavHeapSize  = 0;
                effectContext.bindlessBufferUavHeapSize   = 0;
            }   

            break;
        }
    }

    return FFX_OK;
}

FfxErrorCode GetDeviceCapabilitiesVK(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // no shader model in vulkan so assume the minimum
    deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
    deviceCapabilities->waveLaneCountMin = 32;
    deviceCapabilities->waveLaneCountMax = 32;
    deviceCapabilities->fp16Supported = false;
    deviceCapabilities->raytracingSupported = false;
    deviceCapabilities->deviceCoherentMemorySupported = false;
    deviceCapabilities->dedicatedAllocationSupported = false;
    deviceCapabilities->bufferMarkerSupported = false;
    deviceCapabilities->extendedSynchronizationSupported = false;
    deviceCapabilities->shaderStorageBufferArrayNonUniformIndexing = false;

    BackendContext_VK* context = (BackendContext_VK*)backendInterface->scratchBuffer;

    // check if extensions are enabled

    for (uint32_t i = 0; i < backendContext->numDeviceExtensions; i++)
    {
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME) == 0)
        {
            // check if we the max subgroup size allows us to use wave64
            VkPhysicalDeviceSubgroupSizeControlProperties subgroupSizeControlProperties = {};
            subgroupSizeControlProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;

            VkPhysicalDeviceProperties2 deviceProperties2 = {};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            deviceProperties2.pNext = &subgroupSizeControlProperties;
            vkGetPhysicalDeviceProperties2(context->physicalDevice, &deviceProperties2);

            deviceCapabilities->waveLaneCountMin = subgroupSizeControlProperties.minSubgroupSize;
            deviceCapabilities->waveLaneCountMax = subgroupSizeControlProperties.maxSubgroupSize;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME) == 0)
        {
            // check for fp16 support
            VkPhysicalDeviceShaderFloat16Int8Features shaderFloat18Int8Features = {};
            shaderFloat18Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &shaderFloat18Int8Features;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->fp16Supported = (bool)shaderFloat18Int8Features.shaderFloat16;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
        {
            // check for ray tracing support
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
            accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &accelerationStructureFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->raytracingSupported = (bool)accelerationStructureFeatures.accelerationStructure;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) == 0)
        {
            // check for coherent memory support
            VkPhysicalDeviceCoherentMemoryFeaturesAMD coherentMemoryFeatures = {};
            coherentMemoryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &coherentMemoryFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->deviceCoherentMemorySupported = (bool)coherentMemoryFeatures.deviceCoherentMemory;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0)
        {
            // no features structure so extension name is enough
            deviceCapabilities->dedicatedAllocationSupported = true;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_AMD_BUFFER_MARKER_EXTENSION_NAME) == 0)
        {
            // no features structure so extension name is enough
            deviceCapabilities->bufferMarkerSupported = true;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) == 0)
        {
            // check for extended synchronization support
            VkPhysicalDeviceSynchronization2FeaturesKHR synchronizationFeatures = {};
            synchronizationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &synchronizationFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->extendedSynchronizationSupported = (bool)synchronizationFeatures.synchronization2;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0)
        {
            // check for coherent memory support
            VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
            descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &descriptorIndexingFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->shaderStorageBufferArrayNonUniformIndexing = (bool)descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing;
        }
    }

    return FFX_OK;
}

FfxErrorCode DestroyBackendContextVK(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    FFX_ASSERT(backendContext->refCount > 0);

    // Delete any resources allocated by this context
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (uint32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex) {

        if (backendContext->pResources[currentStaticResourceIndex].imageResource != VK_NULL_HANDLE) {
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxResourceInternal internalResource = { static_cast<int32_t>(currentStaticResourceIndex) };
            DestroyResourceVK(backendInterface, internalResource, effectContextId);
        }
    }

    for (uint32_t frameIndex = 0; frameIndex < FFX_MAX_QUEUED_FRAMES; ++frameIndex)
        destroyDynamicViews(backendContext, effectContextId, frameIndex);

    // clean up descriptor set layouts
    if (effectContext.bindlessTextureSrvDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessTextureSrvDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessTextureSrvDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessBufferSrvDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessBufferSrvDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessBufferSrvDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessTextureUavDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessTextureUavDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessTextureUavDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessBufferUavDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessBufferUavDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessBufferUavDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessDescriptorPool)
    {
        backendContext->vkFunctionTable.vkDestroyDescriptorPool(backendContext->device, effectContext.bindlessDescriptorPool, VK_NULL_HANDLE);
        effectContext.bindlessDescriptorPool = VK_NULL_HANDLE;
    }

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --backendContext->refCount;

    if (!backendContext->refCount) {

        // clean up descriptor pool
        backendContext->vkFunctionTable.vkDestroyDescriptorPool(backendContext->device, backendContext->descriptorPool, VK_NULL_HANDLE);
        backendContext->descriptorPool = VK_NULL_HANDLE;

        // clean up dynamic uniform buffer & memory
        backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendContext->uniformBufferMemory);
        backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backendContext->uniformBufferMemory, VK_NULL_HANDLE);
        backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, backendContext->uniformBuffer, VK_NULL_HANDLE);

        backendContext->device = VK_NULL_HANDLE;
        backendContext->physicalDevice = VK_NULL_HANDLE;

        resetBackendContext(backendContext);
    }

    return FFX_OK;
}

// create a internal resource that will stay alive until effect gets shut down
FfxErrorCode CreateResourceVK(
    FfxInterface* backendInterface,
    const FfxCreateResourceDescription* createResourceDescription,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outResource)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != createResourceDescription);
    FFX_ASSERT(NULL != outResource);
    FFX_ASSERT_MESSAGE(createResourceDescription->initData.type != FFX_RESOURCE_INIT_DATA_TYPE_INVALID,
                       "InitData type cannot be FFX_API_RESOURCE_INIT_DATA_TYPE_INVALID. Please explicitly specify the resource initialization type.");

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    VkDevice vkDevice = backendContext->device;

    FFX_ASSERT(VK_NULL_HANDLE != vkDevice);

    VkMemoryPropertyFlags requiredMemoryProperties;

    switch (createResourceDescription->heapInfo.heapType)
    {
    case FFX_HEAP_TYPE_DEFAULT:
        requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case FFX_HEAP_TYPE_UPLOAD:
    case FFX_HEAP_TYPE_READBACK:
        requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    default:
        requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    }

    // Setup the resource description
    FfxApiResourceDescription resourceDesc = createResourceDescription->resourceDescription;

    if (resourceDesc.mipCount == 0) {
        resourceDesc.mipCount = (uint32_t)(1 + floor(log2(FFX_MAXIMUM(FFX_MAXIMUM(createResourceDescription->resourceDescription.width,
            createResourceDescription->resourceDescription.height), createResourceDescription->resourceDescription.depth))));
    }

    FFX_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);
    outResource->internalIndex = effectContext.nextStaticResource++;
    BackendContext_VK::Resource* backendResource = &backendContext->pResources[outResource->internalIndex];
    backendResource->undefined = true;  // A flag to make sure the first barrier for this image resource always uses an src layout of undefined
    backendResource->dynamic = false;   // Not a dynamic resource (need to track them separately for image views)
    backendResource->resourceDescription = resourceDesc;
    backendResource->allocationSize = 0;

    const auto& initData = createResourceDescription->initData;

    const FfxApiResourceState resourceState =
        ((initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED) && (createResourceDescription->heapInfo.heapType != FFX_HEAP_TYPE_UPLOAD))
            ? FFX_API_RESOURCE_STATE_COPY_DEST
            : createResourceDescription->initialState;
    backendResource->initialState = resourceState;
    backendResource->currentState = resourceState;

#ifdef _DEBUG
    size_t retval = 0;
    wcstombs_s(&retval, backendResource->resourceName, sizeof(backendResource->resourceName), createResourceDescription->name, sizeof(backendResource->resourceName));
    if (retval >= 64) backendResource->resourceName[63] = '\0';
#endif

    VkMemoryRequirements memRequirements = {};

    switch (createResourceDescription->resourceDescription.type)
    {
    case FFX_API_RESOURCE_TYPE_BUFFER:
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = createResourceDescription->resourceDescription.width;
        bufferInfo.usage = ffxGetVKBufferUsageFlagsFromResourceUsage((FfxApiResourceUsage)resourceDesc.usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (resourceState == FFX_API_RESOURCE_STATE_COPY_SRC)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (resourceState == FFX_API_RESOURCE_STATE_COPY_DEST)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &backendResource->bufferResource) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)backendResource->bufferResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendResource->bufferResource, &memRequirements);

        // If a placement heap is provided, bind the resource to the existing VkDeviceMemory (DX12-style placed resource)
        if (createResourceDescription->heapInfo.usePlacementHeap && createResourceDescription->heapInfo.placementHeap)
        {
            uint64_t offset = createResourceDescription->heapInfo.placementHeapOffset;
            FFX_ASSERT_MESSAGE((offset % memRequirements.alignment) == 0, "heap offset is not compatible with required alignment");

            VkDeviceMemory heapMemory = reinterpret_cast<VkDeviceMemory>(createResourceDescription->heapInfo.placementHeap);
            if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, backendResource->bufferResource, heapMemory, offset) != VK_SUCCESS) 
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

        } 
        // allocate the memory
        else 
        {
            FfxErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
            if (FFX_OK != errorCode)
                return errorCode;

            if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, backendResource->bufferResource, backendResource->deviceMemory, 0) != VK_SUCCESS) 
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }
        }

        // if this is an upload buffer (currently only support upload buffers), copy the data and return
        if (createResourceDescription->heapInfo.heapType == FFX_HEAP_TYPE_UPLOAD && initData.size > 0)
        {
            // only allow copies directly into mapped memory for buffer resources since all texture resources are in optimal tiling
            void* data = NULL;

            if (backendContext->vkFunctionTable.vkMapMemory(backendContext->device, backendResource->deviceMemory, 0, initData.size, 0, &data) != VK_SUCCESS) {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            if (initData.type == FFX_RESOURCE_INIT_DATA_TYPE_BUFFER)
                memcpy(data, initData.buffer, initData.size);
            else if (initData.type == FFX_RESOURCE_INIT_DATA_TYPE_VALUE)
                memset(data, initData.value, initData.size);

            // flush mapped range if memory type is not coherant
            if ((backendResource->memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendResource->deviceMemory;
                memoryRange.size = initData.size;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }

            backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendResource->deviceMemory);
            return FFX_OK;
        }

        break;
    }
    case FFX_API_RESOURCE_TYPE_TEXTURE1D:
    case FFX_API_RESOURCE_TYPE_TEXTURE2D:
    case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_API_RESOURCE_TYPE_TEXTURE3D:
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = ffxGetVKImageTypeFromResourceType((FfxApiResourceType)createResourceDescription->resourceDescription.type);
        imageInfo.extent.width = createResourceDescription->resourceDescription.width;
        imageInfo.extent.height = createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE1D ? 1 : createResourceDescription->resourceDescription.height;
        imageInfo.extent.depth = ( createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE3D || createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE_CUBE ) ?
            createResourceDescription->resourceDescription.depth : 1;
        imageInfo.mipLevels = backendResource->resourceDescription.mipCount;
        imageInfo.arrayLayers = (createResourceDescription->resourceDescription.type ==
            FFX_API_RESOURCE_TYPE_TEXTURE1D || createResourceDescription->resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE2D)
            ? createResourceDescription->resourceDescription.depth : 1;
        imageInfo.format = getVkFormatFromSurfaceFormatAndUsage((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format, (FfxApiResourceUsage)resourceDesc.usage);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = getVKImageUsageFlagsFromResourceUsage((FfxApiResourceUsage)resourceDesc.usage);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (FFX_CONTAINS_FLAG(resourceDesc.usage, FFX_API_RESOURCE_USAGE_UAV) && ffxIsSurfaceFormatSRGB((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageInfo.format = ffxGetVkFormatFromSurfaceFormat(ffxGetSurfaceFormatFromGamma((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format));
        }

        if (backendContext->vkFunctionTable.vkCreateImage(backendContext->device, &imageInfo, nullptr, &backendResource->imageResource) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)backendResource->imageResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetImageMemoryRequirements(backendContext->device, backendResource->imageResource, &memRequirements);

        // If a placement heap is provided, bind the resource to the existing VkDeviceMemory (DX12-style placed resource)
        if (createResourceDescription->heapInfo.usePlacementHeap && createResourceDescription->heapInfo.placementHeap)
        {
            uint64_t offset = createResourceDescription->heapInfo.placementHeapOffset;
            FFX_ASSERT_MESSAGE((offset % memRequirements.alignment) == 0, "heap offset is not compatible with required alignment");

            VkDeviceMemory heapMemory = reinterpret_cast<VkDeviceMemory>(createResourceDescription->heapInfo.placementHeap);
            if (backendContext->vkFunctionTable.vkBindImageMemory(backendContext->device, backendResource->imageResource, heapMemory, offset) != VK_SUCCESS) 
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }
        }
        // allocate the memory
        else 
        {
            FfxErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
            if (FFX_OK != errorCode)
                return errorCode;

            if (backendContext->vkFunctionTable.vkBindImageMemory(backendContext->device, backendResource->imageResource, backendResource->deviceMemory, 0) != VK_SUCCESS) 
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }
        }

        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource type creation requested.");
        break;
    }

    // Create SRVs and UAVs
    switch (createResourceDescription->resourceDescription.type)
    {
    case FFX_API_RESOURCE_TYPE_BUFFER:
        break;
    case FFX_API_RESOURCE_TYPE_TEXTURE1D:
    case FFX_API_RESOURCE_TYPE_TEXTURE2D:
    case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_API_RESOURCE_TYPE_TEXTURE3D:
    {
        FFX_ASSERT_MESSAGE(effectContext.nextStaticResourceView + 1 < effectContext.nextDynamicResourceView[0],
            "FFXInterface: Vulkan: We've run out of resource views. Please increase the size.");
        backendResource->srvViewIndex = effectContext.nextStaticResourceView++;

        FfxApiResourceType type = (FfxApiResourceType)createResourceDescription->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_API_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_API_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_API_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = getVkFormatFromSurfaceFormatAndUsage((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format, (FfxApiResourceUsage)backendResource->resourceDescription.usage);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = getImageAspect((FfxApiResourceUsage)backendResource->resourceDescription.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = backendResource->resourceDescription.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
        addMutableViewForSRV(imageViewCreateInfo, imageViewUsageCreateInfo, backendResource->resourceDescription);

        // create an image view containing all mip levels for use as an srv
        if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->srvViewIndex].imageView) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->srvViewIndex].imageView, backendResource->resourceName);
#endif

        // create image views of individual mip levels for use as a uav
        if (FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_API_RESOURCE_USAGE_UAV))
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFX_ASSERT(effectContext.nextStaticResourceView + uavResourceViewCount < effectContext.nextDynamicResourceView[0]);

            backendResource->uavViewIndex = effectContext.nextStaticResourceView;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_API_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKUAVFormatFromSurfaceFormat((FfxApiSurfaceFormat)createResourceDescription->resourceDescription.format);

            for (uint32_t mip = 0; mip < backendResource->resourceDescription.mipCount; ++mip)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseMipLevel = mip;

                if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView) != VK_SUCCESS) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
#ifdef _DEBUG
                setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView, backendResource->resourceName);
#endif
            }

            effectContext.nextStaticResourceView += uavResourceViewCount;
        }
        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource view type creation requested.");
        break;
    }

    // create upload resource and upload job if needed
    if (initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED)
    {
        FfxResourceInternal copySrc;
        FfxCreateResourceDescription uploadDesc = { *createResourceDescription };
        uploadDesc.heapInfo.heapType        = FFX_HEAP_TYPE_UPLOAD;
        uploadDesc.resourceDescription.type = FFX_API_RESOURCE_TYPE_BUFFER;
        uploadDesc.resourceDescription.width = static_cast<uint32_t>(initData.size);
        uploadDesc.resourceDescription.usage = FFX_API_RESOURCE_USAGE_READ_ONLY;
        uploadDesc.initialState = FFX_API_RESOURCE_STATE_GENERIC_READ;
        uploadDesc.initData = createResourceDescription->initData;

        backendInterface->fpCreateResource(backendInterface, &uploadDesc, effectContextId, &copySrc);

        FfxGpuJobDescription* copyJob = nullptr;
        backendInterface->fpQueryNextGpuJobDesc(backendInterface, &copyJob);
        copyJob->jobType = FFX_GPU_JOB_COPY;
        wcscpy_s(copyJob->jobLabel, L"Resource Initialization Copy");

        // setup the upload job
        copyJob->copyJobDescriptor.src = copySrc;
        copyJob->copyJobDescriptor.dst = *outResource;
        copyJob->copyJobDescriptor.srcOffset = 0;
        copyJob->copyJobDescriptor.dstOffset = 0;
        copyJob->copyJobDescriptor.size      = 0;
    }

    backendResource->allocationSize = memRequirements.size;
    effectContext.vramUsage.totalUsageInBytes += static_cast<uint64_t>(backendResource->allocationSize);
    if ((createResourceDescription->resourceDescription.flags & FFX_API_RESOURCE_FLAGS_ALIASABLE) == FFX_API_RESOURCE_FLAGS_ALIASABLE)
    {
        effectContext.vramUsage.aliasableUsageInBytes += static_cast<uint64_t>(backendResource->allocationSize);
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId)
{
    FFX_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    if ((resource.internalIndex >= int32_t(effectContextId * FFX_MAX_RESOURCE_COUNT)) && (resource.internalIndex < int32_t(effectContext.nextStaticResource)))
    {
        BackendContext_VK::Resource& backgroundResource = backendContext->pResources[resource.internalIndex];

        if (backgroundResource.resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER)
        {
            // Destroy the resource
            if (backgroundResource.bufferResource != VK_NULL_HANDLE)
            {
                backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, backgroundResource.bufferResource, nullptr);
                backgroundResource.bufferResource = VK_NULL_HANDLE;
            }
        }
        else
        {
            // Destroy SRV
            if (backgroundResource.srvViewIndex >= 0)
            {
                backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView, nullptr);
                backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView = VK_NULL_HANDLE;
                backgroundResource.srvViewIndex                                           = 0;
            }

             // And UAVs
             if (FFX_CONTAINS_FLAG(backgroundResource.resourceDescription.usage, FFX_API_RESOURCE_USAGE_UAV))
             {
                 for (uint32_t i = 0; i < backgroundResource.uavViewCount; ++i)
                 {
                     if (backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView != VK_NULL_HANDLE)
                     {
                         backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView, nullptr);
                         backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView = VK_NULL_HANDLE;
                     }
                 }
             }

            // Reset indices to resource views
            backgroundResource.uavViewIndex = backgroundResource.srvViewIndex = -1;
            backgroundResource.uavViewCount = 0;

            // Destroy the resource
            if (backgroundResource.imageResource != VK_NULL_HANDLE)
            {
                backendContext->vkFunctionTable.vkDestroyImage(backendContext->device, backgroundResource.imageResource, nullptr);
                backgroundResource.imageResource = VK_NULL_HANDLE;
            }
        }

        if (backgroundResource.deviceMemory)
        {
            backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backgroundResource.deviceMemory, nullptr);
            backgroundResource.deviceMemory = VK_NULL_HANDLE;

            effectContext.vramUsage.totalUsageInBytes -= static_cast<uint64_t>(backgroundResource.allocationSize);
            if ((backendContext->pResources[resource.internalIndex].resourceDescription.flags & FFX_API_RESOURCE_FLAGS_ALIASABLE) == FFX_API_RESOURCE_FLAGS_ALIASABLE)
            {
                effectContext.vramUsage.aliasableUsageInBytes -= static_cast<uint64_t>(backgroundResource.allocationSize);
            }
            backgroundResource.allocationSize = 0;
        }
    }

    return FFX_OK;
}

static uint32_t FindAnyMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties& memProps,
    VkMemoryPropertyFlags requiredFlags,
    VkMemoryPropertyFlags preferredFlags = 0)
{
    uint32_t best = UINT32_MAX;

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) 
    {
        VkMemoryPropertyFlags flags = memProps.memoryTypes[i].propertyFlags;
        if ((flags & requiredFlags) == requiredFlags) 
        {
            // first acceptable
            if (best == UINT32_MAX) best = i;
            // prefer extra flags if available
            if ((flags & preferredFlags) == preferredFlags) return i;
        }
    }
    return best;
}

FfxErrorCode CreateHeapVK(FfxInterface* backendInterface, const FfxCreateHeapDescription* createHeapDescription, FfxUInt32 effectContextId, FfxResourceHeap* outHeap) 
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(backendContext->physicalDevice, &memProps);

    VkMemoryPropertyFlags requiredFlags = 0;
    VkMemoryPropertyFlags preferredFlags = 0;

    switch (createHeapDescription->heapType)
    {
    case FFX_HEAP_TYPE_DEFAULT:
        requiredFlags  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case FFX_HEAP_TYPE_UPLOAD:
        requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case FFX_HEAP_TYPE_READBACK:
        requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    default:
        requiredFlags  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    }

    const bool aliasable = (createHeapDescription->flags & FFX_HEAP_FLAGS_ALIASABLE) > 0;
    
    uint32_t memoryIndex = FindAnyMemoryTypeIndex(memProps, requiredFlags, preferredFlags);
    if (memoryIndex == UINT32_MAX)
    {
        return FFX_ERROR_BACKEND_API_ERROR;
    } 
    
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize  = createHeapDescription->size;
    alloc.memoryTypeIndex = memoryIndex;

    VkDeviceMemory heapMemory;
    if (vkAllocateMemory(backendContext->device, &alloc, nullptr, &heapMemory) != VK_SUCCESS)
    {
        return FFX_ERROR_BACKEND_API_ERROR;
    }   

    outHeap->heap = heapMemory;
	outHeap->size = createHeapDescription->size;
    outHeap->aliasable = aliasable;

    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    effectContext.vramUsage.totalUsageInBytes += outHeap->size;
    if (aliasable)
    {
        effectContext.vramUsage.aliasableUsageInBytes += outHeap->size;
    }

    return FFX_OK;
}

FfxErrorCode DestroyHeapVK(FfxInterface* backendInterface, FfxResourceHeap heap, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    VkDeviceMemory heapMemory = reinterpret_cast<VkDeviceMemory>(heap.heap);
	if (heapMemory)
	{
		backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, heapMemory, nullptr);
	}

    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    effectContext.vramUsage.totalUsageInBytes -= heap.size;
    if (heap.aliasable)
    {
        effectContext.vramUsage.aliasableUsageInBytes -= heap.size;
    }

    return FFX_OK;
}

FfxErrorCode MapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    BackendContext_VK::Resource* res = &backendContext->pResources[resource.internalIndex];

    if ((backendContext->vkFunctionTable.vkMapMemory(backendContext->device, res->deviceMemory, 0, res->resourceDescription.size, 0, ptr) != VK_SUCCESS))
        return FFX_ERROR_BACKEND_API_ERROR;

    return FFX_OK;
}

FfxErrorCode UnmapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    BackendContext_VK::Resource* res = &backendContext->pResources[resource.internalIndex];

    backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, res->deviceMemory);

    return FFX_OK;
}

FfxErrorCode RegisterResourceVK(
    FfxInterface* backendInterface,
    const FfxApiResource* inFfxResource,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outFfxResourceInternal
)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)(backendInterface->scratchBuffer);
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    if (inFfxResource->resource == nullptr) {

        outFfxResourceInternal->internalIndex = 0; // Always maps to FFX_<feature>_RESOURCE_IDENTIFIER_NULL;
        return FFX_OK;
    }

    // In vulkan we need to treat dynamic resources a little differently due to needing views to live as long as the GPU needs them.
    // We will treat them more like static resources and use the nextDynamicResource as a "hint" for where it should be.
    // Failure to find the pre-existing resource at the expected location will force a search until the resource is found.
    // If it is not found, a new entry will be created
    FFX_ASSERT(effectContext.nextDynamicResource > effectContext.nextStaticResource);
    outFfxResourceInternal->internalIndex = effectContext.nextDynamicResource--;

    //bool setupDynamicResource = false;
    BackendContext_VK::Resource* backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];

    // Start by seeing if this entry is empty, as that triggers an automatic setup of a new dynamic resource
    /*if (backendResource->uavViewIndex < 0 && backendResource->srvViewIndex < 0)
    {
        setupDynamicResource = true;
    }

    // If not a new resource, does it match what's current slotted for this dynamic resource
    if (!setupDynamicResource)
    {
        // If this is us, just return as everything is setup as needed
        if ((backendResource->resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxResource->resource) ||
            (backendResource->resourceDescription.type != FFX_API_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxResource->resource))
            return FFX_OK;

        // If this isn't us, search until we either find our entry or an empty resource
        outFfxResourceInternal->internalIndex = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
        while (!setupDynamicResource)
        {
            FFX_ASSERT(outFfxResourceInternal->internalIndex > effectContext.nextStaticResource); // Safety check while iterating
            backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];

            // Is this us?
            if ((backendResource->resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxResource->resource) ||
                (backendResource->resourceDescription.type != FFX_API_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxResource->resource))
            {
                 copyResourceState(backendResource, inFfxResource);
                 return FFX_OK;
            }

            // Empty?
            if (backendResource->uavViewIndex == -1 && backendResource->srvViewIndex == -1)
            {
                setupDynamicResource = true;
                break;
            }

            --outFfxResourceInternal->internalIndex;
        }
    }*/

    // If we got here, we are setting up a new dynamic entry
    backendResource->resourceDescription = inFfxResource->description;
    if (inFfxResource->description.type == FFX_API_RESOURCE_TYPE_BUFFER)
        backendResource->bufferResource = reinterpret_cast<VkBuffer>(inFfxResource->resource);
    else
        backendResource->imageResource = reinterpret_cast<VkImage>(inFfxResource->resource);

    copyResourceState(backendResource, inFfxResource);

    // the first call of RegisterResource can be identified because


    //////////////////////////////////////////////////////////////////////////
    // Create SRVs and UAVs
    switch (backendResource->resourceDescription.type)
    {
    case FFX_API_RESOURCE_TYPE_BUFFER:
        break;
    case FFX_API_RESOURCE_TYPE_TEXTURE1D:
    case FFX_API_RESOURCE_TYPE_TEXTURE2D:
    case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_API_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxApiResourceType type = (FfxApiResourceType)backendResource->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_API_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_API_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_API_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = getVkFormatFromSurfaceFormatAndUsage((FfxApiSurfaceFormat)backendResource->resourceDescription.format, (FfxApiResourceUsage)backendResource->resourceDescription.usage);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = getImageAspect((FfxApiResourceUsage)backendResource->resourceDescription.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = backendResource->resourceDescription.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // create an image view containing all mip levels for use as an srv
        FFX_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
        backendResource->srvViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex]--;

        VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
        addMutableViewForSRV(imageViewCreateInfo, imageViewUsageCreateInfo, backendResource->resourceDescription);

        if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->srvViewIndex].imageView) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->srvViewIndex].imageView, backendResource->resourceName);
#endif

        // create image views of individual mip levels for use as a uav
        if (FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_API_RESOURCE_USAGE_UAV))
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFX_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1 > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
            backendResource->uavViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_API_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKUAVFormatFromSurfaceFormat((FfxApiSurfaceFormat)backendResource->resourceDescription.format);
            imageViewCreateInfo.pNext  = nullptr;

            for (uint32_t mip = 0; mip < backendResource->resourceDescription.mipCount; ++mip)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseMipLevel = mip;

                if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView) != VK_SUCCESS) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
#ifdef _DEBUG
                setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView, backendResource->resourceName);
#endif
            }
            effectContext.nextDynamicResourceView[effectContext.frameIndex] -= uavResourceViewCount;
        }
        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource view type creation requested.");
        break;
    }

    return FFX_OK;
}

FfxApiResource GetResourceVK(FfxInterface* backendInterface, FfxResourceInternal inResource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxApiResourceDescription ffxResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxApiResource resource = {};
    resource.resource = reinterpret_cast<void*>(backendContext->pResources[inResource.internalIndex].imageResource);
    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so we will flag it as such to finish initializing it later
    // before it is used.
    if (backendContext->pResources[inResource.internalIndex].undefined) {
        ffxResDescription.flags = (FfxApiResourceFlags)((int)ffxResDescription.flags | FFX_API_RESOURCE_FLAGS_UNDEFINED);
        // Flag it as no longer being undefined as it will no longer be after workload
        // execution
        backendContext->pResources[inResource.internalIndex].undefined = false;
    }
    resource.state = backendContext->pResources[inResource.internalIndex].currentState;
    resource.description = ffxResDescription;

    return resource;
}

// dispose dynamic resources: This should be called at the end of the frame
FfxErrorCode UnregisterResourcesVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)(backendInterface->scratchBuffer);
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Walk back all the resources that don't belong to us and reset them to their initial state
    const uint32_t dynamicResourceIndexStart = getDynamicResourcesStartIndex(effectContextId);
    for (uint32_t resourceIndex = ++effectContext.nextDynamicResource; resourceIndex <= dynamicResourceIndexStart; ++resourceIndex)
    {
        FfxResourceInternal internalResource;
        internalResource.internalIndex = resourceIndex;

        BackendContext_VK::Resource* backendResource = &backendContext->pResources[resourceIndex];

        // Also clear out their srv/uav indices so they are regenerated each frame
        backendResource->uavViewIndex = -1;
        backendResource->srvViewIndex = -1;

        // Add the barrier
        addBarrier(backendContext, &internalResource, backendResource->initialState);
    }

    FFX_ASSERT(nullptr != commandList);
    VkCommandBuffer pCmdList = reinterpret_cast<VkCommandBuffer>(commandList);

    flushBarriers(backendContext, pCmdList);

    // Just reset the dynamic resource index, but leave the images views.
    // They will be deleted in the first pipeline destroy call as they need to live until then
    effectContext.nextDynamicResource = dynamicResourceIndexStart;

    // destroy the views of the next frame
    effectContext.frameIndex = (effectContext.frameIndex + 1) % FFX_MAX_QUEUED_FRAMES;
    destroyDynamicViews(backendContext, effectContextId, effectContext.frameIndex);

    return FFX_OK;
}

FfxErrorCode registerStaticTextureSrv(BackendContext_VK* backendContext, const FfxApiResource* inResource, uint32_t index, FfxUInt32 effectContextId)
{            
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessTextureSrvHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    VkImage vkImage = reinterpret_cast<VkImage>(inResource->resource);

    // Create SRVs and UAVs
    switch (inResource->description.type)
    {
    case FFX_API_RESOURCE_TYPE_BUFFER:
    {
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Buffer as a Texture SRV.");
        break;
    }
    case FFX_API_RESOURCE_TYPE_TEXTURE1D:
    case FFX_API_RESOURCE_TYPE_TEXTURE2D:
    case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_API_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxApiResourceType       type             = (FfxApiResourceType)inResource->description.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext                 = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(inResource->description.usage, FFX_API_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_API_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (inResource->description.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_API_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (inResource->description.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image                           = vkImage;
        imageViewCreateInfo.format                          = getVkFormatFromSurfaceFormatAndUsage((FfxApiSurfaceFormat)inResource->description.format, (FfxApiResourceUsage)inResource->description.usage);
        imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask     = getImageAspect((FfxApiResourceUsage)inResource->description.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = inResource->description.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        uint32_t imageViewIndex = effectContext.bindlessTextureSrvHeapStart + index;

        if (backendContext->pResourceViews[imageViewIndex].imageView)
            backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[imageViewIndex].imageView, nullptr);

        if (backendContext->vkFunctionTable.vkCreateImageView(
                backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[imageViewIndex].imageView) != VK_SUCCESS)
        {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        VkWriteDescriptorSet  writeDescriptorSet  = {};
        VkDescriptorImageInfo imageDescriptorInfo = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessTextureSrvDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSet.pImageInfo      = &imageDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescriptorInfo.imageView   = backendContext->pResourceViews[imageViewIndex].imageView;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;

        break;
    }
    default:
        break;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}            
             
FfxErrorCode registerStaticBufferSrv(BackendContext_VK* backendContext, const FfxApiResource* inResource, uint32_t offset, uint32_t size, uint32_t, uint32_t index, FfxUInt32 effectContextId)
{            
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessBufferSrvHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (inResource->description.type == FFX_API_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer buffer = reinterpret_cast<VkBuffer>(inResource->resource);

        VkWriteDescriptorSet   writeDescriptorSet   = {};
        VkDescriptorBufferInfo bufferDescriptorInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessBufferSrvDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSet.pBufferInfo     = &bufferDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        bufferDescriptorInfo.buffer = buffer;
        bufferDescriptorInfo.offset = offset;
        bufferDescriptorInfo.range  = size > 0 ? size : VK_WHOLE_SIZE;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;
    }
    else
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Texture as a Buffer SRV.");

    return FFX_ERROR_INVALID_ARGUMENT;
}            
             
FfxErrorCode registerStaticTextureUav(BackendContext_VK* backendContext, const FfxApiResource* inResource, uint32_t, uint32_t index, FfxUInt32 effectContextId)
{            
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessTextureUavHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    VkImage vkImage = reinterpret_cast<VkImage>(inResource->resource);

    // Create SRVs and UAVs
    switch (inResource->description.type)
    {
    case FFX_API_RESOURCE_TYPE_BUFFER:
    {
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Buffer as a Texture UAV.");
        break;
    }
    case FFX_API_RESOURCE_TYPE_TEXTURE1D:
    case FFX_API_RESOURCE_TYPE_TEXTURE2D:
    case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_API_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxApiResourceType       type             = (FfxApiResourceType)inResource->description.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext                 = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(inResource->description.usage, FFX_API_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_API_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (inResource->description.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_API_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (inResource->description.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_API_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image                           = vkImage;
        imageViewCreateInfo.format                          = getVkFormatFromSurfaceFormatAndUsage((FfxApiSurfaceFormat)inResource->description.format, (FfxApiResourceUsage)inResource->description.usage);
        imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask     = getImageAspect((FfxApiResourceUsage)inResource->description.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = inResource->description.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        uint32_t imageViewIndex = effectContext.bindlessTextureUavHeapStart + index;

        if (backendContext->pResourceViews[imageViewIndex].imageView)
            backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[imageViewIndex].imageView, nullptr);

        if (backendContext->vkFunctionTable.vkCreateImageView(
                backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[imageViewIndex].imageView) != VK_SUCCESS)
        {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        VkWriteDescriptorSet  writeDescriptorSet  = {};
        VkDescriptorImageInfo imageDescriptorInfo = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessTextureUavDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSet.pImageInfo      = &imageDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageDescriptorInfo.imageView   = backendContext->pResourceViews[imageViewIndex].imageView;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;

        break;
    }
    default:
        break;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}            
             
FfxErrorCode registerStaticBufferUav(BackendContext_VK* backendContext, const FfxApiResource* inResource, uint32_t offset, uint32_t size, uint32_t, uint32_t index, FfxUInt32 effectContextId)
{
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessBufferUavHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (inResource->description.type == FFX_API_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer buffer = reinterpret_cast<VkBuffer>(inResource->resource);

        VkWriteDescriptorSet   writeDescriptorSet   = {};
        VkDescriptorBufferInfo bufferDescriptorInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessBufferUavDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSet.pBufferInfo     = &bufferDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        bufferDescriptorInfo.buffer = buffer;
        bufferDescriptorInfo.offset = offset;
        bufferDescriptorInfo.range  = size > 0 ? size : VK_WHOLE_SIZE;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;
    }
    else
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Texture as a Buffer UAV.");

    return FFX_ERROR_INVALID_ARGUMENT;
}

FfxErrorCode RegisterStaticResourceVK(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != desc);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    switch (desc->descriptorType) {
    case FFX_DESCRIPTOR_TEXTURE_SRV:
        return registerStaticTextureSrv(backendContext, desc->resource, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_BUFFER_SRV:
        return registerStaticBufferSrv(
            backendContext, desc->resource, desc->bufferOffset, desc->bufferSize, desc->bufferStride, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_TEXTURE_UAV:
        return registerStaticTextureUav(backendContext, desc->resource, desc->textureUavMip, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_BUFFER_UAV:
        return registerStaticBufferUav(
            backendContext, desc->resource, desc->bufferOffset, desc->bufferSize, desc->bufferStride, desc->descriptorIndex, effectContextId);
    default:
        return FFX_ERROR_INVALID_ARGUMENT;
    }
}

FfxApiResourceDescription GetResourceDescriptionVK(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxApiResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

FfxErrorCode StageConstantBufferDataVK(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    if (data && constantBuffer)
    {
        if ((backendContext->stagingRingBufferBase + FFX_ALIGN_UP(size, 256)) >= FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE)
            backendContext->stagingRingBufferBase = 0;

        uint32_t* dstPtr = (uint32_t*)(backendContext->pStagingRingBuffer + backendContext->stagingRingBufferBase);

        memcpy(dstPtr, data, size);

        constantBuffer->data            = dstPtr;
        constantBuffer->num32BitEntries = size / sizeof(uint32_t);

        backendContext->stagingRingBufferBase += FFX_ALIGN_UP(size, 256);

        return FFX_OK;
    }
    else
        return FFX_ERROR_INVALID_POINTER;
}

VkSamplerAddressMode FfxGetAddressModeVK(const FfxAddressMode& addressMode)
{
    switch (addressMode)
    {
    case FFX_ADDRESS_MODE_WRAP:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case FFX_ADDRESS_MODE_MIRROR:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case FFX_ADDRESS_MODE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case FFX_ADDRESS_MODE_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case FFX_ADDRESS_MODE_MIRROR_ONCE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        FFX_ASSERT_MESSAGE(false, "Unsupported addressing mode requested. Please implement");
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    }
}

FfxErrorCode CreatePipelineVK(
    FfxInterface* backendInterface,
    FfxShaderBlob* pShaderBlob,
    const FfxPipelineDescription* pipelineDescription,
    FfxUInt32                     effectContextId,
    FfxPipelineState* outPipeline)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != pipelineDescription);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    //////////////////////////////////////////////////////////////////////////
    // One root signature (or pipeline layout) per pipeline
    FFX_ASSERT_MESSAGE(effectContext.nextPipelineLayout < (effectContextId * FFX_MAX_PASS_COUNT) + FFX_MAX_PASS_COUNT, "FFXInterface: Vulkan: Ran out of pipeline layouts. Please increase FFX_MAX_PASS_COUNT");
    BackendContext_VK::PipelineLayout* pPipelineLayout = &backendContext->pPipelineLayouts[effectContext.nextPipelineLayout++];

    // Start by creating samplers
    FFX_ASSERT(pipelineDescription->samplerCount <= FFX_MAX_SAMPLERS);
    const size_t samplerCount = pipelineDescription->samplerCount;
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.minLod = 0.f;
        createInfo.maxLod = VK_LOD_CLAMP_NONE;
        createInfo.anisotropyEnable = false; // TODO: Do a check for anisotropy once it's an available filter option
        createInfo.compareEnable = false;
        createInfo.compareOp = VK_COMPARE_OP_NEVER;
        createInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.addressModeU = FfxGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeU);
        createInfo.addressModeV = FfxGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeV);
        createInfo.addressModeW = FfxGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeW);

        // Set the right filter
        switch (pipelineDescription->samplers[currentSamplerIndex].filter)
        {
        case FFX_FILTER_TYPE_MINMAGMIP_POINT:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case FFX_FILTER_TYPE_MINMAGMIP_LINEAR:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case FFX_FILTER_TYPE_MINMAGLINEARMIP_POINT:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;

        default:
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported filter type requested. Please implement");
            break;
        }

        if (backendContext->vkFunctionTable.vkCreateSampler(backendContext->device, &createInfo, nullptr, &pPipelineLayout->samplers[currentSamplerIndex]) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    int32_t staticTextureSrvCount = 0;
    int32_t staticBufferSrvCount  = 0;
    int32_t staticTextureUavCount = 0;
    int32_t staticBufferUavCount  = 0;

    pPipelineLayout->staticTextureSrvSet = -1;
    pPipelineLayout->staticBufferSrvSet  = -1;
    pPipelineLayout->staticTextureUavSet = -1;
    pPipelineLayout->staticBufferUavSet  = -1;

    // Setup descriptor sets
    VkDescriptorSetLayoutBinding layoutBindings[MAX_DESCRIPTOR_SET_LAYOUTS];
    uint32_t numLayoutBindings = 0;

    // Support more when needed
    VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Samplers - just the static ones for now
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
        layoutBindings[numLayoutBindings++] = {
            currentSamplerIndex + SAMPLER_BINDING_SHIFT, VK_DESCRIPTOR_TYPE_SAMPLER, 1, shaderStageFlags, &pPipelineLayout->samplers[currentSamplerIndex]};

    // Texture SRVs
    for (uint32_t srvIndex = 0; srvIndex < pShaderBlob->srvTextureCount; ++srvIndex)
    {
        // count static srvs separately.
        if (pShaderBlob->boundSRVTextureSpaces[srvIndex] != 0)
        {
            if (staticTextureSrvCount > 0)
                FFX_ASSERT(pPipelineLayout->staticTextureSrvSet != static_cast<int32_t>(pShaderBlob->boundSRVTextureSpaces[srvIndex]));

            staticTextureSrvCount += pShaderBlob->boundSRVTextureCounts[srvIndex];
            pPipelineLayout->staticTextureSrvSet = pShaderBlob->boundSRVTextureSpaces[srvIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { pShaderBlob->boundSRVTextures[srvIndex], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            pShaderBlob->boundSRVTextureCounts[srvIndex], shaderStageFlags, nullptr };
    }

    // Buffer SRVs
    for (uint32_t srvIndex = 0; srvIndex < pShaderBlob->srvBufferCount; ++srvIndex)
    {
        // count static srvs separately.
        if (pShaderBlob->boundSRVBufferSpaces[srvIndex] != 0)
        {
            if (staticBufferSrvCount > 0)
                FFX_ASSERT(pPipelineLayout->staticBufferSrvSet != static_cast<int32_t>(pShaderBlob->boundSRVBufferSpaces[srvIndex]));

            staticBufferSrvCount += pShaderBlob->boundSRVBufferCounts[srvIndex];
            pPipelineLayout->staticBufferSrvSet = pShaderBlob->boundSRVBufferSpaces[srvIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { pShaderBlob->boundSRVBuffers[srvIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            pShaderBlob->boundSRVBufferCounts[srvIndex], shaderStageFlags, nullptr};
    }

    // Texture UAVs
    for (uint32_t uavIndex = 0; uavIndex < pShaderBlob->uavTextureCount; ++uavIndex)
    {
        // count static uavs separately.
        if (pShaderBlob->boundUAVTextureSpaces[uavIndex] != 0)
        {
            if (staticTextureUavCount > 0)
                FFX_ASSERT(pPipelineLayout->staticTextureUavSet != static_cast<int32_t>(pShaderBlob->boundUAVTextureSpaces[uavIndex]));

            staticTextureUavCount += pShaderBlob->boundUAVTextureCounts[uavIndex];
            pPipelineLayout->staticTextureUavSet = pShaderBlob->boundUAVTextureSpaces[uavIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { pShaderBlob->boundUAVTextures[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            pShaderBlob->boundUAVTextureCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Buffer UAVs
    for (uint32_t uavIndex = 0; uavIndex < pShaderBlob->uavBufferCount; ++uavIndex)
    {
        // count static uavs separately.
        if (pShaderBlob->boundUAVBufferSpaces[uavIndex] != 0)
        {
            if (staticBufferUavCount > 0)
                FFX_ASSERT(pPipelineLayout->staticBufferUavSet != static_cast<int32_t>(pShaderBlob->boundUAVBufferSpaces[uavIndex]));

            staticBufferUavCount += pShaderBlob->boundUAVBufferCounts[uavIndex];
            pPipelineLayout->staticBufferUavSet = pShaderBlob->boundUAVBufferSpaces[uavIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { pShaderBlob->boundUAVBuffers[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            pShaderBlob->boundUAVBufferCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Constant buffers (uniforms)
    for (uint32_t cbIndex = 0; cbIndex < pShaderBlob->cbvCount; ++cbIndex)
    {
        layoutBindings[numLayoutBindings++] = { pShaderBlob->boundConstantBuffers[cbIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            pShaderBlob->boundConstantBufferCounts[cbIndex], shaderStageFlags, nullptr };
    }

    // Create the descriptor layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = numLayoutBindings;
    layoutInfo.pBindings = layoutBindings;

    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(backendContext->device, &layoutInfo, nullptr, &pPipelineLayout->descriptorSetLayout) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // allocate descriptor sets
    pPipelineLayout->descriptorSetIndex = 0;
    for (uint32_t i = 0; i < (FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME); i++)
    {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = backendContext->descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &pPipelineLayout->descriptorSetLayout;

        if (backendContext->vkFunctionTable.vkAllocateDescriptorSets(backendContext->device, &allocateInfo, &pPipelineLayout->descriptorSets[i]) != VK_SUCCESS)
        {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    uint32_t setCount = 0;

    VkDescriptorSetLayout layouts[5];

    layouts[setCount++] = pPipelineLayout->descriptorSetLayout;

    if (staticTextureSrvCount > 0)
    {
        layouts[pPipelineLayout->staticTextureSrvSet] = effectContext.bindlessTextureSrvDescriptorSetLayout;
        setCount++;
    }

    if (staticBufferSrvCount > 0)
    {
        layouts[pPipelineLayout->staticBufferSrvSet] = effectContext.bindlessBufferSrvDescriptorSetLayout;
        setCount++;
    }

    if (staticTextureUavCount > 0)
    {
        layouts[pPipelineLayout->staticTextureUavSet] = effectContext.bindlessTextureUavDescriptorSetLayout;
        setCount++;
    }

    if (staticBufferUavCount > 0)
    {
        layouts[pPipelineLayout->staticBufferUavSet] = effectContext.bindlessBufferUavDescriptorSetLayout;
        setCount++;
    }

    // create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = setCount;
    pipelineLayoutInfo.pSetLayouts            = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    if (backendContext->vkFunctionTable.vkCreatePipelineLayout(backendContext->device, &pipelineLayoutInfo, nullptr, &pPipelineLayout->pipelineLayout) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // set the root signature to pipeline
    outPipeline->rootSignature = reinterpret_cast<FfxRootSignature>(pPipelineLayout);

    // Only set the command signature if this is setup as an indirect workload
    if (pipelineDescription->indirectWorkload)
    {
        // Only need the stride ahead of time in Vulkan
        outPipeline->cmdSignature = reinterpret_cast<FfxCommandSignature>(sizeof(VkDispatchIndirectCommand));
    }
    else
    {
        outPipeline->cmdSignature = nullptr;
    }

    uint32_t flattenedSrvTextureCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < pShaderBlob->srvTextureCount; ++srvIndex)
    {
        uint32_t slotIndex = pShaderBlob->boundSRVTextures[srvIndex];
        uint32_t bindCount = pShaderBlob->boundSRVTextureCounts[srvIndex];

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvTextureCount++;

            outPipeline->srvTextureBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->srvTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(pShaderBlob->boundSRVTextureNames[srvIndex], outPipeline->srvTextureBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->srvTextureCount = flattenedSrvTextureCount;
    FFX_ASSERT(outPipeline->srvTextureCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavTextureCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < pShaderBlob->uavTextureCount; ++uavIndex)
    {
        uint32_t slotIndex = pShaderBlob->boundUAVTextures[uavIndex];
        uint32_t bindCount = pShaderBlob->boundUAVTextureCounts[uavIndex];

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavTextureCount++;

            outPipeline->uavTextureBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->uavTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(pShaderBlob->boundUAVTextureNames[uavIndex], outPipeline->uavTextureBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->uavTextureCount = flattenedUavTextureCount;
    FFX_ASSERT(outPipeline->uavTextureCount < FFX_MAX_NUM_UAVS);

    uint32_t flattenedSrvBufferCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < pShaderBlob->srvBufferCount; ++srvIndex)
    {
        uint32_t slotIndex  = pShaderBlob->boundSRVBuffers[srvIndex];
        uint32_t spaceIndex = pShaderBlob->boundSRVBufferSpaces[srvIndex];
        uint32_t bindCount  = pShaderBlob->boundSRVBufferCounts[srvIndex];

        // Skip static resources
        if (spaceIndex == 1)
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvBufferCount++;

            outPipeline->srvBufferBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->srvBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(pShaderBlob->boundSRVBufferNames[srvIndex], outPipeline->srvBufferBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->srvBufferCount = flattenedSrvBufferCount;
    FFX_ASSERT(outPipeline->srvBufferCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavBufferCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < pShaderBlob->uavBufferCount; ++uavIndex)
    {
        uint32_t slotIndex = pShaderBlob->boundUAVBuffers[uavIndex];
        uint32_t bindCount = pShaderBlob->boundUAVBufferCounts[uavIndex];

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavBufferCount++;

            outPipeline->uavBufferBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->uavBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(pShaderBlob->boundUAVBufferNames[uavIndex], outPipeline->uavBufferBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->uavBufferCount = flattenedUavBufferCount;
    FFX_ASSERT(outPipeline->uavBufferCount < FFX_MAX_NUM_UAVS);

    for (uint32_t cbIndex = 0; cbIndex < pShaderBlob->cbvCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex  = pShaderBlob->boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].arrayIndex = 1;
        ConvertUTF8ToUTF16(pShaderBlob->boundConstantBufferNames[cbIndex], outPipeline->constantBufferBindings[cbIndex].name, FFX_RESOURCE_NAME_SIZE);
    }

    outPipeline->constCount = pShaderBlob->cbvCount;
    FFX_ASSERT(outPipeline->constCount < FFX_MAX_NUM_CONST_BUFFERS);

    outPipeline->staticTextureSrvCount = staticTextureSrvCount;
    FFX_ASSERT(outPipeline->staticTextureSrvCount <= effectContext.bindlessTextureSrvHeapSize);

    outPipeline->staticBufferSrvCount = staticBufferSrvCount;
    FFX_ASSERT(outPipeline->staticBufferSrvCount <= effectContext.bindlessBufferSrvHeapSize);

    outPipeline->staticTextureUavCount = staticTextureUavCount;
    FFX_ASSERT(outPipeline->staticTextureUavCount <= effectContext.bindlessTextureUavHeapSize);

    outPipeline->staticBufferUavCount = staticBufferUavCount;
    FFX_ASSERT(outPipeline->staticBufferUavCount <= effectContext.bindlessBufferUavHeapSize);

    // Todo when needed
    //outPipeline->samplerCount      = pShaderBlob->samplerCount;
    //outPipeline->rtAccelStructCount= pShaderBlob->rtAccelStructCount;

    //////////////////////////////////////////////////////////////////////////
    // pipeline creation
    FfxDeviceCapabilities capabilities;
    backendInterface->fpGetDeviceCapabilities(backendInterface, &capabilities);

    // shader module
    //
    // Embedded blobs may be deflate-compressed (see ffx_blob_compression.h). Decompress
    // into a local buffer when needed; uncompressed blobs pass through untouched. The
    // buffer lives until after vkCreateShaderModule below, which copies the bytecode.
    std::vector<uint8_t> shaderCodeScratch;
    const void*          shaderCodeData = nullptr;
    uint32_t             shaderCodeSize = 0;
    ffxDecompressShaderBlob(pShaderBlob->data, pShaderBlob->size, shaderCodeScratch, &shaderCodeData, &shaderCodeSize);

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (const uint32_t*)shaderCodeData;
    shaderModuleCreateInfo.codeSize = shaderCodeSize;

    if (backendContext->vkFunctionTable.vkCreateShaderModule(backendContext->device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // fill out shader stage create info
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT reqSubgroupSize = {};
    reqSubgroupSize.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT;
    reqSubgroupSize.requiredSubgroupSize = 32;

    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.pName  = pShaderBlob->entryName;
    shaderStageCreateInfo.module = shaderModule;

    if (pipelineDescription->forceWave32)
    {
        shaderStageCreateInfo.pNext = &reqSubgroupSize;
    }

    // create the compute pipeline
    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = pPipelineLayout->pipelineLayout;

    VkPipeline computePipeline = VK_NULL_HANDLE;
    if (backendContext->vkFunctionTable.vkCreateComputePipelines(backendContext->device, nullptr, 1, &pipelineCreateInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // done with shader module, so clean up
    backendContext->vkFunctionTable.vkDestroyShaderModule(backendContext->device, shaderModule, nullptr);

    // set the pipeline
    outPipeline->pipeline = reinterpret_cast<FfxPipeline>(computePipeline);

    // Setup the pipeline name
    wcscpy_s(outPipeline->name, pipelineDescription->name);

    return FFX_OK;
}

FfxErrorCode DestroyPipelineVK(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32)
{
    FFX_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    if (!pipeline)
        return FFX_OK;

    // Destroy the pipeline
    VkPipeline vkPipeline = reinterpret_cast<VkPipeline>(pipeline->pipeline);
    if (vkPipeline != VK_NULL_HANDLE) {
        backendContext->vkFunctionTable.vkDestroyPipeline(backendContext->device, vkPipeline, VK_NULL_HANDLE);
        pipeline->pipeline = VK_NULL_HANDLE;
    }

    // Zero out the cmd signature
    pipeline->cmdSignature = nullptr;

    // Destroy the pipeline layout
    BackendContext_VK::PipelineLayout* pPipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(pipeline->rootSignature);

    if (pPipelineLayout)
    {
        // Descriptor set layout
        if (pPipelineLayout->pipelineLayout != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyPipelineLayout(backendContext->device, pPipelineLayout->pipelineLayout, VK_NULL_HANDLE);
            pPipelineLayout->pipelineLayout = VK_NULL_HANDLE;
        }

        // Descriptor sets
        for (uint32_t i = 0; i < FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME; i++) {
            backendContext->vkFunctionTable.vkFreeDescriptorSets(backendContext->device, backendContext->descriptorPool, 1, &pPipelineLayout->descriptorSets[i]);
            pPipelineLayout->descriptorSets[i] = VK_NULL_HANDLE;
        }

        // Descriptor set layout
        if (pPipelineLayout->descriptorSetLayout != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(backendContext->device, pPipelineLayout->descriptorSetLayout, VK_NULL_HANDLE);
            pPipelineLayout->descriptorSetLayout = VK_NULL_HANDLE;
        }

        // Samplers
        for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < FFX_MAX_SAMPLERS; ++currentSamplerIndex)
        {
            if (pPipelineLayout->samplers[currentSamplerIndex] != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroySampler(backendContext->device, pPipelineLayout->samplers[currentSamplerIndex], VK_NULL_HANDLE);
                pPipelineLayout->samplers[currentSamplerIndex] = VK_NULL_HANDLE;
            }
        }
    }

    return FFX_OK;
}

FfxErrorCode ScheduleGpuJobVK(FfxInterface* backendInterface, const FfxGpuJobDescription* job)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != job);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFX_ASSERT(backendContext->gpuJobCount < FFX_MAX_GPU_JOBS);

    backendContext->pGpuJobs[backendContext->gpuJobCount] = *job;
    backendContext->gpuJobCount++;

    return FFX_OK;
}

FfxErrorCode QueryNextGpuJobDescVK(FfxInterface* backendInterface, FfxGpuJobDescription** outJobPtr)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_VK* backendContext = (BackendContext_VK*) backendInterface->scratchBuffer;

    FFX_ASSERT(backendContext->gpuJobCount < FFX_MAX_GPU_JOBS);

    *outJobPtr = &(backendContext->pGpuJobs[backendContext->gpuJobCount]);
    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_VK*    backendContext,
                                         FfxGpuJobDescription* job,
                                         VkCommandBuffer       vkCommandBuffer,
                                         FfxUInt32             effectContextId)
{
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->computeJobDescriptor.pipeline->rootSignature);

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    uint32_t               descriptorWriteIndex = 0;
    VkWriteDescriptorSet   writeDescriptorSets[FFX_MAX_RESOURCE_COUNT];

    // These MUST be initialized
    uint32_t               imageDescriptorIndex = 0;
    VkDescriptorImageInfo  imageDescriptorInfos[FFX_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFX_MAX_RESOURCE_COUNT; ++i)
        imageDescriptorInfos[i] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    // These MUST be initialized
    uint32_t               bufferDescriptorIndex = 0;
    VkDescriptorBufferInfo bufferDescriptorInfos[FFX_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFX_MAX_RESOURCE_COUNT; ++i)
        bufferDescriptorInfos[i] = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };

    // bind texture UAVs
    for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline->uavTextureCount; ++currentPipelineUavIndex)
    {
        FfxTextureUAV& textureUAV = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &textureUAV.resource, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

        const FfxResourceBinding& binding = job->computeJobDescriptor.pipeline->uavTextureBindings[currentPipelineUavIndex];

        // source: UAV of resource to bind
        const uint32_t resourceIndex = textureUAV.resource.internalIndex;
        uint32_t mipOffset = textureUAV.mip;
        if (textureUAV.mip >= backendContext->pResources[resourceIndex].resourceDescription.mipCount)
            mipOffset = backendContext->pResources[resourceIndex].resourceDescription.mipCount - 1;
        const uint32_t uavViewIndex  = backendContext->pResources[resourceIndex].uavViewIndex + mipOffset;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        imageDescriptorInfos[imageDescriptorIndex]             = {};
        imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[uavViewIndex].imageView;

        imageDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // bind buffer UAVs
    for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline->uavBufferCount; ++currentPipelineUavIndex) 
    {
        FfxBufferUAV& bufferUAV = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &bufferUAV.resource, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

        const FfxResourceBinding& binding = job->computeJobDescriptor.pipeline->uavBufferBindings[currentPipelineUavIndex];

        // source: UAV of buffer to bind
        const uint32_t resourceIndex = bufferUAV.resource.internalIndex;

        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        bufferDescriptorInfos[bufferDescriptorIndex] = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = bufferUAV.offset;
        bufferDescriptorInfos[bufferDescriptorIndex].range  = bufferUAV.size > 0 ? bufferUAV.size : VK_WHOLE_SIZE;

        bufferDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // bind texture SRVs
    for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline->srvTextureCount; ++currentPipelineSrvIndex)
    {
        FfxTextureSRV& textureSRV = job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &textureSRV.resource, FFX_API_RESOURCE_STATE_COMPUTE_READ);

        const FfxResourceBinding& binding = job->computeJobDescriptor.pipeline->srvTextureBindings[currentPipelineSrvIndex];

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        const uint32_t resourceIndex = textureSRV.resource.internalIndex;
        const uint32_t srvViewIndex  = backendContext->pResources[resourceIndex].srvViewIndex;

        imageDescriptorInfos[imageDescriptorIndex]             = {};
        imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[srvViewIndex].imageView;

        imageDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // bind buffer SRVs
    for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline->srvBufferCount; ++currentPipelineSrvIndex)
    {
        FfxBufferSRV& bufferSRV = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &bufferSRV.resource, FFX_API_RESOURCE_STATE_COMPUTE_READ);

        const FfxResourceBinding& binding = job->computeJobDescriptor.pipeline->srvBufferBindings[currentPipelineSrvIndex];

        // source: SRV of buffer to bind
        const uint32_t resourceIndex = bufferSRV.resource.internalIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo     = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        bufferDescriptorInfos[bufferDescriptorIndex]        = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = bufferSRV.offset;
        bufferDescriptorInfos[bufferDescriptorIndex].range  = bufferSRV.size > 0 ? bufferSRV.size : VK_WHOLE_SIZE;
    
        bufferDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // update uniform buffers
    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline->constCount; ++currentRootConstantIndex)
    {
        uint32_t dataSize = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t);
        
        // If we have a constant buffer allocator, use that, otherwise use the default backend allocator
        FfxApiConstantBufferAllocation allocation;
        if (s_fpConstantAllocator)
            allocation = s_fpConstantAllocator(job->computeJobDescriptor.cbs[currentRootConstantIndex].data, dataSize);
        else
            allocation = backendContext->FallbackConstantAllocator(job->computeJobDescriptor.cbs[currentRootConstantIndex].data, dataSize);
            
        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo     = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding =
            job->computeJobDescriptor.pipeline->constantBufferBindings[currentRootConstantIndex].slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        bufferDescriptorInfos[bufferDescriptorIndex].buffer = static_cast<VkBuffer>(allocation.resource.resource);
        bufferDescriptorInfos[bufferDescriptorIndex].offset = static_cast<VkDeviceSize>(allocation.handle);
        bufferDescriptorInfos[bufferDescriptorIndex].range  = dataSize;

        bufferDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // If we are dispatching indirectly, transition the argument resource to indirect argument
    if (job->computeJobDescriptor.pipeline->cmdSignature)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.cmdArgument, FFX_API_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    // insert all the barriers
    flushBarriers(backendContext, vkCommandBuffer);

    // update all uavs and srvs
    backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, descriptorWriteIndex, writeDescriptorSets, 0, nullptr);

    // bind pipeline
    backendContext->vkFunctionTable.vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, reinterpret_cast<VkPipeline>(job->computeJobDescriptor.pipeline->pipeline));

    // bind descriptor sets
    {
        backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, 0, 1, &pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex], 0, nullptr);

        BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

        if (job->computeJobDescriptor.pipeline->staticTextureSrvCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticTextureSrvSet, 1, &effectContext.bindlessTextureSrvDescriptorSet, 0, nullptr);

        if (job->computeJobDescriptor.pipeline->staticBufferSrvCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticBufferSrvSet, 1, &effectContext.bindlessBufferSrvDescriptorSet, 0, nullptr);

        if (job->computeJobDescriptor.pipeline->staticTextureUavCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticTextureUavSet, 1, &effectContext.bindlessTextureUavDescriptorSet, 0, nullptr);

        if (job->computeJobDescriptor.pipeline->staticBufferUavCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticBufferUavSet, 1, &effectContext.bindlessBufferUavDescriptorSet, 0, nullptr);
    }
    
    // Dispatch (or dispatch indirect)
    if (job->computeJobDescriptor.pipeline->cmdSignature)
    {
        const uint32_t resourceIndex = job->computeJobDescriptor.cmdArgument.internalIndex;
        VkBuffer buffer = backendContext->pResources[resourceIndex].bufferResource;
        backendContext->vkFunctionTable.vkCmdDispatchIndirect(vkCommandBuffer, buffer, job->computeJobDescriptor.cmdArgumentOffset);
    }
    else
    {
        backendContext->vkFunctionTable.vkCmdDispatch(vkCommandBuffer, job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
    }

    // move to another descriptor set for the next compute render job so that we don't overwrite descriptors in-use
    ++pipelineLayout->descriptorSetIndex;
    if (pipelineLayout->descriptorSetIndex >= (FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME))
        pipelineLayout->descriptorSetIndex = 0;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::Resource ffxResourceSrc = backendContext->pResources[job->copyJobDescriptor.src.internalIndex];
    BackendContext_VK::Resource ffxResourceDst = backendContext->pResources[job->copyJobDescriptor.dst.internalIndex];

    addBarrier(backendContext, &job->copyJobDescriptor.src, FFX_API_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFX_API_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, vkCommandBuffer);

    if (ffxResourceSrc.resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER && ffxResourceDst.resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxResourceSrc.bufferResource;
        VkBuffer vkResourceDst = ffxResourceDst.bufferResource;

        VkBufferCopy bufferCopy = {};

        bufferCopy.dstOffset = job->copyJobDescriptor.dstOffset;
        bufferCopy.srcOffset = job->copyJobDescriptor.srcOffset;
        bufferCopy.size      = job->copyJobDescriptor.size > 0 ? job->copyJobDescriptor.size : ffxResourceSrc.resourceDescription.width;

        backendContext->vkFunctionTable.vkCmdCopyBuffer(vkCommandBuffer, vkResourceSrc, vkResourceDst, 1, &bufferCopy);
    }
    else if (ffxResourceSrc.resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER && ffxResourceDst.resourceDescription.type != FFX_API_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxResourceSrc.bufferResource;
        VkImage vkResourceDst = ffxResourceDst.imageResource;

        VkImageSubresourceLayers subresourceLayers = {};

        subresourceLayers.aspectMask = getImageAspect((FfxApiResourceUsage)ffxResourceDst.resourceDescription.usage);
        subresourceLayers.baseArrayLayer = 0;
        subresourceLayers.layerCount = 1;
        subresourceLayers.mipLevel = 0;

        VkOffset3D offset = {};

        offset.x = 0;
        offset.y = 0;
        offset.z = 0;

        VkExtent3D extent = {};

        extent.width = ffxResourceDst.resourceDescription.width;
        extent.height = ffxResourceDst.resourceDescription.height;
        extent.depth = ffxResourceDst.resourceDescription.depth;

        // TODO: account for source buffer offset
        VkBufferImageCopy bufferImageCopy = {};

        bufferImageCopy.bufferOffset = 0;
        bufferImageCopy.bufferRowLength = 0;
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.imageSubresource = subresourceLayers;
        bufferImageCopy.imageOffset = offset;
        bufferImageCopy.imageExtent = extent;

        backendContext->vkFunctionTable.vkCmdCopyBufferToImage(vkCommandBuffer, vkResourceSrc, vkResourceDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
    }
    else
    {
        bool isSrcDepth = FFX_CONTAINS_FLAG(ffxResourceSrc.resourceDescription.usage, FFX_API_RESOURCE_USAGE_DEPTHTARGET);
        bool isDstDepth = FFX_CONTAINS_FLAG(ffxResourceDst.resourceDescription.usage, FFX_API_RESOURCE_USAGE_DEPTHTARGET);
        FFX_ASSERT_MESSAGE(isSrcDepth == isDstDepth, "Copy operations aren't allowed between depth and color textures in the vulkan backend of the FFX SDK.");

        #define FFX_MAX_IMAGE_COPY_MIPS 14 // Will handle 4k down to 1x1
        VkImageCopy             imageCopies[FFX_MAX_IMAGE_COPY_MIPS];
        VkImage vkResourceSrc = ffxResourceSrc.imageResource;
        VkImage vkResourceDst = ffxResourceDst.imageResource;

        uint32_t numMipsToCopy = FFX_MINIMUM(ffxResourceSrc.resourceDescription.mipCount, ffxResourceDst.resourceDescription.mipCount);

        for (uint32_t mip = 0; mip < numMipsToCopy; mip++)
        {
            VkImageSubresourceLayers srcSubresourceLayers = {};

            srcSubresourceLayers.aspectMask     = getImageAspect((FfxApiResourceUsage)ffxResourceSrc.resourceDescription.usage);
            srcSubresourceLayers.baseArrayLayer = 0;
            srcSubresourceLayers.layerCount     = 1;
            srcSubresourceLayers.mipLevel       = mip;

            VkImageSubresourceLayers dstSubresourceLayers = {};

            dstSubresourceLayers.aspectMask     = getImageAspect((FfxApiResourceUsage)ffxResourceDst.resourceDescription.usage);
            dstSubresourceLayers.baseArrayLayer = 0;
            dstSubresourceLayers.layerCount     = 1;
            dstSubresourceLayers.mipLevel       = mip;

            VkOffset3D offset = {};

            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};

            extent.width = ffxResourceSrc.resourceDescription.width / (mip + 1);
            extent.height = ffxResourceSrc.resourceDescription.height / (mip + 1);
            extent.depth = ffxResourceSrc.resourceDescription.depth / (mip + 1);

            VkImageCopy& copyRegion = imageCopies[mip];

            copyRegion.srcSubresource = srcSubresourceLayers;
            copyRegion.srcOffset = offset;
            copyRegion.dstSubresource = dstSubresourceLayers;
            copyRegion.dstOffset = offset;
            copyRegion.extent = extent;
        }

        backendContext->vkFunctionTable.vkCmdCopyImage(vkCommandBuffer, vkResourceSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkResourceDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numMipsToCopy, imageCopies);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobBarrier(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    addBarrier(backendContext, &job->barrierDescriptor.resource, job->barrierDescriptor.newState);
    flushBarriers(backendContext, vkCommandBuffer);

    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_VK::Resource ffxResource = backendContext->pResources[idx];

    if (ffxResource.resourceDescription.type == FFX_API_RESOURCE_TYPE_BUFFER)
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_API_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkBuffer vkResource = ffxResource.bufferResource;

        backendContext->vkFunctionTable.vkCmdFillBuffer(vkCommandBuffer, vkResource, 0, VK_WHOLE_SIZE, (uint32_t)job->clearJobDescriptor.color[0]);
    }
    else
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_API_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkImage vkResource = ffxResource.imageResource;

        VkClearColorValue clearColorValue = {};

        clearColorValue.float32[0] = job->clearJobDescriptor.color[0];
        clearColorValue.float32[1] = job->clearJobDescriptor.color[1];
        clearColorValue.float32[2] = job->clearJobDescriptor.color[2];
        clearColorValue.float32[3] = job->clearJobDescriptor.color[3];

        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = ffxResource.resourceDescription.mipCount;
        range.baseArrayLayer = 0;
        if (ffxResource.resourceDescription.type == FFX_API_RESOURCE_TYPE_TEXTURE3D)
            range.layerCount = 1;
        else
            range.layerCount = ffxResource.resourceDescription.depth;  // in that case depth is the number of layers
        
        backendContext->vkFunctionTable.vkCmdClearColorImage(vkCommandBuffer, vkResource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &range);
    }

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFX_ASSERT(nullptr != commandList);
    VkCommandBuffer vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList);

    FfxErrorCode errorCode = FFX_OK;

    // execute all renderjobs
    for (uint32_t i = 0; i < backendContext->gpuJobCount; ++i)
    {
        FfxGpuJobDescription* gpuJob = &backendContext->pGpuJobs[i];

        // If we have a label for the job, drop a marker for it
        if (gpuJob->jobLabel[0]) {
            beginMarkerVK(backendContext, vkCommandBuffer, gpuJob->jobLabel);
        }

        
        switch (gpuJob->jobType)
        {
        case FFX_GPU_JOB_CLEAR_FLOAT:
        {
            errorCode = executeGpuJobClearFloat(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFX_GPU_JOB_COPY:
        {
            errorCode = executeGpuJobCopy(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFX_GPU_JOB_COMPUTE:
        {
            errorCode = executeGpuJobCompute(backendContext, gpuJob, vkCommandBuffer, effectContextId);
            break;
        }
        case FFX_GPU_JOB_BARRIER:
        {
            errorCode = executeGpuJobBarrier(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        default:;
        }

        if (gpuJob->jobLabel[0]) {
            endMarkerVK(backendContext, vkCommandBuffer);
        }
    }

    // check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(
        errorCode == FFX_OK,
        FFX_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFX_OK;
}

void ffxRegisterConstantBufferAllocatorVK(FfxApiConstantBufferAllocator fpConstantAllocator)
{
    s_fpConstantAllocator = fpConstantAllocator;
}

void ffxRegisterResourceAllocatorVK(PfnFfxResourceAllocatorFunc fpResourceAllocator)
{
    s_fpResourceAllocator = fpResourceAllocator;
}

void ffxRegisterResourceDeallocatorVK(PfnFfxResourceDeallocatorFunc fpResourceDeallocator)
{
    s_fpResourceDeallocator = fpResourceDeallocator;
}


FfxCommandQueue ffxGetCommandQueueVK(VkQueue commandQueue)
{
    FFX_ASSERT(commandQueue != VK_NULL_HANDLE);
    return reinterpret_cast<FfxCommandQueue>(commandQueue);
}

FfxSwapchain ffxGetSwapchainVK(VkSwapchainKHR swapchain)
{
    FFX_ASSERT(swapchain != VK_NULL_HANDLE);
    return reinterpret_cast<FfxSwapchain>(swapchain);
}

VkSwapchainKHR ffxGetVKSwapchain(FfxSwapchain ffxSwapchain)
{
    return reinterpret_cast<VkSwapchainKHR>(ffxSwapchain);
}
