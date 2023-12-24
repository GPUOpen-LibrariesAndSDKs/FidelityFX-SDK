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
#include <ffx_shader_blobs.h>

#include "ffx_cauldron.h"

#include "core/framework.h"
#include "core/taskmanager.h"
#include "core/loaders/textureloader.h"
#include "render/device.h"
#include "render/texture.h"
#include "render/copyresource.h"
#include "render/dynamicresourcepool.h"
#include "render/indirectworkload.h"
#include "render/rootsignature.h"
#include "render/pipelineobject.h"
#include "render/parameterset.h"
#include "render/resourceviewallocator.h"
#include "render/uploadheap.h"

// To avoid having to always append it
using namespace::cauldron;

#if _VK
#include "render/vk/pipelinedesc_vk.h"  // to avoid double defining the binding shifts
#else // _VK
// Redefine offsets for compilation purposes
#define BINDING_SHIFT(name, shift)                       \
constexpr uint32_t name##_BINDING_SHIFT     = shift; \
constexpr wchar_t* name##_BINDING_SHIFT_STR = L#shift;

// put it there for now
BINDING_SHIFT(TEXTURE, 0);
BINDING_SHIFT(SAMPLER, 0);
BINDING_SHIFT(UNORDERED_ACCESS_VIEW, 0);
BINDING_SHIFT(CONSTANT_BUFFER, 0);
#endif // _VK

#if _DX12
#include "render/dx12/device_dx12.h"
#endif  // _DX12

