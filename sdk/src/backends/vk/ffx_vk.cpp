// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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


#include <FidelityFX/host/ffx_interface.h>
#include <FidelityFX/host/ffx_util.h>
#include <FidelityFX/host/ffx_assert.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#include <ffx_shader_blobs.h>
#include <codecvt>

// prototypes for functions in the interface
FfxUInt32              GetSDKVersionVK(FfxInterface* backendInterface);
FfxErrorCode           CreateBackendContextVK(FfxInterface* backendInterface, FfxUInt32* effectContextId);
FfxErrorCode           GetDeviceCapabilitiesVK(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode           DestroyBackendContextVK(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode           CreateResourceVK(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode           DestroyResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId);
FfxErrorCode           RegisterResourceVK(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxResource            GetResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           UnregisterResourcesVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxResourceDescription GetResourceDescriptionVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           CreatePipelineVK(FfxInterface* backendInterface, FfxEffect effect, FfxPass passId, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);
FfxErrorCode           DestroyPipelineVK(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode           ScheduleGpuJobVK(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode           ExecuteGpuJobsVK(FfxInterface* backendInterface, FfxCommandList commandList);

static VkDeviceContext sVkDeviceContext = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

#define MAX_DESCRIPTOR_SET_LAYOUTS      (32)

// Redefine offsets for compilation purposes
#define BINDING_SHIFT(name, shift)                       \
constexpr uint32_t name##_BINDING_SHIFT     = shift; \
constexpr wchar_t* name##_BINDING_SHIFT_STR = L#shift;

// put it there for now
BINDING_SHIFT(TEXTURE, 0);
BINDING_SHIFT(SAMPLER, 1000);
BINDING_SHIFT(UNORDERED_ACCESS_VIEW, 2000);
BINDING_SHIFT(CONSTANT_BUFFER, 3000);

// To track parallel effect context usage
static uint32_t s_BackendRefCount = 0;
static uint32_t s_MaxEffectContexts = 0;

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

        FfxResourceDescription  resourceDescription;
        FfxResourceStates       initialState;
        FfxResourceStates       currentState;
        int32_t                 srvViewIndex;
        int32_t                 uavViewIndex;
        uint32_t                uavViewCount;

        VkDeviceMemory          deviceMemory;
        VkMemoryPropertyFlags   memoryProperties;

        bool                    undefined;
        bool                    dynamic;

    } Resource;

    typedef struct UniformBuffer
    {
        VkBuffer bufferResource;
        uint8_t* pData;
    } UniformBuffer;

    typedef struct PipelineLayout {

        VkSampler               samplers[FFX_MAX_SAMPLERS];
        VkDescriptorSetLayout   descriptorSetLayout;
        VkDescriptorSet         descriptorSets[FFX_MAX_QUEUED_FRAMES];
        uint32_t                descriptorSetIndex;
        VkPipelineLayout        pipelineLayout;

    } PipelineLayout;

    typedef struct VKFunctionTable
    {
        PFN_vkGetDeviceProcAddr             vkGetDeviceProcAddr = 0;
        PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT = 0;
        PFN_vkCreateDescriptorPool          vkCreateDescriptorPool = 0;
        PFN_vkCreateSampler                 vkCreateSampler = 0;
        PFN_vkCreateDescriptorSetLayout     vkCreateDescriptorSetLayout = 0;
        PFN_vkCreateBuffer                  vkCreateBuffer = 0;
        PFN_vkCreateImage                   vkCreateImage = 0;
        PFN_vkCreateImageView               vkCreateImageView = 0;
        PFN_vkCreateShaderModule            vkCreateShaderModule = 0;
        PFN_vkCreatePipelineLayout          vkCreatePipelineLayout = 0;
        PFN_vkCreateComputePipelines        vkCreateComputePipelines = 0;
        PFN_vkDestroyPipelineLayout         vkDestroyPipelineLayout = 0;
        PFN_vkDestroyPipeline               vkDestroyPipeline = 0;
        PFN_vkDestroyImage                  vkDestroyImage = 0;
        PFN_vkDestroyImageView              vkDestroyImageView = 0;
        PFN_vkDestroyBuffer                 vkDestroyBuffer = 0;
        PFN_vkDestroyDescriptorSetLayout    vkDestroyDescriptorSetLayout = 0;
        PFN_vkDestroyDescriptorPool         vkDestroyDescriptorPool = 0;
        PFN_vkDestroySampler                vkDestroySampler = 0;
        PFN_vkDestroyShaderModule           vkDestroyShaderModule = 0;
        PFN_vkGetBufferMemoryRequirements   vkGetBufferMemoryRequirements = 0;
        PFN_vkGetImageMemoryRequirements    vkGetImageMemoryRequirements = 0;
        PFN_vkAllocateDescriptorSets        vkAllocateDescriptorSets = 0;
        PFN_vkAllocateMemory                vkAllocateMemory = 0;
        PFN_vkFreeMemory                    vkFreeMemory = 0;
        PFN_vkMapMemory                     vkMapMemory = 0;
        PFN_vkUnmapMemory                   vkUnmapMemory = 0;
        PFN_vkBindBufferMemory              vkBindBufferMemory = 0;
        PFN_vkBindImageMemory               vkBindImageMemory = 0;
        PFN_vkUpdateDescriptorSets          vkUpdateDescriptorSets = 0;
        PFN_vkFlushMappedMemoryRanges       vkFlushMappedMemoryRanges = 0;
        PFN_vkCmdPipelineBarrier            vkCmdPipelineBarrier = 0;
        PFN_vkCmdBindPipeline               vkCmdBindPipeline = 0;
        PFN_vkCmdBindDescriptorSets         vkCmdBindDescriptorSets = 0;
        PFN_vkCmdDispatch                   vkCmdDispatch = 0;
        PFN_vkCmdDispatchIndirect           vkCmdDispatchIndirect = 0;
        PFN_vkCmdCopyBuffer                 vkCmdCopyBuffer = 0;
        PFN_vkCmdCopyImage                  vkCmdCopyImage = 0;
        PFN_vkCmdCopyBufferToImage          vkCmdCopyBufferToImage = 0;
        PFN_vkCmdClearColorImage            vkCmdClearColorImage = 0;
        PFN_vkCmdFillBuffer                 vkCmdFillBuffer = 0;
    } VkFunctionTable;

    VkDevice                device = nullptr;
    VkPhysicalDevice        physicalDevice = nullptr;
    VkFunctionTable         vkFunctionTable = {};

    FfxGpuJobDescription*   pGpuJobs;
    uint32_t                gpuJobCount = 0;

    typedef struct VkResourceView {
        VkImageView imageView;
    } VkResourceView;
    VkResourceView*         pResourceViews;

    VkDeviceMemory          ringBufferMemory = nullptr;
    VkMemoryPropertyFlags   ringBufferMemoryProperties = 0;
    UniformBuffer*          pRingBuffer;
    uint32_t                ringBufferBase = 0;

    PipelineLayout*         pPipelineLayouts;

    VkDescriptorPool        descriptorPool;

    VkImageMemoryBarrier    imageMemoryBarriers[FFX_MAX_BARRIERS] = {};
    VkBufferMemoryBarrier   bufferMemoryBarriers[FFX_MAX_BARRIERS] = {};
    uint32_t                scheduledImageBarrierCount = 0;
    uint32_t                scheduledBufferBarrierCount = 0;
    VkPipelineStageFlags    srcStageMask = 0;
    VkPipelineStageFlags    dstStageMask = 0;

    typedef struct alignas(32) EffectContext {

        // Resource allocation
        uint32_t              nextStaticResource;
        uint32_t              nextDynamicResource;

        // UAV offsets
        uint32_t              nextStaticResourceView;
        uint32_t              nextDynamicResourceView[FFX_MAX_QUEUED_FRAMES];

        // Pipeline layout
        uint32_t              nextPipelineLayout;

        // the frame index for the context
        uint32_t              frameIndex;

        // Usage
        bool                  active;

    } EffectContext;

    Resource*               pResources;
    EffectContext*          pEffectContexts;

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
    uint32_t resourceViewArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2 * sizeof(BackendContext_VK::VkResourceView), sizeof(uint32_t));
    uint32_t ringBufferArraySize = FFX_ALIGN_UP(maxContexts * FFX_RING_BUFFER_SIZE * sizeof(BackendContext_VK::UniformBuffer), sizeof(uint32_t));
    uint32_t pipelineArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(uint32_t));
    uint32_t resourceArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(uint32_t));
    uint32_t contextArraySize = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_VK::EffectContext), sizeof(uint32_t));

    return FFX_ALIGN_UP(sizeof(BackendContext_VK) + extensionPropArraySize + gpuJobDescArraySize + resourceArraySize + ringBufferArraySize +
        pipelineArraySize + resourceArraySize + contextArraySize, sizeof(uint64_t));
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
        !s_BackendRefCount,
        FFX_ERROR_BACKEND_API_ERROR);
    FFX_RETURN_ON_ERROR(
        backendInterface,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBuffer,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBufferSize >= ffxGetScratchMemorySizeVK(((VkDeviceContext*)device)->vkPhysicalDevice, maxContexts),
        FFX_ERROR_INSUFFICIENT_MEMORY);

    backendInterface->fpGetSDKVersion = GetSDKVersionVK;
    backendInterface->fpCreateBackendContext = CreateBackendContextVK;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesVK;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextVK;
    backendInterface->fpCreateResource = CreateResourceVK;
    backendInterface->fpDestroyResource = DestroyResourceVK;
    backendInterface->fpRegisterResource = RegisterResourceVK;
    backendInterface->fpGetResource = GetResourceVK;
    backendInterface->fpUnregisterResources = UnregisterResourcesVK;
    backendInterface->fpGetResourceDescription = GetResourceDescriptionVK;
    backendInterface->fpCreatePipeline = CreatePipelineVK;
    backendInterface->fpDestroyPipeline = DestroyPipelineVK;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobVK;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsVK;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    // Map the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    s_MaxEffectContexts = static_cast<uint32_t>(maxContexts);

    return FFX_OK;
}