// Cauldron prototypes for functions in the backend interface
FfxUInt32              GetSDKVersionCauldron(FfxInterface* backendInterface);
FfxErrorCode           CreateBackendContextCauldron(FfxInterface* backendInterface, FfxUInt32* effectContextId);
FfxErrorCode           GetDeviceCapabilitiesCauldron(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode           DestroyBackendContextCauldron(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode           CreateResourceCauldron(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode           DestroyResourceCauldron(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId);
FfxErrorCode           RegisterResourceCauldron(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxResource            GetResourceCauldron(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           UnregisterResourcesCauldron(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxResourceDescription GetResourceDescriptionCauldron(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           CreatePipelineCauldron(FfxInterface* backendInterface, FfxEffect effect, FfxPass passId, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);
FfxErrorCode           DestroyPipelineCauldron(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode           ScheduleGpuJobCauldron(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode           ExecuteGpuJobsCauldron(FfxInterface* backendInterface, FfxCommandList commandList);

// To track parallel effect context usage
static uint32_t s_BackendRefCount = 0;
static uint32_t s_FrameContextUpdateCount = 0;
static uint32_t s_MaxEffectContexts = 0;

typedef struct BackendContext_Cauldron
{
    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        wchar_t resourceName[64] = {};
#endif
        const GPUResource*     resourcePtr;
        MemTextureDataBlock*   uploadResourcePtr;

        FfxResourceDescription resourceDescription;
        FfxResourceStates      initialState;
        FfxResourceStates      currentState;
        int32_t                srvDescIndex;
        int32_t                uavDescIndex;
        int32_t                uavDescCount;
    } Resource;

    FfxGpuJobDescription* pGpuJobs;
    uint32_t              gpuJobCount;

    // Resource views
    ResourceViewAllocator* pResourceViewAllocator;
    ResourceView*          pGpuResourceViews;       // Base resource GPU views for clears
    ResourceView*          pCpuResourceViews;       // Base resource CPU views for clears
    ResourceView*          pRingBufferViews;        // Resource views for execution binding

    // Ring buffer params
    uint32_t              descRingBufferSize;
    uint32_t              descRingBufferBase;

    // Barriers
    Barrier               barriers[FFX_MAX_BARRIERS];
    uint32_t              barrierCount;

    // Additional information
    uint32_t              currentFrame;

    typedef struct alignas(32) EffectContext {

        // Resource allocation
        uint32_t              nextStaticResource;
        uint32_t              nextDynamicResource;

        // UAV offsets
        uint32_t              nextStaticUavDescriptor;
        uint32_t              nextDynamicUavDescriptor;

        // Usage
        bool                  active;

    } EffectContext;

    // Resource holder
    Resource*                 pResources;
    EffectContext*            pEffectContexts;

} BackendContext_Cauldron;

typedef struct PipelineData final
{
    PipelineObject*            pipelineObject;
    std::vector<ParameterSet*> parameterSets[FFX_MAX_QUEUED_FRAMES];
    uint32_t                   lastFrame;
    uint32_t                   nextParameterSetIndex;

    PipelineData()
        : pipelineObject(nullptr)
        , lastFrame(FFX_MAX_QUEUED_FRAMES)
        , nextParameterSetIndex(0)
    {
    }

    ~PipelineData()
    {
        delete pipelineObject;
        for (uint32_t i = 0; i < FFX_MAX_QUEUED_FRAMES; ++i)
        {
            for (auto it = parameterSets[i].begin(); it != parameterSets[i].end(); ++it)
                delete *it;
            parameterSets[i].clear();
        }
    }

} PipelineObjectSet;

FFX_API size_t ffxGetScratchMemorySizeCauldron(size_t maxContexts)
{
    uint32_t resourceArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_Cauldron::Resource), sizeof(uint64_t));
    uint32_t contextArraySize = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_Cauldron::EffectContext), sizeof(uint32_t));
    uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
    return FFX_ALIGN_UP(sizeof(BackendContext_Cauldron) + resourceArraySize + contextArraySize + gpuJobDescArraySize, sizeof(uint64_t));
}

FfxDevice ffxGetDeviceCauldron(Device* cauldronDevice)
{
    FFX_ASSERT(nullptr != cauldronDevice);
    return reinterpret_cast<FfxDevice>(cauldronDevice);
}

FfxCommandQueue ffxGetCommandQueueCauldron(cauldron::CommandQueue cmdQueue)
{
#if _DX12
    ID3D12CommandQueue* pGameQueue = GetDevice()->GetImpl()->DX12CmdQueue(CommandQueue::Graphics);
    return reinterpret_cast<FfxCommandQueue>(pGameQueue);
#else
    FFX_ASSERT(false && "Not implemented!");
    return NULL;
#endif
}

// Populate interface with Cauldron pointers.
FfxErrorCode ffxGetInterfaceCauldron(FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    size_t maxContexts)
{
    FFX_RETURN_ON_ERROR(!s_BackendRefCount, FFX_ERROR_BACKEND_API_ERROR);
    FFX_RETURN_ON_ERROR(backendInterface, FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(scratchBuffer, FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(scratchBufferSize >= ffxGetScratchMemorySizeCauldron(maxContexts), FFX_ERROR_INSUFFICIENT_MEMORY);

    // Function callback assignments
    backendInterface->fpGetSDKVersion = GetSDKVersionCauldron;
    backendInterface->fpCreateBackendContext = CreateBackendContextCauldron;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesCauldron;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextCauldron;
    backendInterface->fpCreateResource = CreateResourceCauldron;
    backendInterface->fpDestroyResource = DestroyResourceCauldron;
    backendInterface->fpRegisterResource = RegisterResourceCauldron;
    backendInterface->fpGetResource = GetResourceCauldron;
    backendInterface->fpUnregisterResources = UnregisterResourcesCauldron;
    backendInterface->fpGetResourceDescription = GetResourceDescriptionCauldron;
    backendInterface->fpCreatePipeline = CreatePipelineCauldron;
    backendInterface->fpDestroyPipeline = DestroyPipelineCauldron;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobCauldron;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsCauldron;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    // Set the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    s_MaxEffectContexts = (uint32_t)maxContexts;

    return FFX_OK;
}

FfxCommandList ffxGetCommandListCauldron(CommandList* cauldronCmdList)
{
    FFX_ASSERT(nullptr != cauldronCmdList);
    return reinterpret_cast<FfxCommandList>(cauldronCmdList);
}

FfxSwapchain ffxGetSwapchainCauldron(SwapChain* pSwapchain)
{
    FFX_ASSERT(nullptr != pSwapchain);
    return reinterpret_cast<FfxSwapchain>(pSwapchain);
}

FfxResource ffxGetResourceCauldron(const cauldron::GPUResource* cauldronResource,
    FfxResourceDescription       ffxResDescription,
    wchar_t* ffxResName,
    FfxResourceStates            state /*=FFX_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxResource resource = {};
    resource.resource = const_cast<void*>(reinterpret_cast<const void*>(cauldronResource));
    resource.state = state;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (ffxResName)
    {
        wcscpy_s(resource.name, ffxResName);
    }
#endif

    return resource;
}

//////////////////////////////////////////////////////////////////////////
// Local converters

ResourceFlags CauldronGetResourceFlagsFfx(FfxResourceUsage flags)
{
    ResourceFlags cauldronResourceFlags = ResourceFlags::None;

    if (flags & FFX_RESOURCE_USAGE_RENDERTARGET)
        cauldronResourceFlags |= ResourceFlags::AllowRenderTarget;

    if (flags & FFX_RESOURCE_USAGE_DEPTHTARGET)
        cauldronResourceFlags |= ResourceFlags::AllowDepthStencil;

    if (flags & FFX_RESOURCE_USAGE_UAV)
        cauldronResourceFlags |= ResourceFlags::AllowUnorderedAccess;

    if (flags & FFX_RESOURCE_USAGE_INDIRECT)
        cauldronResourceFlags |= ResourceFlags::AllowIndirect;

    return cauldronResourceFlags;
}

FfxResourceUsage FfxGetResourceFlagsCauldron(ResourceFlags flags)
{
    uint32_t ffxResourceFlags      = FFX_RESOURCE_USAGE_READ_ONLY;

    if (static_cast<bool>(flags & ResourceFlags::AllowRenderTarget))
        ffxResourceFlags |= FFX_RESOURCE_USAGE_RENDERTARGET;

    if (static_cast<bool>(flags & ResourceFlags::AllowDepthStencil))
        ffxResourceFlags |= FFX_RESOURCE_USAGE_DEPTHTARGET;

    if (static_cast<bool>(flags & ResourceFlags::AllowUnorderedAccess))
        ffxResourceFlags |= FFX_RESOURCE_USAGE_UAV;

    return static_cast<FfxResourceUsage>(ffxResourceFlags);
}

ResourceState CauldronGetStateFfx(FfxResourceStates state)
{
    switch (state)
    {
    case FFX_RESOURCE_STATE_UNORDERED_ACCESS:
        return ResourceState::UnorderedAccess;
        break;
    case FFX_RESOURCE_STATE_COMPUTE_READ:
        return ResourceState::NonPixelShaderResource;
        break;
    case FFX_RESOURCE_STATE_PIXEL_READ:
        return ResourceState::PixelShaderResource;
        break;
    case FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ:
        return ResourceState(ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
        break;
    case FFX_RESOURCE_STATE_COPY_SRC:
        return ResourceState::CopySource;
        break;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return ResourceState::CopyDest;
        break;
    case FFX_RESOURCE_STATE_GENERIC_READ:
        return (ResourceState)((uint32_t)ResourceState::NonPixelShaderResource | (uint32_t)ResourceState::PixelShaderResource);
        break;
    case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return ResourceState::IndirectArgument;
        break;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported resource state requested. Please implement.");
        return ResourceState::CommonResource;
        break;
    }
}

FfxResourceStates FfxGetStateCauldron(ResourceState state)
{
    switch (state)
    {
    case ResourceState::UnorderedAccess:
        return FFX_RESOURCE_STATE_UNORDERED_ACCESS;
        break;
    case ResourceState::NonPixelShaderResource:
        return FFX_RESOURCE_STATE_COMPUTE_READ;
        break;
    case ResourceState::CopySource:
        return FFX_RESOURCE_STATE_COPY_SRC;
        break;
    case ResourceState::CopyDest:
        return FFX_RESOURCE_STATE_COPY_DEST;
        break;
    case (ResourceState)((uint32_t)ResourceState::NonPixelShaderResource | (uint32_t)ResourceState::PixelShaderResource):
        return FFX_RESOURCE_STATE_GENERIC_READ;
        break;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported resource state requested. Please implement.");
        return FFX_RESOURCE_STATE_GENERIC_READ;
        break;
    }
}

ResourceFormat CauldronGetSurfaceFormatFfx(FfxSurfaceFormat format)
{
    switch (format)
    {
    case FFX_SURFACE_FORMAT_UNKNOWN:
        return ResourceFormat::Unknown;
    case FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS:
        return ResourceFormat::RGBA32_TYPELESS;
    case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT:
        return ResourceFormat::RGBA32_FLOAT;
    case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT:
        return ResourceFormat::RGBA16_FLOAT;
    case FFX_SURFACE_FORMAT_R32G32_FLOAT:
        return ResourceFormat::RG32_FLOAT;
    case FFX_SURFACE_FORMAT_R32_UINT:
        return ResourceFormat::R32_UINT;
    case FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS:
        return ResourceFormat::RGBA8_TYPELESS;
    case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM:
        return ResourceFormat::RGBA8_UNORM;
    case FFX_SURFACE_FORMAT_R8G8B8A8_SNORM:
        return ResourceFormat::RGBA8_SNORM;
    case FFX_SURFACE_FORMAT_R8G8B8A8_SRGB:
        return ResourceFormat::RGBA8_SRGB;
    case FFX_SURFACE_FORMAT_R11G11B10_FLOAT:
        return ResourceFormat::RG11B10_FLOAT;
    case FFX_SURFACE_FORMAT_R16G16_FLOAT:
        return ResourceFormat::RG16_FLOAT;
    case FFX_SURFACE_FORMAT_R16G16_UINT:
        return ResourceFormat::RG16_UINT;
    case FFX_SURFACE_FORMAT_R16_FLOAT:
        return ResourceFormat::R16_FLOAT;
    case FFX_SURFACE_FORMAT_R16_UINT:
        return ResourceFormat::R16_UINT;
    case FFX_SURFACE_FORMAT_R16_UNORM:
        return ResourceFormat::R16_UNORM;
    case FFX_SURFACE_FORMAT_R16_SNORM:
        return ResourceFormat::R16_SNORM;
    case FFX_SURFACE_FORMAT_R8_UNORM:
        return ResourceFormat::R8_UNORM;
    case FFX_SURFACE_FORMAT_R8G8_UNORM:
        return ResourceFormat::RG8_UNORM;
    case FFX_SURFACE_FORMAT_R32_FLOAT:
        return ResourceFormat::R32_FLOAT;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported format requested. Please implement.");
        return ResourceFormat::Unknown;
    }
}

FfxSurfaceFormat FfxGetSurfaceFormatCauldron(ResourceFormat format)
{
    switch (format)
    {
    case (ResourceFormat::RGBA32_TYPELESS):
        return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
    case (ResourceFormat::RGBA32_UINT):
        return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
    case (ResourceFormat::RGBA32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case (ResourceFormat::RGBA16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case (ResourceFormat::RG32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32_FLOAT;
    case (ResourceFormat::R8_UINT):
        return FFX_SURFACE_FORMAT_R8_UINT;
    case (ResourceFormat::R32_UINT):
        return FFX_SURFACE_FORMAT_R32_UINT;
    case (ResourceFormat::RGBA8_TYPELESS):
        return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
    case (ResourceFormat::RGBA8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (ResourceFormat::RGBA8_SNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_SNORM;
    case (ResourceFormat::RGBA8_SRGB):
        return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case (ResourceFormat::RG11B10_FLOAT):
        return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
    case (ResourceFormat::RG16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case (ResourceFormat::RG16_UINT):
        return FFX_SURFACE_FORMAT_R16G16_UINT;
    case (ResourceFormat::R16_FLOAT):
        return FFX_SURFACE_FORMAT_R16_FLOAT;
    case (ResourceFormat::R16_UINT):
        return FFX_SURFACE_FORMAT_R16_UINT;
    case (ResourceFormat::R16_UNORM):
        return FFX_SURFACE_FORMAT_R16_UNORM;
    case (ResourceFormat::R16_SNORM):
        return FFX_SURFACE_FORMAT_R16_SNORM;
    case (ResourceFormat::R8_UNORM):
        return FFX_SURFACE_FORMAT_R8_UNORM;
    case ResourceFormat::RG8_UNORM:
        return FFX_SURFACE_FORMAT_R8G8_UNORM;
    case ResourceFormat::R32_FLOAT:
    case ResourceFormat::D32_FLOAT:
        return FFX_SURFACE_FORMAT_R32_FLOAT;
    case (ResourceFormat::Unknown):
        return FFX_SURFACE_FORMAT_UNKNOWN;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported format requested. Please implement.");
        return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

const GPUResource* ffxGetCauldronResourcePtr(BackendContext_Cauldron* backendContext, uint32_t resId)
{
    FFX_ASSERT(nullptr != backendContext);
    return reinterpret_cast<const GPUResource*>(backendContext->pResources[resId].resourcePtr);
}

void addBarrier(BackendContext_Cauldron* backendContext, FfxResourceInternal* resource, FfxResourceStates newState)
{
    FFX_ASSERT(nullptr != backendContext);
    FFX_ASSERT(nullptr != resource);

    const GPUResource* cauldronResource = ffxGetCauldronResourcePtr(backendContext, resource->internalIndex);
    Barrier* barrier = &backendContext->barriers[backendContext->barrierCount];
    FFX_ASSERT(backendContext->barrierCount < FFX_MAX_BARRIERS);

    FfxResourceStates* currentState = &backendContext->pResources[resource->internalIndex].currentState;
    if ((*currentState & newState) != newState)
    {
        *barrier = Barrier::Transition(cauldronResource, CauldronGetStateFfx(*currentState), CauldronGetStateFfx(newState));
        *currentState = newState;
        ++backendContext->barrierCount;
    }
    // By the time a resource gets here, its current state will already FFX_RESOURCE_STATE_UNORDERED_ACCESS
    else if (newState == FFX_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        *barrier = Barrier::UAV(cauldronResource);
        ++backendContext->barrierCount;
    }
}

void flushBarriers(BackendContext_Cauldron* backendContext, CommandList* cauldronCommandList)
{
    FFX_ASSERT(nullptr != backendContext);
    FFX_ASSERT(nullptr != cauldronCommandList);

    if (backendContext->barrierCount > 0)
    {
        ResourceBarrier(cauldronCommandList, backendContext->barrierCount, backendContext->barriers);
        backendContext->barrierCount = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// Cauldron back end implementation

FfxUInt32 GetSDKVersionCauldron(FfxInterface* backendInterface)
{
    return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

FfxErrorCode CreateBackendContextCauldron(FfxInterface* backendInterface, FfxUInt32* effectContextId)
{
    FFX_ASSERT(nullptr != backendInterface);
    FFX_ASSERT(nullptr != backendInterface->device);

    Device* cauldronDevice = reinterpret_cast<Device*>(backendInterface->device);
    FFX_ASSERT_MESSAGE(cauldronDevice == GetDevice(), "FFXInterface: Cauldron: Unknown device passed to CreateBackendContextCauldron.");

    // Set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!s_BackendRefCount) {

        // Clear everything out
        memset(backendContext, 0, sizeof(*backendContext));

        // Reset
        s_FrameContextUpdateCount = 0;

        // Map all of our pointers
        uint32_t resourceArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_Cauldron::Resource), sizeof(uint64_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * sizeof(BackendContext_Cauldron::EffectContext), sizeof(uint32_t));
        uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint8_t* pMem = (uint8_t*)((BackendContext_Cauldron*)(backendContext + 1));

        // Map the resources
        backendContext->pResources = (BackendContext_Cauldron::Resource*)(pMem);
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Map the effect contexts
        backendContext->pEffectContexts = reinterpret_cast<BackendContext_Cauldron::EffectContext*>(pMem);
        memset(backendContext->pEffectContexts, 0, contextArraySize);
        pMem += contextArraySize;

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);

        // Setup the ring buffer and resource views
        backendContext->pResourceViewAllocator = ResourceViewAllocator::CreateResourceViewAllocator();   // Will use cauldron defaults
        backendContext->pResourceViewAllocator->AllocateGPUResourceViews(&backendContext->pRingBufferViews, FFX_RING_BUFFER_SIZE * s_MaxEffectContexts);
        backendContext->pResourceViewAllocator->AllocateGPUResourceViews(&backendContext->pGpuResourceViews, FFX_MAX_RESOURCE_COUNT * s_MaxEffectContexts);
        backendContext->pResourceViewAllocator->AllocateCPUResourceViews(&backendContext->pCpuResourceViews, FFX_MAX_RESOURCE_COUNT * s_MaxEffectContexts);
        backendContext->descRingBufferSize = FFX_RING_BUFFER_SIZE * s_MaxEffectContexts;
        backendContext->descRingBufferBase = 0;

        // Reset current frame
        backendContext->currentFrame = 0;
    }

    // Increment the ref count
    ++s_BackendRefCount;

    // Get an available context id
    for (uint32_t i = 0; i < s_MaxEffectContexts; ++i) {
        if (!backendContext->pEffectContexts[i].active) {
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_Cauldron::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.nextStaticResource = (i * FFX_MAX_RESOURCE_COUNT) + 1;
            effectContext.nextDynamicResource = (i * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
            effectContext.nextStaticUavDescriptor = (i * FFX_MAX_RESOURCE_COUNT);
            effectContext.nextDynamicUavDescriptor = (i * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
            break;
        }
    }

    return FFX_OK;
}

FfxErrorCode GetDeviceCapabilitiesCauldron(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
    ShaderModel maxSupportedModel = GetDevice()->MaxSupportedShaderModel();
    switch (maxSupportedModel)
    {
    case cauldron::ShaderModel::SM5_1:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
        break;
    case cauldron::ShaderModel::SM6_0:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
        break;
    case cauldron::ShaderModel::SM6_1:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_1;
        break;
    case cauldron::ShaderModel::SM6_2:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_2;
        break;
    case cauldron::ShaderModel::SM6_3:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_3;
        break;
    case cauldron::ShaderModel::SM6_4:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_4;
        break;
    case cauldron::ShaderModel::SM6_5:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_5;
        break;
    case cauldron::ShaderModel::SM6_6:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
        break;
    case cauldron::ShaderModel::SM6_7:
    default:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_7;
        break;
    }

    // Get wave capabilities
    deviceCapabilities->waveLaneCountMax = GetDevice()->GetMaxWaveLaneCount();
    deviceCapabilities->waveLaneCountMin = GetDevice()->GetMinWaveLaneCount();

    // Get FP16 support
    deviceCapabilities->fp16Supported = GetDevice()->FeatureSupported(DeviceFeature::FP16);

    // Check RayTracing support
    deviceCapabilities->raytracingSupported = GetDevice()->FeatureSupported(DeviceFeature::RT_1_0);

    return FFX_OK;
}

FfxErrorCode DestroyBackendContextCauldron(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    // Delete any resources allocated by this context
    BackendContext_Cauldron::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (uint32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex)
    {
        if (backendContext->pResources[currentStaticResourceIndex].resourcePtr)
        {
            // All resources should have been cleaned up through individual destroy resource calls, but also have destruction here in case something was missed
            CauldronWarning(L"FFXInterface: Cauldron: SDK Resource %ls was not destroyed prior to destroying the backend context. There may be a resource leak.",
                backendContext->pResources[currentStaticResourceIndex].resourcePtr->GetName());
            FfxResourceInternal internalResource = { (int32_t)currentStaticResourceIndex };
            DestroyResourceCauldron(backendInterface, internalResource, effectContextId);
        }
    }
    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --s_BackendRefCount;

    if (!s_BackendRefCount) {

        // Reset resource view pools
        delete backendContext->pRingBufferViews;
        backendContext->pRingBufferViews = nullptr;
        delete backendContext->pGpuResourceViews;
        backendContext->pGpuResourceViews = nullptr;
        delete backendContext->pCpuResourceViews;
        backendContext->pCpuResourceViews = nullptr;
        delete backendContext->pResourceViewAllocator;
        backendContext->pResourceViewAllocator = nullptr;

        // Reset the frame index
        backendContext->currentFrame = 0;
    }

    return FFX_OK;
}

FfxErrorCode CreateResourceCauldron(FfxInterface* backendInterface, const FfxCreateResourceDescription* createResourceDescription,
    FfxUInt32 effectContextId, FfxResourceInternal* outTexture)
{
    FFX_ASSERT(nullptr != backendInterface);
    FFX_ASSERT(nullptr != createResourceDescription);
    FFX_ASSERT(nullptr != outTexture);

    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;
    BackendContext_Cauldron::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Assign to an internal resource
    FFX_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);
    outTexture->internalIndex                           = effectContext.nextStaticResource++;
    BackendContext_Cauldron::Resource* backendResource  = &backendContext->pResources[outTexture->internalIndex];
    backendResource->resourceDescription                = createResourceDescription->resourceDescription;

    // If not an upload resource, go through full resource creation
    const FfxResourceStates       resourceState          = (createResourceDescription->initData && (createResourceDescription->heapType != FFX_HEAP_TYPE_UPLOAD)) ? FFX_RESOURCE_STATE_COPY_DEST : createResourceDescription->initalState;
    const ResourceState           cauldronResourceState  = CauldronGetStateFfx(resourceState);
    const FfxResourceDescription  resDesc                = createResourceDescription->resourceDescription;

    ViewDimension                 viewDimension;
    const Texture*                pTexture;
    const Buffer*                 pBuffer;

    if (resDesc.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        BufferDesc bufferDesc = BufferDesc::Data(createResourceDescription->name, resDesc.width, resDesc.height, resDesc.alignment, CauldronGetResourceFlagsFfx(backendResource->resourceDescription.usage));
        viewDimension         = ViewDimension::Buffer;

        // Create and map the underlying GPUResource
        pBuffer = GetDynamicResourcePool()->CreateBuffer(&bufferDesc, cauldronResourceState);
        backendResource->resourcePtr = pBuffer->GetResource();
    }
    else
    {
        TextureDesc textureDesc;
        switch (resDesc.type)
        {
        case FFX_RESOURCE_TYPE_TEXTURE1D:
            textureDesc = TextureDesc::Tex1D(createResourceDescription->name,
                                                CauldronGetSurfaceFormatFfx(resDesc.format),
                                                resDesc.width,
                                                resDesc.depth,
                                                resDesc.mipCount,
                                                CauldronGetResourceFlagsFfx(backendResource->resourceDescription.usage));
            viewDimension = textureDesc.DepthOrArraySize > 1 ? ViewDimension::Texture1DArray : ViewDimension::Texture1D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            textureDesc = TextureDesc::Tex2D(createResourceDescription->name,
                                                CauldronGetSurfaceFormatFfx(resDesc.format),
                                                resDesc.width,
                                                resDesc.height,
                                                resDesc.depth,
                                                resDesc.mipCount,
                                                CauldronGetResourceFlagsFfx(backendResource->resourceDescription.usage));
            viewDimension = textureDesc.DepthOrArraySize > 1 ? ViewDimension::Texture2DArray : ViewDimension::Texture2D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
            textureDesc = TextureDesc::TexCube(createResourceDescription->name,
                                                CauldronGetSurfaceFormatFfx(resDesc.format),
                                                resDesc.width,
                                                resDesc.height,
                                                resDesc.depth,
                                                resDesc.mipCount,
                                                CauldronGetResourceFlagsFfx(backendResource->resourceDescription.usage));
            viewDimension = ViewDimension::TextureCube;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE3D:
            textureDesc = TextureDesc::Tex3D(createResourceDescription->name,
                                                CauldronGetSurfaceFormatFfx(resDesc.format),
                                                resDesc.width,
                                                resDesc.height,
                                                resDesc.depth,
                                                resDesc.mipCount,
                                                CauldronGetResourceFlagsFfx(backendResource->resourceDescription.usage));
            viewDimension = ViewDimension::Texture3D;
            break;
        default:
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported resource type requested. Please implement.");
        }

        // Create and map the underlying GPUResource (Note that because this is an independent backend, we cannot auto-handle the resource resizing as we normally do.
        // Cauldron will call a special rendermodule callback for context destruction/creation on resize)
        pTexture = GetDynamicResourcePool()->CreateTexture(&textureDesc, cauldronResourceState);
        backendResource->resourcePtr = pTexture->GetResource();
        backendResource->initialState = resourceState;
        backendResource->currentState = resourceState;
    }

    backendResource->initialState = resourceState;
    backendResource->currentState = resourceState;

    // Copy the name over in debug
#ifdef _DEBUG
    wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif

    // Create SRVs and UAVs for the resource
    if (viewDimension == ViewDimension::Buffer)
    {
        const BufferDesc& bufDesc = pBuffer->GetDesc();

        if (static_cast<bool>(bufDesc.Flags & ResourceFlags::AllowUnorderedAccess))
        {
            // Check and assign uav index
            FFX_ASSERT(effectContext.nextStaticUavDescriptor + 1 < effectContext.nextDynamicResource);
            backendResource->uavDescCount = 1;
            backendResource->uavDescIndex = effectContext.nextStaticUavDescriptor;

            // Buffer mappings are 1:1
            backendContext->pGpuResourceViews->BindBufferResource(backendResource->resourcePtr, pBuffer->GetDesc(), ResourceViewType::BufferUAV, -1, -1, backendResource->uavDescIndex);
            backendContext->pCpuResourceViews->BindBufferResource(backendResource->resourcePtr, pBuffer->GetDesc(), ResourceViewType::BufferUAV, -1, -1, backendResource->uavDescIndex);

            // Update the next uav index pointer
            ++effectContext.nextStaticUavDescriptor;
        }
    }
    else
    {
        const TextureDesc& texDesc = pTexture->GetDesc();

        // Base texture SRV
        // SRV will be mapped dynamically when executing

        // Map UAVs for each mip
        if (static_cast<bool>(texDesc.Flags & ResourceFlags::AllowUnorderedAccess))
        {
            const int32_t uavDescriptorCount = static_cast<int32_t>(texDesc.MipLevels);

            // Check and assign index to start at
            FFX_ASSERT(effectContext.nextStaticUavDescriptor + uavDescriptorCount < effectContext.nextDynamicResource);
            backendResource->uavDescCount = uavDescriptorCount;
            backendResource->uavDescIndex = effectContext.nextStaticUavDescriptor;

            for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorCount; ++currentMipIndex)
            {
                backendContext->pGpuResourceViews->BindTextureResource(backendResource->resourcePtr, pTexture->GetDesc(), ResourceViewType::TextureUAV, viewDimension, currentMipIndex, -1, -1, backendResource->uavDescIndex + currentMipIndex);
                backendContext->pCpuResourceViews->BindTextureResource(backendResource->resourcePtr, pTexture->GetDesc(), ResourceViewType::TextureUAV, viewDimension, currentMipIndex, -1, -1, backendResource->uavDescIndex + currentMipIndex);
            }

            // Update the next uav index pointer
            effectContext.nextStaticUavDescriptor += uavDescriptorCount;
        }
    }

    // Create upload resource and upload job
    if (createResourceDescription->initData)
    {
        CopyResource* pCopyResource = CopyResource::CreateCopyResource(backendResource->resourcePtr, createResourceDescription->initData, createResourceDescription->initDataSize, CauldronGetStateFfx(FFX_RESOURCE_STATE_GENERIC_READ));

        // Assign to an internal resource
        FFX_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);
        FfxResourceInternal copySrc;
        copySrc.internalIndex                                  = effectContext.nextStaticResource++;
        BackendContext_Cauldron::Resource* backendCopyResource = &backendContext->pResources[copySrc.internalIndex];
        backendCopyResource->resourceDescription               = resDesc;
        backendCopyResource->initialState                      = FFX_RESOURCE_STATE_GENERIC_READ;
        backendCopyResource->currentState                      = FFX_RESOURCE_STATE_GENERIC_READ;
        backendCopyResource->resourcePtr                       = pCopyResource->GetResource();

#ifdef _DEBUG
        wcscpy_s(backendCopyResource->resourceName, pCopyResource->GetResource()->GetName());
#endif

        // Setup the upload job
        FfxGpuJobDescription copyJob = { FFX_GPU_JOB_COPY };
        copyJob.copyJobDescriptor.src = copySrc;
        copyJob.copyJobDescriptor.dst = *outTexture;

        backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceCauldron(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId)
{
    // Since SDK needs to be able to destroy resources when resizing, need to explicitly allow resource destruction (which is normally handled automatically by cauldron)
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;
    BackendContext_Cauldron::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    for (uint32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT;
         currentStaticResourceIndex < (uint32_t)effectContext.nextStaticResource;
         ++currentStaticResourceIndex)
    {
        BackendContext_Cauldron::Resource* backendResource = &backendContext->pResources[resource.internalIndex];
        if (backendResource->resourcePtr)
        {
            if (backendResource->resourcePtr->IsCopyBuffer())
                delete backendResource->resourcePtr;
            else
                GetDynamicResourcePool()->DestroyResource(backendResource->resourcePtr);

            backendResource->resourcePtr = nullptr;
        }
    }

    return FFX_OK;
}

FfxErrorCode RegisterResourceCauldron(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;
    BackendContext_Cauldron::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Get the cauldron resource
    const GPUResource* cauldronResource = reinterpret_cast<GPUResource*>(inResource->resource);

    FfxResourceStates state                  = inResource->state;

    // If no resource was set, point it to a null entry
    if (cauldronResource == nullptr)
    {
        outResourceInternal->internalIndex = 0; // Always maps to FFX_<feature>_RESOURCE_IDENTIFIER_NULL;
        return FFX_OK;
    }

    FFX_ASSERT(effectContext.nextDynamicResource > effectContext.nextStaticResource);
    outResourceInternal->internalIndex = effectContext.nextDynamicResource--;

    // Setup the backend resource
    BackendContext_Cauldron::Resource* backendResource = &backendContext->pResources[outResourceInternal->internalIndex];
    backendResource->resourcePtr                       = cauldronResource;
    backendResource->initialState                      = state;
    backendResource->currentState                      = state;

#ifdef _DEBUG
    const wchar_t* name = inResource->name;
    if (name)
    {
        wcscpy_s(backendResource->resourceName, name);
    }
#endif

    if (cauldronResource->IsBuffer())
    {
        const Buffer*     pBuffer = cauldronResource->GetBufferResource();
        const BufferDesc& bufDesc = pBuffer->GetDesc();

        // Setup the FfxResourceDescription
        backendResource->resourceDescription.type      = FFX_RESOURCE_TYPE_BUFFER;
        backendResource->resourceDescription.format    = FfxGetSurfaceFormatCauldron(ResourceFormat::Unknown);
        backendResource->resourceDescription.size      = bufDesc.Size;
        backendResource->resourceDescription.stride    = bufDesc.Stride;
        backendResource->resourceDescription.alignment = bufDesc.Alignment;
        backendResource->resourceDescription.flags     = FFX_RESOURCE_FLAGS_NONE;   // Change in the future if we need to support non-aliasing resources

        // Resource mappings are handled dynamically
    }
    else
    {
        const Texture*     pTexture = cauldronResource->GetTextureResource();
        const TextureDesc& texDesc  = pTexture->GetDesc();

        bool requestArrayView = FFX_CONTAINS_FLAG(inResource->description.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        // Setup the FfxResourceDescription
        switch (texDesc.Dimension)
        {
        case TextureDimension::Texture1D:
            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            break;
        case TextureDimension::Texture2D:
            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            break;
        case TextureDimension::CubeMap:
            if (requestArrayView)
                backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            else
                backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE_CUBE;
            break;
        case TextureDimension::Texture3D:
            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            break;
        default:
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unhandled view dimension requested. Please implement.");
            break;
        }

        backendResource->resourceDescription.format    = FfxGetSurfaceFormatCauldron(texDesc.Format);
        backendResource->resourceDescription.width     = texDesc.Width;
        backendResource->resourceDescription.height    = texDesc.Height;
        backendResource->resourceDescription.depth     = texDesc.DepthOrArraySize;
        backendResource->resourceDescription.mipCount  = texDesc.MipLevels;
        backendResource->resourceDescription.flags     = FFX_RESOURCE_FLAGS_NONE;  // Change in the future if we need to support non-aliasing resources
        backendResource->resourceDescription.usage     = inResource->description.usage;

        // Create resource views

        // SRV will be mapped dynamically

        // Store UAV offsets for mips (will be mapped dynamically)
        if (static_cast<bool>(texDesc.Flags & ResourceFlags::AllowUnorderedAccess))
        {
            const uint32_t uavDescriptorCount = texDesc.MipLevels;

            // Check and assign index to start at
            FFX_ASSERT(effectContext.nextDynamicUavDescriptor - uavDescriptorCount + 1 > effectContext.nextStaticResource);
            backendResource->uavDescCount = uavDescriptorCount;
            backendResource->uavDescIndex = effectContext.nextDynamicUavDescriptor - uavDescriptorCount + 1;

            // Update the next uav index pointer
            effectContext.nextDynamicUavDescriptor -= uavDescriptorCount;
        }
    }

    return FFX_OK;
}

FfxResource GetResourceCauldron(FfxInterface* backendInterface, FfxResourceInternal inResource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    FfxResourceDescription ffxResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxResource resource = {};
    resource.resource = const_cast<void*>(reinterpret_cast<const void*>(reinterpret_cast<const GPUResource*>(backendContext->pResources[inResource.internalIndex].resourcePtr)));
    resource.state = backendContext->pResources[inResource.internalIndex].currentState;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (backendContext->pResources[inResource.internalIndex].resourceName)
    {
        wcscpy_s(resource.name, backendContext->pResources[inResource.internalIndex].resourceName);
    }
#endif

    return resource;
}

FfxErrorCode UnregisterResourcesCauldron(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)(backendInterface->scratchBuffer);
    BackendContext_Cauldron::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Walk back all the resources that don't belong to us and reset them to their initial state
    for (uint32_t resourceIndex = ++effectContext.nextDynamicResource; resourceIndex < (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT; ++resourceIndex)
    {
        FfxResourceInternal internalResource;
        internalResource.internalIndex = resourceIndex;

        BackendContext_Cauldron::Resource* backendResource = &backendContext->pResources[resourceIndex];
        addBarrier(backendContext, &internalResource, backendResource->initialState);
    }

    FFX_ASSERT(nullptr != commandList);
    CommandList* pCmdList = reinterpret_cast<CommandList*>(commandList);

    flushBarriers(backendContext, pCmdList);

    effectContext.nextDynamicResource      = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
    effectContext.nextDynamicUavDescriptor = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;

    // Only cycle the current frame if all active contexts have finished the frame (calling this function)
    ++s_FrameContextUpdateCount;
    if (s_FrameContextUpdateCount == s_BackendRefCount)
    {
        s_FrameContextUpdateCount = 0;

        backendContext->currentFrame += 1;
        if (backendContext->currentFrame >= FFX_MAX_QUEUED_FRAMES)
            backendContext->currentFrame = 0;
    }

    return FFX_OK;
}

FfxResourceDescription GetResourceDescriptionCauldron(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    FfxResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

cauldron::AddressMode FfxGetAddressModeCauldron(const FfxAddressMode& addressMode)
{
    switch (addressMode)
    {
    case FFX_ADDRESS_MODE_WRAP:
        return cauldron::AddressMode::Wrap;
    case FFX_ADDRESS_MODE_MIRROR:
        return cauldron::AddressMode::Mirror;
    case FFX_ADDRESS_MODE_CLAMP:
        return cauldron::AddressMode::Clamp;
    case FFX_ADDRESS_MODE_BORDER:
        return cauldron::AddressMode::Border;
    case FFX_ADDRESS_MODE_MIRROR_ONCE:
        return cauldron::AddressMode::MirrorOnce;
    default:
        FFX_ASSERT_MESSAGE(false, "Unsupported addressing mode requested, defaulting to wrap. Please implement");
        return cauldron::AddressMode::Wrap;
        break;
    }
}

FfxErrorCode CreatePipelineCauldron(FfxInterface*                 backendInterface,
                                    FfxEffect                     effect,
                                    FfxPass                       passId,
                                    uint32_t                      permutationOptions,
                                    const FfxPipelineDescription* desc,
                                    FfxUInt32                     effectContextId,
                                    FfxPipelineState*             outPipeline)
{
    FFX_ASSERT(nullptr != backendInterface);
    FFX_ASSERT(nullptr != desc);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    FfxShaderBlob shaderBlob = { 0 };
    FfxErrorCode  errorCode  = ffxGetPermutationBlobByIndex(effect, passId, desc->stage, permutationOptions, &shaderBlob);
    FFX_ASSERT(errorCode == FFX_OK);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

    // Set up root signature
    // Easiest implementation: simply create one root signature per pipeline
    // Should add some management later on to avoid unnecessarily re-binding the root signature
    RootSignatureDesc rootSigDesc;

    // We currently only support compute shaders, but we need to add something to the pipeline description at some point to support other types
    ShaderBindStage bindStage = ShaderBindStage::Compute;

    // Add static samplers
    FFX_ASSERT(desc->samplerCount <= FFX_MAX_SAMPLERS);
    const size_t samplerCount = desc->samplerCount;
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
    {
        SamplerDesc samplerDesc;

        samplerDesc.Comparison = ComparisonFunc::Never;
        samplerDesc.MinLOD = 0.f;
        samplerDesc.MaxLOD = std::numeric_limits<float>::max();
        samplerDesc.MipLODBias = 0.f;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.AddressU = FfxGetAddressModeCauldron(desc->samplers[currentSamplerIndex].addressModeU);
        samplerDesc.AddressV = FfxGetAddressModeCauldron(desc->samplers[currentSamplerIndex].addressModeV);
        samplerDesc.AddressW = FfxGetAddressModeCauldron(desc->samplers[currentSamplerIndex].addressModeW);

        switch (desc->samplers[currentSamplerIndex].filter)
        {
        case FFX_FILTER_TYPE_MINMAGMIP_POINT:
            samplerDesc.Filter = FilterFunc::MinMagMipPoint;
            break;
        case FFX_FILTER_TYPE_MINMAGMIP_LINEAR:
            samplerDesc.Filter = FilterFunc::MinMagMipLinear;
            break;
        case FFX_FILTER_TYPE_MINMAGLINEARMIP_POINT:
            samplerDesc.Filter = FilterFunc::MinMagLinearMipPoint;
            break;

        default:
            FFX_ASSERT_MESSAGE(false, "Unsupported filter type requested. Please implement");
            break;
        }

        // NOTE: there is no need to offset sampler index as it is assumed to be from 0 to samplerCount-1
        rootSigDesc.AddStaticSamplers(currentSamplerIndex,
                                      bindStage,
                                      1,
                                      &samplerDesc);
    }

    // Bind SRVs and UAVs
    for (uint32_t srvEntry = 0; srvEntry < shaderBlob.srvTextureCount; ++srvEntry)
        rootSigDesc.AddTextureSRVSet(shaderBlob.boundSRVTextures[srvEntry] - TEXTURE_BINDING_SHIFT, bindStage, shaderBlob.boundSRVTextureCounts[srvEntry]);

    for (uint32_t uavEntry = 0; uavEntry < shaderBlob.uavTextureCount; ++uavEntry)
        rootSigDesc.AddTextureUAVSet(shaderBlob.boundUAVTextures[uavEntry] - UNORDERED_ACCESS_VIEW_BINDING_SHIFT, bindStage, shaderBlob.boundUAVTextureCounts[uavEntry]);

    for (uint32_t srvEntry = 0; srvEntry < shaderBlob.srvBufferCount; ++srvEntry)
        rootSigDesc.AddBufferSRVSet(shaderBlob.boundSRVBuffers[srvEntry] - TEXTURE_BINDING_SHIFT, bindStage, shaderBlob.boundSRVBufferCounts[srvEntry]);

    for (uint32_t uavEntry = 0; uavEntry < shaderBlob.uavBufferCount; ++uavEntry)
        rootSigDesc.AddBufferUAVSet(shaderBlob.boundUAVBuffers[uavEntry] - UNORDERED_ACCESS_VIEW_BINDING_SHIFT, bindStage, shaderBlob.boundUAVBufferCounts[uavEntry]);

    // Todo when needed: AddSamplerSets and RTAccelerationStructureSets

    // Add root constant buffers as 32 bit constant buffers
    // NOTE: there is no need to offset constant buffer index as it is assumed to be from 0 to rootConstantBufferCount-1
    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < desc->rootConstantBufferCount; currentRootConstantIndex++)
        rootSigDesc.AddConstantBufferView(currentRootConstantIndex, bindStage, 1);

    // Create and build the root signature
    RootSignature* pRootSignature = RootSignature::CreateRootSignature(L"FidelityFX SDK Root Signature", rootSigDesc);
    outPipeline->rootSignature = reinterpret_cast<FfxRootSignature>(pRootSignature);

    // Only set the command signature if this is setup as an indirect workload
    if (desc->indirectWorkload)
    {
        IndirectWorkload* pIndirectWorkload = IndirectWorkload::CreateIndirectWorkload(IndirectCommandType::Dispatch);
        outPipeline->cmdSignature = pIndirectWorkload;
    }
    else
    {
        outPipeline->cmdSignature = nullptr;
    }

    // Populate the pass.
    outPipeline->srvTextureCount = shaderBlob.srvTextureCount;
    outPipeline->uavTextureCount = shaderBlob.uavTextureCount;
    outPipeline->srvBufferCount  = shaderBlob.srvBufferCount;
    outPipeline->uavBufferCount  = shaderBlob.uavBufferCount;

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    outPipeline->constCount        = shaderBlob.cbvCount;

    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvTextureCount; ++srvIndex)
    {
        outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];
        wcscpy_s(outPipeline->srvTextureBindings[srvIndex].name, StringToWString(shaderBlob.boundSRVTextureNames[srvIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavTextureCount; ++uavIndex)
    {
        outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];
        wcscpy_s(outPipeline->uavTextureBindings[uavIndex].name, StringToWString(shaderBlob.boundUAVTextureNames[uavIndex]).c_str());
    }
    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvBufferCount; ++srvIndex)
    {
        outPipeline->srvBufferBindings[srvIndex].slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];
        wcscpy_s(outPipeline->srvBufferBindings[srvIndex].name, StringToWString(shaderBlob.boundSRVBufferNames[srvIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavBufferCount; ++uavIndex)
    {
        outPipeline->uavBufferBindings[uavIndex].slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];
        wcscpy_s(outPipeline->uavBufferBindings[uavIndex].name, StringToWString(shaderBlob.boundUAVBufferNames[uavIndex]).c_str());
    }
    for (uint32_t cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlob.boundConstantBufferCounts[cbIndex];
        wcscpy_s(outPipeline->constantBufferBindings[cbIndex].name, StringToWString(shaderBlob.boundConstantBufferNames[cbIndex]).c_str());
    }

    // Setup the pipeline object
    PipelineDesc psoDesc;
    psoDesc.SetRootSignature(pRootSignature);

    ShaderBlobDesc blobDesc = ShaderBlobDesc::Compute(shaderBlob.data, shaderBlob.size);
    psoDesc.AddShaderBlobDesc(blobDesc);

    bool isWave64 = false;
    ffxIsWave64(effect, permutationOptions, isWave64);
    psoDesc.SetWave64(isWave64);

    PipelineObject* pPipelineObj = PipelineObject::CreatePipelineObject(desc->name, psoDesc);

    // Setup the pipeline data and the parameter sets
    PipelineData* pPipelineData = new PipelineData();
    pPipelineData->pipelineObject = pPipelineObj;
    outPipeline->pipeline = reinterpret_cast<FfxPipeline>(pPipelineData);

    return FFX_OK;
}

FfxErrorCode DestroyPipelineCauldron(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId)
{
    // Destroy the pipeline
    delete reinterpret_cast<PipelineData*>(pipeline->pipeline);
    pipeline->pipeline = nullptr;

    // Destroy the cmd signature
    delete reinterpret_cast<IndirectWorkload*>(pipeline->cmdSignature);
    pipeline->cmdSignature = nullptr;

    // Destroy the root signature
    delete reinterpret_cast<RootSignature*>(pipeline->rootSignature);
    pipeline->rootSignature = nullptr;

    return FFX_OK;
}

FfxErrorCode ScheduleGpuJobCauldron(FfxInterface* backendInterface, const FfxGpuJobDescription* job)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    FFX_ASSERT(nullptr != job);
    FFX_ASSERT(backendContext->gpuJobCount < FFX_MAX_GPU_JOBS);

    backendContext->pGpuJobs[backendContext->gpuJobCount] = *job;

    if (job->jobType == FFX_GPU_JOB_COMPUTE)
    {
        // needs to copy SRVs and UAVs in case they are on the stack only
        FfxComputeJobDescription* computeJob      = &backendContext->pGpuJobs[backendContext->gpuJobCount].computeJobDescriptor;
        const uint32_t            numConstBuffers = job->computeJobDescriptor.pipeline.constCount;
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < numConstBuffers; ++currentRootConstantIndex)
        {
            computeJob->cbs[currentRootConstantIndex].num32BitEntries = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries;
            memcpy(computeJob->cbs[currentRootConstantIndex].data,
                   job->computeJobDescriptor.cbs[currentRootConstantIndex].data,
                   computeJob->cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));
        }
    }

    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_Cauldron* backendContext, FfxGpuJobDescription* job, CommandList* pCmdList)
{
    // Set all resource view heaps
    SetAllResourceViewHeaps(pCmdList, backendContext->pResourceViewAllocator);

    // Create a local parameter set to bind everything
    RootSignature*      cauldronRootSignature    = reinterpret_cast<RootSignature*>(job->computeJobDescriptor.pipeline.rootSignature);
    PipelineData*       cauldronPipelineData     = reinterpret_cast<PipelineData*>(job->computeJobDescriptor.pipeline.pipeline);
    IndirectWorkload*   cauldronIndirectWorkload = reinterpret_cast<IndirectWorkload*>(job->computeJobDescriptor.pipeline.cmdSignature);

    // Find the ParameterSet to use
    // reset parameter set index and update current frame
    if (cauldronPipelineData->lastFrame != backendContext->currentFrame)
    {
        cauldronPipelineData->lastFrame             = backendContext->currentFrame;
        cauldronPipelineData->nextParameterSetIndex = 0;
    }

    std::vector<ParameterSet*>& frameParameterSets = cauldronPipelineData->parameterSets[backendContext->currentFrame];
    // allocate a new ParameterSet if needed
    if (static_cast<uint32_t>(frameParameterSets.size()) <= cauldronPipelineData->nextParameterSetIndex)
        frameParameterSets.push_back(ParameterSet::CreateParameterSet(cauldronRootSignature, backendContext->pRingBufferViews));
    ParameterSet& paramSet = *frameParameterSets[cauldronPipelineData->nextParameterSetIndex];
    ++cauldronPipelineData->nextParameterSetIndex;

    // Bind Texture SRVs
    {
        uint32_t maximumSrvIndex = 0;
        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex)
        {
            const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex +
                                                     job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount - 1 -
                                                     TEXTURE_BINDING_SHIFT;
            maximumSrvIndex = currentSrvResourceIndex > maximumSrvIndex ? currentSrvResourceIndex : maximumSrvIndex;
        }

        // check if this fits into the ringbuffer, loop if not fitting
        if (backendContext->descRingBufferBase + maximumSrvIndex + 1 > FFX_RING_BUFFER_SIZE * s_MaxEffectContexts)
            backendContext->descRingBufferBase = 0;

        // Set the offset to use for binding
        paramSet.SetBindTypeOffset(BindingType::TextureSRV, backendContext->descRingBufferBase);
        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex)
        {
            for (uint32_t bindNum = 0; bindNum < job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount; ++bindNum)
            {
                uint32_t currPipelineSrvIndex = currentPipelineSrvIndex + bindNum;
                if (job->computeJobDescriptor.srvTextures[currPipelineSrvIndex].internalIndex == 0)
                    break;

                addBarrier(backendContext, &job->computeJobDescriptor.srvTextures[currPipelineSrvIndex], FFX_RESOURCE_STATE_COMPUTE_READ);

                // source: SRV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.srvTextures[currPipelineSrvIndex].internalIndex;

                // Where to bind it
                uint32_t currentSrvResourceIndex;
                if (bindNum >= 1)
                {
                    currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex + bindNum;
                }
                else
                {
                    currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currPipelineSrvIndex].slotIndex;
                }

                const BackendContext_Cauldron::Resource& resource = backendContext->pResources[resourceIndex];
                const Texture* pResource   = reinterpret_cast<const GPUResource*>(resource.resourcePtr)->GetTextureResource();
                const TextureDesc& texDesc = pResource->GetDesc();
                ViewDimension      viewDimension;

                bool requestArrayView = FFX_CONTAINS_FLAG(resource.resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

                switch (texDesc.Dimension)
                {
                case TextureDimension::Texture1D:
                    viewDimension = (texDesc.DepthOrArraySize > 1 || requestArrayView) ? ViewDimension::Texture1DArray : ViewDimension::Texture1D;
                    break;
                case TextureDimension::Texture2D:
                    viewDimension = (texDesc.DepthOrArraySize > 1 || requestArrayView) ? ViewDimension::Texture2DArray : ViewDimension::Texture2D;
                    break;
                case TextureDimension::CubeMap:
                    if (requestArrayView)
                        viewDimension = ViewDimension::Texture2DArray;
                    else
                        viewDimension = ViewDimension::TextureCube;
                    break;
                case TextureDimension::Texture3D:
                    viewDimension = ViewDimension::Texture3D;
                    break;
                default:
                    FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unhandled view dimension requested. Please implement.");
                    break;
                }

                // Bind the resource
                backendContext->pRingBufferViews->BindTextureResource(pResource->GetResource(),
                                                                      texDesc,
                                                                      ResourceViewType::TextureSRV,
                                                                      viewDimension,
                                                                      -1,
                                                                      -1,
                                                                      -1,
                                                                      backendContext->descRingBufferBase + currentSrvResourceIndex - TEXTURE_BINDING_SHIFT);

            }

        }

        if (job->computeJobDescriptor.pipeline.srvTextureCount)
        {
            backendContext->descRingBufferBase += maximumSrvIndex + 1;
        }


    }

    // Bind Texture UAVs
    {
        uint32_t maximumUavIndex = 0;
        for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex)
        {
            uint32_t uavResourceOffset = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex +
                                         job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindCount - 1 - UNORDERED_ACCESS_VIEW_BINDING_SHIFT;
            maximumUavIndex           = uavResourceOffset > maximumUavIndex ? uavResourceOffset : maximumUavIndex;
        }

        // Check if this fits into the ring buffer, loop if not fitting
        if (backendContext->descRingBufferBase + maximumUavIndex + 1 > FFX_RING_BUFFER_SIZE)
            backendContext->descRingBufferBase = 0;

        // Set the offset to use for binding
        paramSet.SetBindTypeOffset(BindingType::TextureUAV, backendContext->descRingBufferBase);
        for (uint32_t currentPipelineUavIndex = 0, currentUAVResource = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex)
        {
            addBarrier(backendContext, &job->computeJobDescriptor.uavTextures[currentPipelineUavIndex], FFX_RESOURCE_STATE_UNORDERED_ACCESS);

            for (uint32_t uavEntry = 0; uavEntry < job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindCount; ++uavEntry, ++currentUAVResource)
            {
                // source: UAV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.uavTextures[currentUAVResource].internalIndex;
                const uint32_t mipIndex = job->computeJobDescriptor.uavTextureMips[currentUAVResource];

                // where to bind it
                const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex + uavEntry;

                const BackendContext_Cauldron::Resource& resource = backendContext->pResources[resourceIndex];
                const Texture* pResource = reinterpret_cast<const GPUResource*>(resource.resourcePtr)->GetTextureResource();
                const TextureDesc& texDesc = pResource->GetDesc();
                ViewDimension viewDimension;

                bool requestArrayView = FFX_CONTAINS_FLAG(resource.resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

                switch (texDesc.Dimension)
                {
                case TextureDimension::Texture1D:
                    viewDimension = (texDesc.DepthOrArraySize > 1 || requestArrayView) ? ViewDimension::Texture1DArray : ViewDimension::Texture1D;
                    break;
                case TextureDimension::Texture2D:
                case TextureDimension::CubeMap:
                    viewDimension = (texDesc.DepthOrArraySize > 1 || requestArrayView) ? ViewDimension::Texture2DArray : ViewDimension::Texture2D;
                    break;
                case TextureDimension::Texture3D:
                    viewDimension = ViewDimension::Texture3D;
                    break;
                default:
                    FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unhandled view dimension requested. Please implement.");
                    break;
                }

                backendContext->pRingBufferViews->BindTextureResource(
                    pResource->GetResource(),
                    texDesc,
                    ResourceViewType::TextureUAV,
                    viewDimension,
                    mipIndex,
                    -1,
                    -1,
                    backendContext->descRingBufferBase + currentUavResourceIndex - UNORDERED_ACCESS_VIEW_BINDING_SHIFT);
            }
        }

        if (job->computeJobDescriptor.pipeline.uavTextureCount) {
            backendContext->descRingBufferBase += maximumUavIndex + 1;
        }
    }

    // Bind Buffer SRVs
    {
        uint32_t maximumSrvIndex = 0;
        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
        {
            uint32_t srvResourceOffset = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex +
                                         job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].bindCount - 1 - TEXTURE_BINDING_SHIFT;
            maximumSrvIndex = srvResourceOffset > maximumSrvIndex ? srvResourceOffset : maximumSrvIndex;
        }

        // Check if this fits into the ring buffer, loop if not fitting
        if (backendContext->descRingBufferBase + maximumSrvIndex + 1 > FFX_RING_BUFFER_SIZE)
            backendContext->descRingBufferBase = 0;

        // Set the offset to use for binding
        paramSet.SetBindTypeOffset(BindingType::BufferSRV, backendContext->descRingBufferBase);
        for (uint32_t currentPipelineSrvIndex = 0, currentSRVResource = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
        {
            addBarrier(backendContext, &job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex], FFX_RESOURCE_STATE_COMPUTE_READ);

            for (uint32_t srvEntry = 0; srvEntry < job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].bindCount; ++srvEntry, ++currentSRVResource)
            {
                // source: SRV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.srvBuffers[currentSRVResource].internalIndex;

                // where to bind it
                const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex + srvEntry;

                const Buffer*     pResource = reinterpret_cast<const GPUResource*>(backendContext->pResources[resourceIndex].resourcePtr)->GetBufferResource();
                const BufferDesc& bufDesc   = pResource->GetDesc();
                backendContext->pRingBufferViews->BindBufferResource(
                    pResource->GetResource(),
                    bufDesc,
                    cauldron::ResourceViewType::BufferSRV,
                    0,
                    bufDesc.Size / bufDesc.Stride,
                    backendContext->descRingBufferBase + currentSrvResourceIndex - TEXTURE_BINDING_SHIFT);
            }
        }

        if (job->computeJobDescriptor.pipeline.srvBufferCount) {
            backendContext->descRingBufferBase += maximumSrvIndex + 1;
        }
    }

    // Bind Buffer UAVs
    {
        uint32_t maximumUavIndex = 0;
        for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex)
        {
            uint32_t uavResourceOffset = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex +
                                         job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].bindCount - 1 - UNORDERED_ACCESS_VIEW_BINDING_SHIFT;
            maximumUavIndex = uavResourceOffset > maximumUavIndex ? uavResourceOffset : maximumUavIndex;
        }

        // Check if this fits into the ring buffer, loop if not fitting
        if (backendContext->descRingBufferBase + maximumUavIndex + 1 > FFX_RING_BUFFER_SIZE)
            backendContext->descRingBufferBase = 0;

        // Set the offset to use for binding
        paramSet.SetBindTypeOffset(BindingType::BufferUAV, backendContext->descRingBufferBase);
        for (uint32_t currentPipelineUavIndex = 0, currentUAVResource = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex)
        {
            addBarrier(backendContext, &job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex], FFX_RESOURCE_STATE_UNORDERED_ACCESS);

            for (uint32_t uavEntry = 0; uavEntry < job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].bindCount;
                 ++uavEntry, ++currentUAVResource)
            {
                // source: UAV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.uavBuffers[currentUAVResource].internalIndex;

                // where to bind it
                const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex + uavEntry;

                const Buffer*     pResource  = reinterpret_cast<const GPUResource*>(backendContext->pResources[resourceIndex].resourcePtr)->GetBufferResource();
                const BufferDesc& bufDesc    = pResource->GetDesc();

                backendContext->pRingBufferViews->BindBufferResource(
                    pResource->GetResource(),
                    bufDesc,
                    cauldron::ResourceViewType::BufferUAV,
                    0,
                    bufDesc.Size / bufDesc.Stride,
                    backendContext->descRingBufferBase + currentUavResourceIndex - UNORDERED_ACCESS_VIEW_BINDING_SHIFT);

            }

        }

        if (job->computeJobDescriptor.pipeline.uavBufferCount) {
            backendContext->descRingBufferBase += maximumUavIndex + 1;
        }
    }

    // If we are dispatching indirectly, transition the argument resource to indirect argument
    if (cauldronIndirectWorkload)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.cmdArgument, FFX_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    // Flush barriers
    flushBarriers(backendContext, pCmdList);

    // Bind constants using Dynamic buffer pool
    BufferAddressInfo bufferViewInfo[FFX_MAX_NUM_CONST_BUFFERS];
    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex)
    {
        // NOTE: for vulkan, we should shift by CONSTANT_BUFFER_BINDING_SHIFT, but since we are assuming that constants buffers are using slots from 0 to constCount - 1, there is no need to do so.
        paramSet.SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(),
                                               job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t),
                                               currentRootConstantIndex);
        bufferViewInfo[currentRootConstantIndex] =
            GetDynamicBufferPool()->AllocConstantBuffer(job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t),
                                                        job->computeJobDescriptor.cbs[currentRootConstantIndex].data);
        paramSet.UpdateRootConstantBuffer(&bufferViewInfo[currentRootConstantIndex], currentRootConstantIndex);
    }

    // Bind the pipeline and the parameters
    SetPipelineState(pCmdList, cauldronPipelineData->pipelineObject);
    paramSet.Bind(pCmdList, cauldronPipelineData->pipelineObject);

    // Dispatch (or dispatch indirect)
    if (cauldronIndirectWorkload)
    {
        const uint32_t resourceIndex = job->computeJobDescriptor.cmdArgument.internalIndex;
        const Buffer* pResource = reinterpret_cast<const GPUResource*>(backendContext->pResources[resourceIndex].resourcePtr)->GetBufferResource();
        ExecuteIndirect(pCmdList, cauldronIndirectWorkload, pResource, 1, job->computeJobDescriptor.cmdArgumentOffset);
    }
    else
    {
        Dispatch(pCmdList, job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_Cauldron* backendContext, FfxGpuJobDescription* job, CommandList* pCmdList)
{
    const GPUResource* pSrcResource = ffxGetCauldronResourcePtr(backendContext, job->copyJobDescriptor.src.internalIndex);
    const GPUResource* pDstResource = ffxGetCauldronResourcePtr(backendContext, job->copyJobDescriptor.dst.internalIndex);

    // Transition the resources
    addBarrier(backendContext, &job->copyJobDescriptor.src, FFX_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFX_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, pCmdList);

    // Do the resource copy
    if (pDstResource->IsBuffer())
    {
        BufferCopyDesc copyDesc = BufferCopyDesc(pSrcResource, pDstResource);
        CopyBufferRegion(pCmdList, &copyDesc);
    }
    else
    {
        TextureCopyDesc copyDesc = TextureCopyDesc(pSrcResource, pDstResource);
        CopyTextureRegion(pCmdList, &copyDesc);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_Cauldron* backendContext, FfxGpuJobDescription* job, CommandList* pCmdList)
{
    uint32_t                          idx              = job->clearJobDescriptor.target.internalIndex;
    BackendContext_Cauldron::Resource ffxResource      = backendContext->pResources[idx];
    const GPUResource*                cauldronResource = reinterpret_cast<const GPUResource*>(ffxResource.resourcePtr);

    // Grab the srv/uav indicies for the resource
    uint32_t srvIndex = ffxResource.srvDescIndex;
    uint32_t uavIndex = ffxResource.uavDescIndex;

    // Set all resource view heaps
    SetAllResourceViewHeaps(pCmdList, backendContext->pResourceViewAllocator);

    // Barrier for clear
    addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    flushBarriers(backendContext, pCmdList);

    ResourceViewInfo gpuViewInfo = backendContext->pGpuResourceViews->GetViewInfo(uavIndex);
    ResourceViewInfo cpuViewInfo = backendContext->pCpuResourceViews->GetViewInfo(uavIndex);
    ClearUAVFloat(pCmdList, cauldronResource, &gpuViewInfo, &cpuViewInfo, job->clearJobDescriptor.color);

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsCauldron(FfxInterface* backendInterface, FfxCommandList commandList)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_Cauldron* backendContext = (BackendContext_Cauldron*)backendInterface->scratchBuffer;

    FFX_ASSERT(nullptr != commandList);
    CommandList* pCmdList = reinterpret_cast<CommandList*>(commandList);

    FfxErrorCode errorCode = FFX_OK;

    // Execute all GpuJobs
    for (uint32_t currentGpuJobIndex = 0; currentGpuJobIndex < backendContext->gpuJobCount; ++currentGpuJobIndex)
    {
        FfxGpuJobDescription*      GpuJob          = &backendContext->pGpuJobs[currentGpuJobIndex];

        switch (GpuJob->jobType)
        {
        case FFX_GPU_JOB_CLEAR_FLOAT:
            errorCode = executeGpuJobClearFloat(backendContext, GpuJob, pCmdList);
            break;

        case FFX_GPU_JOB_COPY:
            errorCode = executeGpuJobCopy(backendContext, GpuJob, pCmdList);
            break;

        case FFX_GPU_JOB_COMPUTE:
            errorCode = executeGpuJobCompute(backendContext, GpuJob, pCmdList);
            break;

        default:
            break;
        }
    }

    // Check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(errorCode == FFX_OK, FFX_ERROR_BACKEND_API_ERROR);
    backendContext->gpuJobCount = 0;
    return FFX_OK;
}