FfxCommandList ffxGetCommandListVK(VkCommandBuffer cmdBuf)
{
    FFX_ASSERT(NULL != cmdBuf);
    return reinterpret_cast<FfxCommandList>(cmdBuf);
}

FfxResource ffxGetResourceVK(void* vkResource,
    FfxResourceDescription          ffxResDescription,
    wchar_t* ffxResName,
    FfxResourceStates               state /*=FFX_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxResource resource = {};
    resource.resource = vkResource;
    resource.state = state;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (ffxResName) {
        wcscpy_s(resource.name, ffxResName);
    }
#endif

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

VkBufferUsageFlags ffxGetVKBufferUsageFlagsFromResourceUsage(FfxResourceUsage flags)
{
    VkBufferUsageFlags indirectBit = 0;

    if (flags & FFX_RESOURCE_USAGE_INDIRECT)
        indirectBit = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (flags & FFX_RESOURCE_USAGE_UAV)
        return indirectBit | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else
        return indirectBit | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

VkImageType ffxGetVKImageTypeFromResourceType(FfxResourceType type)
{
    switch (type) {
    case(FFX_RESOURCE_TYPE_TEXTURE1D):
        return VK_IMAGE_TYPE_1D;
    case(FFX_RESOURCE_TYPE_TEXTURE2D):
        return VK_IMAGE_TYPE_2D;
    case(FFX_RESOURCE_TYPE_TEXTURE_CUBE):
    case(FFX_RESOURCE_TYPE_TEXTURE3D):
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

bool ffxIsSurfaceFormatSRGB(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return true;
    case (FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
    case (FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
    case (FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
    case (FFX_SURFACE_FORMAT_R32G32_FLOAT):
    case (FFX_SURFACE_FORMAT_R32_UINT):
    case (FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
    case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
    case (FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
    case (FFX_SURFACE_FORMAT_R16G16_FLOAT):
    case (FFX_SURFACE_FORMAT_R16G16_UINT):
    case (FFX_SURFACE_FORMAT_R16_FLOAT):
    case (FFX_SURFACE_FORMAT_R16_UINT):
    case (FFX_SURFACE_FORMAT_R16_UNORM):
    case (FFX_SURFACE_FORMAT_R16_SNORM):
    case (FFX_SURFACE_FORMAT_R8_UNORM):
    case (FFX_SURFACE_FORMAT_R8_UINT):
    case (FFX_SURFACE_FORMAT_R8G8_UNORM):
    case (FFX_SURFACE_FORMAT_R32_FLOAT):
    case (FFX_SURFACE_FORMAT_UNKNOWN):
        return false;
    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return false;
    }
}

FfxSurfaceFormat ffxGetSurfaceFormatFromGamma(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
    case (FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case (FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case (FFX_SURFACE_FORMAT_R32G32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32_FLOAT;
    case (FFX_SURFACE_FORMAT_R32_UINT):
        return FFX_SURFACE_FORMAT_R32_UINT;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_SNORM;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
        return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
    case (FFX_SURFACE_FORMAT_R16G16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case (FFX_SURFACE_FORMAT_R16G16_UINT):
        return FFX_SURFACE_FORMAT_R16G16_UINT;
    case (FFX_SURFACE_FORMAT_R16_FLOAT):
        return FFX_SURFACE_FORMAT_R16_FLOAT;
    case (FFX_SURFACE_FORMAT_R16_UINT):
        return FFX_SURFACE_FORMAT_R16_UINT;
    case (FFX_SURFACE_FORMAT_R16_UNORM):
        return FFX_SURFACE_FORMAT_R16_UNORM;
    case (FFX_SURFACE_FORMAT_R16_SNORM):
        return FFX_SURFACE_FORMAT_R16_SNORM;
    case (FFX_SURFACE_FORMAT_R8_UNORM):
        return FFX_SURFACE_FORMAT_R8_UNORM;
    case (FFX_SURFACE_FORMAT_R8G8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8_UNORM;
    case (FFX_SURFACE_FORMAT_R32_FLOAT):
        return FFX_SURFACE_FORMAT_R32_FLOAT;
    case (FFX_SURFACE_FORMAT_UNKNOWN):
        return FFX_SURFACE_FORMAT_UNKNOWN;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

VkFormat ffxGetVKSurfaceFormatFromSurfaceFormat(FfxSurfaceFormat fmt)
{
    switch (fmt) {

    case(FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_SRGB;
    case(FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case(FFX_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFX_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFX_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFX_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFX_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
    case(FFX_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFX_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFX_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFX_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

VkFormat ffxGetVKUAVFormatFromSurfaceFormat(FfxSurfaceFormat fmt)
{
    switch (fmt) {

    case(FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case(FFX_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFX_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFX_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFX_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFX_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
    case(FFX_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFX_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFX_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFX_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

VkImageUsageFlags getVKImageUsageFlagsFromResourceUsage(FfxResourceUsage flags)
{
    VkImageUsageFlags ret = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (flags & FFX_RESOURCE_USAGE_RENDERTARGET) ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (flags & FFX_RESOURCE_USAGE_UAV) ret |= (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    return ret;
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

VkAccessFlags getVKAccessFlagsFromResourceState(FfxResourceStates state)
{
    switch (state) {

    case(FFX_RESOURCE_STATE_GENERIC_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case(FFX_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_READ):
    case(FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case FFX_RESOURCE_STATE_COPY_SRC:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    default:
        FFX_ASSERT_MESSAGE(false, "State flag not yet supported");
        return VK_ACCESS_SHADER_READ_BIT;
    }
}

VkPipelineStageFlags getVKPipelineStageFlagsFromResourceState(FfxResourceStates state)
{
    switch (state) {

    case(FFX_RESOURCE_STATE_GENERIC_READ):
    case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
    case(FFX_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_READ):
    case(FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case(FFX_RESOURCE_STATE_INDIRECT_ARGUMENT):
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    case FFX_RESOURCE_STATE_COPY_SRC:
    case FFX_RESOURCE_STATE_COPY_DEST:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    default:
        FFX_ASSERT_MESSAGE(false, "Pipeline stage flag not yet supported");
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
}

VkImageLayout getVKImageLayoutFromResourceState(FfxResourceStates state)
{
    switch (state) {

    case(FFX_RESOURCE_STATE_GENERIC_READ):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_READ):
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case FFX_RESOURCE_STATE_COPY_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    default:
        FFX_ASSERT_MESSAGE(false, "Image layout flag not yet supported");
        return VK_IMAGE_LAYOUT_GENERAL;
    }
}

void addMutableViewForSRV(VkImageViewCreateInfo& imageViewCreateInfo, VkImageViewUsageCreateInfo& imageViewUsageCreateInfo, FfxResourceDescription resourceDescription)
{
    if (ffxIsSurfaceFormatSRGB(resourceDescription.format) && (resourceDescription.usage & FFX_RESOURCE_USAGE_UAV) != 0)
    {
        // mutable is only for sRGB textures that will need a storage
        imageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
        imageViewUsageCreateInfo.pNext = nullptr;
        imageViewUsageCreateInfo.usage = getVKImageUsageFlagsFromResourceUsage(FFX_RESOURCE_USAGE_READ_ONLY);  // we can assume that SRV is enough
        imageViewCreateInfo.pNext      = &imageViewUsageCreateInfo;
    }
}

void copyResourceState(BackendContext_VK::Resource* backendResource, const FfxResource* inFfxResource)
{
    FfxResourceStates state = inFfxResource->state;

    // copy the new states
    backendResource->initialState = state;
    backendResource->currentState = state;
    backendResource->undefined    = false;
    backendResource->dynamic      = true;

    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so tag the resource as undefined so we can transition it accordingly.
    if (backendResource->resourceDescription.flags & FFX_RESOURCE_FLAGS_UNDEFINED)
    {
        backendResource->undefined                 = true;
        backendResource->resourceDescription.flags = (FfxResourceFlags)((int)backendResource->resourceDescription.flags & ~FFX_RESOURCE_FLAGS_UNDEFINED);
    }
}

void addBarrier(BackendContext_VK* backendContext, FfxResourceInternal* resource, FfxResourceStates newState)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != resource);

    BackendContext_VK::Resource& ffxResource = backendContext->pResources[resource->internalIndex];

    if (ffxResource.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResource = ffxResource.bufferResource;
        VkBufferMemoryBarrier* barrier = &backendContext->bufferMemoryBarriers[backendContext->scheduledBufferBarrierCount];

        FfxResourceStates& curState = backendContext->pResources[resource->internalIndex].currentState;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->srcQueueFamilyIndex = 0;
        barrier->dstQueueFamilyIndex = 0;
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
        VkImageMemoryBarrier* barrier = &backendContext->imageMemoryBarriers[backendContext->scheduledImageBarrierCount];

        FfxResourceStates& curState = backendContext->pResources[resource->internalIndex].currentState;

        VkImageSubresourceRange range;
        range.aspectMask = ffxResource.resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->oldLayout = ffxResource.undefined ? VK_IMAGE_LAYOUT_UNDEFINED : getVKImageLayoutFromResourceState(curState);
        barrier->newLayout = getVKImageLayoutFromResourceState(newState);
        barrier->srcQueueFamilyIndex = 0;
        barrier->dstQueueFamilyIndex = 0;
        barrier->image = vkResource;
        barrier->subresourceRange = range;

        backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
        backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

        curState = newState;

        ++backendContext->scheduledImageBarrierCount;
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

//////////////////////////////////////////////////////////////////////////
// VK back end implementation

FfxUInt32 GetSDKVersionVK(FfxInterface* backendInterface)
{
    return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

FfxErrorCode CreateBackendContextVK(FfxInterface* backendInterface, FfxUInt32* effectContextId)
{
    VkDeviceContext* vkDeviceContext = reinterpret_cast<VkDeviceContext*>(backendInterface->device);

    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != vkDeviceContext);
    FFX_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkDevice);
    FFX_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkPhysicalDevice);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!s_BackendRefCount) {

        // clear out mem prior to initializing
        memset(backendContext, 0, sizeof(BackendContext_VK));

        // Map all of our pointers
        uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint32_t resourceViewArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2 * sizeof(BackendContext_VK::VkResourceView), sizeof(uint32_t));
        uint32_t ringBufferArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_RING_BUFFER_SIZE * sizeof(BackendContext_VK::UniformBuffer), sizeof(uint32_t));
        uint32_t pipelineArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(uint32_t));
        uint32_t resourceArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(uint32_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * sizeof(BackendContext_VK::EffectContext), sizeof(uint32_t));
        uint8_t* pMem = (uint8_t*)((BackendContext_VK*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);
        pMem += gpuJobDescArraySize;

        // Map the resource view array
        backendContext->pResourceViews = (BackendContext_VK::VkResourceView*)(pMem);
        memset(backendContext->pResourceViews, 0, resourceViewArraySize);
        pMem += resourceViewArraySize;

        // Map the ring buffer array
        backendContext->pRingBuffer = (BackendContext_VK::UniformBuffer*)pMem;
        memset(backendContext->pRingBuffer, 0, ringBufferArraySize);
        pMem += ringBufferArraySize;

        // Map pipeline array
        backendContext->pPipelineLayouts = (BackendContext_VK::PipelineLayout*)pMem;
        memset(backendContext->pPipelineLayouts, 0, pipelineArraySize);
        pMem += pipelineArraySize;

        // Map resource array
        backendContext->pResources = (BackendContext_VK::Resource*)pMem;
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Clear out all resource mappings
        for (int i = 0; i < s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT; ++i) {
            backendContext->pResources[i].uavViewIndex = backendContext->pResources[i].srvViewIndex = -1;
        }

        // Map context array
        backendContext->pEffectContexts = (BackendContext_VK::EffectContext*)pMem;
        memset(backendContext->pEffectContexts, 0, contextArraySize);
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
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorSetLayout");
        backendContext->vkFunctionTable.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorPool");
        backendContext->vkFunctionTable.vkDestroySampler = (PFN_vkDestroySampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroySampler");
        backendContext->vkFunctionTable.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyShaderModule");
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetBufferMemoryRequirements");
        backendContext->vkFunctionTable.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetImageMemoryRequirements");
        backendContext->vkFunctionTable.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateDescriptorSets");
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

        // enumerate all the device extensions
        backendContext->numDeviceExtensions = 0;
        vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, nullptr);
        vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, backendContext->extensionProperties);

        // create a global descriptor pool to hold all descriptors we'll need
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES },
            { VK_DESCRIPTOR_TYPE_SAMPLER, s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES },
        };

        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.poolSizeCount = 5;
        descriptorPoolCreateInfo.pPoolSizes = poolSizes;
        descriptorPoolCreateInfo.maxSets = s_MaxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES;

        if (backendContext->vkFunctionTable.vkCreateDescriptorPool(backendContext->device, &descriptorPoolCreateInfo, nullptr, &backendContext->descriptorPool) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        // allocate ring buffer of uniform buffers
        {
            for (uint32_t i = 0; i < FFX_RING_BUFFER_SIZE * s_MaxEffectContexts; i++)
            {
                BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[i];
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = FFX_BUFFER_SIZE;
                bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &uBuffer.bufferResource) != VK_SUCCESS) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            // allocate memory block for all uniform buffers
            VkMemoryRequirements memRequirements = {};
            backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendContext->pRingBuffer[0].bufferResource, &memRequirements);

            VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = s_MaxEffectContexts * FFX_RING_BUFFER_MEM_BLOCK_SIZE;
            allocInfo.memoryTypeIndex = findMemoryTypeIndex(backendContext->physicalDevice, memRequirements, requiredMemoryProperties, backendContext->ringBufferMemoryProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX) {
                requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.memoryTypeIndex = findMemoryTypeIndex(backendContext->physicalDevice, memRequirements, requiredMemoryProperties, backendContext->ringBufferMemoryProperties);

                if (allocInfo.memoryTypeIndex == UINT32_MAX) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendContext->ringBufferMemory);

            if (result != VK_SUCCESS) {
                switch (result) {
                case(VK_ERROR_OUT_OF_HOST_MEMORY):
                case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
                    return FFX_ERROR_OUT_OF_MEMORY;
                default:
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            // map the memory block
            uint8_t* pData = nullptr;

            if (backendContext->vkFunctionTable.vkMapMemory(backendContext->device, backendContext->ringBufferMemory, 0, s_MaxEffectContexts * FFX_RING_BUFFER_MEM_BLOCK_SIZE, 0, reinterpret_cast<void**>(&pData)) != VK_SUCCESS) {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            // bind each FFX_BUFFER_SIZE-byte block to the ubos
            for (uint32_t i = 0; i < s_MaxEffectContexts * FFX_RING_BUFFER_SIZE; i++)
            {
                BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[i];

                // get the buffer memory requirements for each buffer object to silence validation errors
                VkMemoryRequirements memRequirements = {};
                backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, uBuffer.bufferResource, &memRequirements);

                uBuffer.pData = pData + FFX_BUFFER_SIZE * i;

                if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, uBuffer.bufferResource, backendContext->ringBufferMemory, FFX_BUFFER_SIZE * i) != VK_SUCCESS) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }
        }
    }

    // Increment the ref count
    ++s_BackendRefCount;

    // Get an available context id
    for (size_t i = 0; i < s_MaxEffectContexts; ++i) {
        if (!backendContext->pEffectContexts[i].active) {
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.nextStaticResource = (i * FFX_MAX_RESOURCE_COUNT);
            effectContext.nextDynamicResource = getDynamicResourcesStartIndex(i);
            effectContext.nextStaticResourceView = (i * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2);
            for (uint32_t frameIndex = 0; frameIndex < FFX_MAX_QUEUED_FRAMES; ++frameIndex)
            {
                effectContext.nextDynamicResourceView[frameIndex] = getDynamicResourceViewsStartIndex(i, frameIndex);
            }
            effectContext.nextPipelineLayout = (i * FFX_MAX_PASS_COUNT);
            effectContext.frameIndex = 0;
            break;
        }
    }

    return FFX_OK;
}

FfxErrorCode GetDeviceCapabilitiesVK(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // no shader model in vulkan so assume the minimum
    deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
    deviceCapabilities->waveLaneCountMin = 32;
    deviceCapabilities->waveLaneCountMax = 32;
    deviceCapabilities->fp16Supported = false;
    deviceCapabilities->raytracingSupported = false;

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
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME) == 0)
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
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
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
    }

    return FFX_OK;
}

FfxErrorCode DestroyBackendContextVK(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // Delete any resources allocated by this context
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (int32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex) {

        if (backendContext->pResources[currentStaticResourceIndex].imageResource != VK_NULL_HANDLE) {
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxResourceInternal internalResource = { currentStaticResourceIndex };
            DestroyResourceVK(backendInterface, internalResource, effectContextId);
        }
    }

    for (uint32_t frameIndex = 0; frameIndex < FFX_MAX_QUEUED_FRAMES; ++frameIndex)
        destroyDynamicViews(backendContext, effectContextId, frameIndex);

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --s_BackendRefCount;

    if (!s_BackendRefCount) {

        // clean up descriptor pool
        backendContext->vkFunctionTable.vkDestroyDescriptorPool(backendContext->device, backendContext->descriptorPool, VK_NULL_HANDLE);
        backendContext->descriptorPool = VK_NULL_HANDLE;

        // clean up ring buffer & memory
        for (uint32_t i = 0; i < s_MaxEffectContexts * FFX_RING_BUFFER_SIZE; i++) {
            BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[i];
            backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, uBuffer.bufferResource, VK_NULL_HANDLE);
            uBuffer.bufferResource = VK_NULL_HANDLE;
            uBuffer.pData = VK_NULL_HANDLE;
        }

        backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendContext->ringBufferMemory);
        backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backendContext->ringBufferMemory, VK_NULL_HANDLE);
        backendContext->ringBufferMemory = VK_NULL_HANDLE;

        if (backendContext->device != VK_NULL_HANDLE)
            backendContext->device = VK_NULL_HANDLE;
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

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    VkDevice vkDevice = backendContext->device;

    FFX_ASSERT(VK_NULL_HANDLE != vkDevice);

    VkMemoryPropertyFlags requiredMemoryProperties;
    if (createResourceDescription->heapType == FFX_HEAP_TYPE_UPLOAD)
        requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    else
        requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Setup the resource description
    FfxResourceDescription resourceDesc = createResourceDescription->resourceDescription;

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

    const FfxResourceStates resourceState = (createResourceDescription->initData && (createResourceDescription->heapType != FFX_HEAP_TYPE_UPLOAD)) ? FFX_RESOURCE_STATE_COPY_DEST : createResourceDescription->initalState;
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
    case FFX_RESOURCE_TYPE_BUFFER:
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = createResourceDescription->resourceDescription.width;
        bufferInfo.usage = ffxGetVKBufferUsageFlagsFromResourceUsage(resourceDesc.usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (createResourceDescription->initData)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &backendResource->bufferResource) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)backendResource->bufferResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendResource->bufferResource, &memRequirements);

        // allocate the memory
        FfxErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
        if (FFX_OK != errorCode)
            return errorCode;

        if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, backendResource->bufferResource, backendResource->deviceMemory, 0) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        // if this is an upload buffer (currently only support upload buffers), copy the data and return
        if (createResourceDescription->heapType == FFX_HEAP_TYPE_UPLOAD)
        {
            // only allow copies directly into mapped memory for buffer resources since all texture resources are in optimal tiling
            void* data = NULL;

            if (backendContext->vkFunctionTable.vkMapMemory(backendContext->device, backendResource->deviceMemory, 0, createResourceDescription->initDataSize, 0, &data) != VK_SUCCESS) {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            memcpy(data, createResourceDescription->initData, createResourceDescription->initDataSize);

            // flush mapped range if memory type is not coherant
            if ((backendResource->memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendResource->deviceMemory;
                memoryRange.size = createResourceDescription->initDataSize;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }

            backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendResource->deviceMemory);
            return FFX_OK;
        }

        break;
    }
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = ffxGetVKImageTypeFromResourceType(createResourceDescription->resourceDescription.type);
        imageInfo.extent.width = createResourceDescription->resourceDescription.width;
        imageInfo.extent.height = createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE1D ? 1 : createResourceDescription->resourceDescription.height;
        imageInfo.extent.depth = ( createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE3D || createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE_CUBE ) ?
            createResourceDescription->resourceDescription.depth : 1;
        imageInfo.mipLevels = backendResource->resourceDescription.mipCount;
        imageInfo.arrayLayers = (createResourceDescription->resourceDescription.type ==
                                FFX_RESOURCE_TYPE_TEXTURE1D || createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE2D)
            ? createResourceDescription->resourceDescription.depth : 1;
        imageInfo.format = ((resourceDesc.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) != 0) ? VK_FORMAT_D32_SFLOAT : ffxGetVKSurfaceFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = getVKImageUsageFlagsFromResourceUsage(resourceDesc.usage);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ((resourceDesc.usage & FFX_RESOURCE_USAGE_UAV) != 0 && ffxIsSurfaceFormatSRGB(createResourceDescription->resourceDescription.format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageInfo.format = ffxGetVKSurfaceFormatFromSurfaceFormat(ffxGetSurfaceFormatFromGamma(createResourceDescription->resourceDescription.format));
        }

        if (backendContext->vkFunctionTable.vkCreateImage(backendContext->device, &imageInfo, nullptr, &backendResource->imageResource) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)backendResource->imageResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetImageMemoryRequirements(backendContext->device, backendResource->imageResource, &memRequirements);

        // allocate the memory
        FfxErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
        if (FFX_OK != errorCode)
            return errorCode;

        if (backendContext->vkFunctionTable.vkBindImageMemory(backendContext->device, backendResource->imageResource, backendResource->deviceMemory, 0) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
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
    case FFX_RESOURCE_TYPE_BUFFER:
        break;
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        FFX_ASSERT_MESSAGE(effectContext.nextStaticResourceView + 1 < effectContext.nextDynamicResourceView[0],
            "FFXInterface: Vulkan: We've run out of resource views. Please increase the size.");
        backendResource->srvViewIndex = effectContext.nextStaticResourceView++;

        FfxResourceType type = createResourceDescription->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        bool isDepth = ((backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) != 0);
        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = isDepth ? VK_FORMAT_D32_SFLOAT : ffxGetVKSurfaceFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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
        if (backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_UAV)
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFX_ASSERT(effectContext.nextStaticResourceView + uavResourceViewCount < effectContext.nextDynamicResourceView[0]);

            backendResource->uavViewIndex = effectContext.nextStaticResourceView;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = (backendResource->resourceDescription.usage == FFX_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKUAVFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);

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
    if (createResourceDescription->initData)
    {
        FfxResourceInternal copySrc;
        FfxCreateResourceDescription uploadDesc = { *createResourceDescription };
        uploadDesc.heapType = FFX_HEAP_TYPE_UPLOAD;
        uploadDesc.resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
        uploadDesc.resourceDescription.width = createResourceDescription->initDataSize;
        uploadDesc.resourceDescription.usage = FFX_RESOURCE_USAGE_READ_ONLY;
        uploadDesc.initalState = FFX_RESOURCE_STATE_GENERIC_READ;
        uploadDesc.initData = createResourceDescription->initData;
        uploadDesc.initDataSize = createResourceDescription->initDataSize;

        backendInterface->fpCreateResource(backendInterface, &uploadDesc, effectContextId, &copySrc);

        // setup the upload job
        FfxGpuJobDescription copyJob =
        {
            FFX_GPU_JOB_COPY
        };
        copyJob.copyJobDescriptor.src = copySrc;
        copyJob.copyJobDescriptor.dst = *outResource;

        backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId)
{
    FFX_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

     if (resource.internalIndex != -1 &&
        ((resource.internalIndex >= int32_t(effectContextId * FFX_MAX_RESOURCE_COUNT)) && (resource.internalIndex < int32_t(effectContext.nextStaticResource))))
     {
         BackendContext_VK::Resource& backgroundResource = backendContext->pResources[resource.internalIndex];

         if (backgroundResource.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
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
             backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView, nullptr);
             backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView = VK_NULL_HANDLE;
             backgroundResource.srvViewIndex = 0;

             // And UAVs
             if (backgroundResource.resourceDescription.usage & FFX_RESOURCE_USAGE_UAV)
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
         }
     }

    return FFX_OK;
}

FfxErrorCode RegisterResourceVK(
    FfxInterface* backendInterface,
    const FfxResource* inFfxResource,
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
        if ((backendResource->resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxResource->resource) ||
            (backendResource->resourceDescription.type != FFX_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxResource->resource))
            return FFX_OK;

        // If this isn't us, search until we either find our entry or an empty resource
        outFfxResourceInternal->internalIndex = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
        while (!setupDynamicResource)
        {
            FFX_ASSERT(outFfxResourceInternal->internalIndex > effectContext.nextStaticResource); // Safety check while iterating
            backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];

            // Is this us?
            if ((backendResource->resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxResource->resource) ||
                (backendResource->resourceDescription.type != FFX_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxResource->resource))
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
    if (inFfxResource->description.type == FFX_RESOURCE_TYPE_BUFFER)
        backendResource->bufferResource = reinterpret_cast<VkBuffer>(inFfxResource->resource);
    else
        backendResource->imageResource = reinterpret_cast<VkImage>(inFfxResource->resource);

    copyResourceState(backendResource, inFfxResource);

#ifdef _DEBUG
    size_t retval = 0;
    wcstombs_s(&retval, backendResource->resourceName, sizeof(backendResource->resourceName), inFfxResource->name, sizeof(backendResource->resourceName));
    if (retval >= 64) backendResource->resourceName[63] = '\0';
#endif

    // the first call of RegisterResource can be identified because


    //////////////////////////////////////////////////////////////////////////
    // Create SRVs and UAVs
    switch (backendResource->resourceDescription.type)
    {
    case FFX_RESOURCE_TYPE_BUFFER:
        break;
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxResourceType type = backendResource->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = (backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKSurfaceFormatFromSurfaceFormat(backendResource->resourceDescription.format);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = (backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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
        if (backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_UAV)
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFX_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1 > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
            backendResource->uavViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = (backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKUAVFormatFromSurfaceFormat(backendResource->resourceDescription.format);
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

FfxResource GetResourceVK(FfxInterface* backendInterface, FfxResourceInternal inResource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxResourceDescription ffxResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxResource resource = {};
    resource.resource = reinterpret_cast<void*>(backendContext->pResources[inResource.internalIndex].imageResource);
    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so we will flag it as such to finish initializing it later
    // before it is used.
    if (backendContext->pResources[inResource.internalIndex].undefined) {
        ffxResDescription.flags = (FfxResourceFlags)((int)ffxResDescription.flags | FFX_RESOURCE_FLAGS_UNDEFINED);
        // Flag it as no longer being undefined as it will no longer be after workload
        // execution
        backendContext->pResources[inResource.internalIndex].undefined = false;
    }
    resource.state = backendContext->pResources[inResource.internalIndex].currentState;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (backendContext->pResources[inResource.internalIndex].resourceName)
    {
        mbstowcs(resource.name, backendContext->pResources[inResource.internalIndex].resourceName, strlen(backendContext->pResources[inResource.internalIndex].resourceName));
    }
#endif

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

FfxResourceDescription GetResourceDescriptionVK(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
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

FfxErrorCode CreatePipelineVK(FfxInterface* backendInterface,
    FfxEffect effect,
    FfxPass pass,
    uint32_t permutationOptions,
    const FfxPipelineDescription* pipelineDescription,
    FfxUInt32 effectContextId,
    FfxPipelineState* outPipeline)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != pipelineDescription);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // start by fetching the shader blob
    FfxShaderBlob shaderBlob = { };
    ffxGetPermutationBlobByIndex(effect, pass, FFX_BIND_COMPUTE_SHADER_STAGE, permutationOptions, &shaderBlob);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

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

    // Setup descriptor sets
    VkDescriptorSetLayoutBinding layoutBindings[MAX_DESCRIPTOR_SET_LAYOUTS];
    uint32_t numLayoutBindings = 0;

    // Support more when needed
    VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Samplers - just the static ones for now
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
        layoutBindings[numLayoutBindings++] = { currentSamplerIndex + SAMPLER_BINDING_SHIFT, VK_DESCRIPTOR_TYPE_SAMPLER, (uint32_t)pipelineDescription->samplerCount, shaderStageFlags, pPipelineLayout->samplers };

    // Texture SRVs
    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        layoutBindings[numLayoutBindings++] = { shaderBlob.boundSRVTextures[srvIndex], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            shaderBlob.boundSRVTextureCounts[srvIndex], shaderStageFlags, nullptr };
    }

    // Buffer SRVs
    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        layoutBindings[numLayoutBindings++] = { shaderBlob.boundSRVBuffers[srvIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundSRVBufferCounts[srvIndex], shaderStageFlags, nullptr};
    }

    // Texture UAVs
    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        layoutBindings[numLayoutBindings++] = { shaderBlob.boundUAVTextures[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            shaderBlob.boundUAVTextureCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Buffer UAVs
    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        layoutBindings[numLayoutBindings++] = { shaderBlob.boundUAVBuffers[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundUAVBufferCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Constant buffers (uniforms)
    for (uint32_t cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        layoutBindings[numLayoutBindings++] = { shaderBlob.boundConstantBuffers[cbIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            shaderBlob.boundConstantBufferCounts[cbIndex], shaderStageFlags, nullptr };
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
    for (uint32_t i = 0; i < FFX_MAX_QUEUED_FRAMES; i++)
    {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = backendContext->descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &pPipelineLayout->descriptorSetLayout;

        backendContext->vkFunctionTable.vkAllocateDescriptorSets(backendContext->device, &allocateInfo, &pPipelineLayout->descriptorSets[i]);
    }

    // create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &pPipelineLayout->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

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

    // populate the pipeline information for this pass
    outPipeline->srvTextureCount = shaderBlob.srvTextureCount;
    outPipeline->srvBufferCount  = shaderBlob.srvBufferCount;
    outPipeline->uavTextureCount = shaderBlob.uavTextureCount;
    outPipeline->uavBufferCount = shaderBlob.uavBufferCount;

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    outPipeline->constCount = shaderBlob.cbvCount;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvTextureCount; ++srvIndex)
    {
        outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];
        wcscpy_s(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVTextureNames[srvIndex]).c_str());
    }
    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvBufferCount; ++srvIndex)
    {
        outPipeline->srvBufferBindings[srvIndex].slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];
        wcscpy_s(outPipeline->srvBufferBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVBufferNames[srvIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavTextureCount; ++uavIndex)
    {
        outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];
        wcscpy_s(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVTextureNames[uavIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavBufferCount; ++uavIndex)
    {
        outPipeline->uavBufferBindings[uavIndex].slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];
        wcscpy_s(outPipeline->uavBufferBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVBufferNames[uavIndex]).c_str());
    }
    for (uint32_t cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlob.boundConstantBufferCounts[cbIndex];
        wcscpy_s(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlob.boundConstantBufferNames[cbIndex]).c_str());
    }

    //////////////////////////////////////////////////////////////////////////
    // pipeline creation
    FfxDeviceCapabilities capabilities;
    backendInterface->fpGetDeviceCapabilities(backendInterface, &capabilities);

    // shader module
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (uint32_t*)shaderBlob.data;
    shaderModuleCreateInfo.codeSize = shaderBlob.size;

    if (backendContext->vkFunctionTable.vkCreateShaderModule(backendContext->device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // fill out shader stage create info
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfo.module = shaderModule;

    // check if wave64 is requested
    bool isWave64 = false;
    ffxIsWave64(effect, permutationOptions, isWave64);
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroupSizeCreateInfo = {};
    if (isWave64 && (capabilities.waveLaneCountMin == 32 && capabilities.waveLaneCountMax == 64))
    {
        subgroupSizeCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT;
        subgroupSizeCreateInfo.requiredSubgroupSize = 64;
        shaderStageCreateInfo.pNext = &subgroupSizeCreateInfo;
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

    return FFX_OK;
}

FfxErrorCode DestroyPipelineVK(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId)
{
    FFX_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

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
        for (uint32_t i = 0; i < FFX_MAX_QUEUED_FRAMES; i++)
            pPipelineLayout->descriptorSets[i] = VK_NULL_HANDLE;

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

    if (job->jobType == FFX_GPU_JOB_COMPUTE) {

        // needs to copy SRVs and UAVs in case they are on the stack only
        FfxComputeJobDescription* computeJob = &backendContext->pGpuJobs[backendContext->gpuJobCount].computeJobDescriptor;
        const uint32_t numConstBuffers = job->computeJobDescriptor.pipeline.constCount;
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < numConstBuffers; ++currentRootConstantIndex)
        {
            computeJob->cbs[currentRootConstantIndex].num32BitEntries = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries;
            memcpy(computeJob->cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, computeJob->cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));
        }
    }

    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->computeJobDescriptor.pipeline.rootSignature);

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
    for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.uavTextures[currentPipelineUavIndex], FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        // where to bind it
        const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex;

        for (uint32_t uavEntry = 0; uavEntry < job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindCount; ++uavEntry, ++imageDescriptorIndex, ++descriptorWriteIndex)
        {
            // source: UAV of resource to bind
            const uint32_t resourceIndex = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].internalIndex;
            const uint32_t uavViewIndex = backendContext->pResources[resourceIndex].uavViewIndex + job->computeJobDescriptor.uavTextureMips[imageDescriptorIndex];

            writeDescriptorSets[descriptorWriteIndex] = {};
            writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
            writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
            writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[descriptorWriteIndex].pImageInfo = &imageDescriptorInfos[imageDescriptorIndex];
            writeDescriptorSets[descriptorWriteIndex].dstBinding = currentUavResourceIndex;
            writeDescriptorSets[descriptorWriteIndex].dstArrayElement = uavEntry;

            imageDescriptorInfos[imageDescriptorIndex] = {};
            imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageDescriptorInfos[imageDescriptorIndex].imageView = backendContext->pResourceViews[uavViewIndex].imageView;
        }
    }

    // bind buffer UAVs
    for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex) {

        addBarrier(backendContext, &job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex], FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        // source: UAV of buffer to bind
        const uint32_t resourceIndex = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].internalIndex;

        // where to bind it
        const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding = currentUavResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        bufferDescriptorInfos[bufferDescriptorIndex] = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range = VK_WHOLE_SIZE;
    }

    // bind texture SRVs
    for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount;
         ++currentPipelineSrvIndex, ++descriptorWriteIndex)
    {

        // where to bind it
        const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = currentSrvResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        for (int i = 0; i < job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount; ++i, ++imageDescriptorIndex)
        {
            addBarrier(backendContext, &job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex + i], FFX_RESOURCE_STATE_COMPUTE_READ);

            const uint32_t resourceIndex = job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex + i].internalIndex;
            const uint32_t srvViewIndex  = backendContext->pResources[resourceIndex].srvViewIndex;

            imageDescriptorInfos[imageDescriptorIndex]             = {};
            imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[srvViewIndex].imageView;
        }
    }

    // bind buffer SRVs
    for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount;
         ++currentPipelineSrvIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex], FFX_RESOURCE_STATE_COMPUTE_READ);

        // source: SRV of buffer to bind
        const uint32_t resourceIndex = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].internalIndex;

        // where to bind it
        const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo     = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = currentSrvResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        bufferDescriptorInfos[bufferDescriptorIndex]        = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range  = VK_WHOLE_SIZE;
    }

    // update uniform buffers
    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex)
    {
        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding = job->computeJobDescriptor.pipeline.constantBufferBindings[currentRootConstantIndex].slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        // the ubo ring buffer is pre-populated with VkBuffer objects of FFX_BUFFER_SIZE-bytes to prevent creating buffers at runtime
        uint32_t dataSize = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t);
        FFX_ASSERT(dataSize <= FFX_MAX_CONST_SIZE * sizeof(uint32_t));

        BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[backendContext->ringBufferBase];

        bufferDescriptorInfos[bufferDescriptorIndex].buffer = uBuffer.bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range = dataSize;

        if (job->computeJobDescriptor.cbs[currentRootConstantIndex].data)
        {
            memcpy(uBuffer.pData, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, dataSize);

            // flush mapped range if memory type is not coherent
            if ((backendContext->ringBufferMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange;
                memset(&memoryRange, 0, sizeof(memoryRange));

                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendContext->ringBufferMemory;
                memoryRange.offset = FFX_BUFFER_SIZE * backendContext->ringBufferBase;
                memoryRange.size = dataSize;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }
        }

        // Increment the base pointer in the ring buffer
        ++backendContext->ringBufferBase;
        if (backendContext->ringBufferBase >= FFX_RING_BUFFER_SIZE * s_MaxEffectContexts)
            backendContext->ringBufferBase = 0;
    }

    // If we are dispatching indirectly, transition the argument resource to indirect argument
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.cmdArgument, FFX_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    // insert all the barriers
    flushBarriers(backendContext, vkCommandBuffer);

    // update all uavs and srvs
    backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, descriptorWriteIndex, writeDescriptorSets, 0, nullptr);

    // bind pipeline
    backendContext->vkFunctionTable.vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, reinterpret_cast<VkPipeline>(job->computeJobDescriptor.pipeline.pipeline));

    // bind descriptor sets
    backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, 0, 1, &pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex], 0, nullptr);

    // Dispatch (or dispatch indirect)
    if (job->computeJobDescriptor.pipeline.cmdSignature)
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
    if (pipelineLayout->descriptorSetIndex >= FFX_MAX_QUEUED_FRAMES)
        pipelineLayout->descriptorSetIndex = 0;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::Resource ffxResourceSrc = backendContext->pResources[job->copyJobDescriptor.src.internalIndex];
    BackendContext_VK::Resource ffxResourceDst = backendContext->pResources[job->copyJobDescriptor.dst.internalIndex];

    addBarrier(backendContext, &job->copyJobDescriptor.src, FFX_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFX_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, vkCommandBuffer);

    if (ffxResourceSrc.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && ffxResourceDst.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxResourceSrc.bufferResource;
        VkBuffer vkResourceDst = ffxResourceDst.bufferResource;

        VkBufferCopy bufferCopy = {};

        bufferCopy.dstOffset = 0;
        bufferCopy.srcOffset = 0;
        bufferCopy.size = ffxResourceSrc.resourceDescription.width;

        backendContext->vkFunctionTable.vkCmdCopyBuffer(vkCommandBuffer, vkResourceSrc, vkResourceDst, 1, &bufferCopy);
    }
    else if (ffxResourceSrc.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && ffxResourceDst.resourceDescription.type != FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxResourceSrc.bufferResource;
        VkImage vkResourceDst = ffxResourceDst.imageResource;

        VkImageSubresourceLayers subresourceLayers = {};

        subresourceLayers.aspectMask = ((ffxResourceDst.resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) != 0) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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
        bool isSrcDepth = ((ffxResourceSrc.resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) != 0);
        bool isDstDepth = ((ffxResourceDst.resourceDescription.usage & FFX_RESOURCE_USAGE_DEPTHTARGET) != 0);
        FFX_ASSERT_MESSAGE(isSrcDepth == isDstDepth, "Copy operations aren't allowed between depth and color textures in the vulkan backend of the FFX SDK.");

        #define FFX_MAX_IMAGE_COPY_MIPS 14 // Will handle 4k down to 1x1
        VkImageCopy             imageCopies[FFX_MAX_IMAGE_COPY_MIPS];
        VkImage vkResourceSrc = ffxResourceSrc.imageResource;
        VkImage vkResourceDst = ffxResourceDst.imageResource;

        uint32_t numMipsToCopy = FFX_MINIMUM(ffxResourceSrc.resourceDescription.mipCount, ffxResourceDst.resourceDescription.mipCount);

        for (uint32_t mip = 0; mip < numMipsToCopy; mip++)
        {
            VkImageSubresourceLayers srcSubresourceLayers = {};

            srcSubresourceLayers.aspectMask     = isSrcDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            srcSubresourceLayers.baseArrayLayer = 0;
            srcSubresourceLayers.layerCount     = 1;
            srcSubresourceLayers.mipLevel       = mip;

            VkImageSubresourceLayers dstSubresourceLayers = {};

            dstSubresourceLayers.aspectMask     = isDstDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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

static FfxErrorCode executeGpuJobClearFloat(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_VK::Resource ffxResource = backendContext->pResources[idx];

    if (ffxResource.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkBuffer vkResource = ffxResource.bufferResource;

        backendContext->vkFunctionTable.vkCmdFillBuffer(vkCommandBuffer, vkResource, 0, VK_WHOLE_SIZE, (uint32_t)job->clearJobDescriptor.color[0]);
    }
    else
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_COPY_DEST);
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
        range.layerCount = ffxResource.resourceDescription.depth;

        backendContext->vkFunctionTable.vkCmdClearColorImage(vkCommandBuffer, vkResource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &range);
    }

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsVK(FfxInterface* backendInterface, FfxCommandList commandList)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxErrorCode errorCode = FFX_OK;

    // execute all renderjobs
    for (uint32_t i = 0; i < backendContext->gpuJobCount; ++i)
    {
        FfxGpuJobDescription* gpuJob = &backendContext->pGpuJobs[i];
        VkCommandBuffer vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList);

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
            errorCode = executeGpuJobCompute(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        default:;
        }
    }

    // check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(
        errorCode == FFX_OK,
        FFX_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFX_OK;
}
