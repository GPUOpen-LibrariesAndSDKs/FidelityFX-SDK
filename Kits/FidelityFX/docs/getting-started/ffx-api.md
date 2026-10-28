<!-- @page page_ffx-api Introduction to AMD FSR™ API -->

<h1>Introduction to the AMD FSR™ API</h1>

The AMD FSR™ API is a simple API designed with a small ABI surface and forward compatibility in mind. It is delivered in Dynamic Link Library form and consists of 5 functions, declared in [`ffx_api.h`](../../api/include/ffx_api.h):

* `ffxCreateContext`
* `ffxDestroyContext`
* `ffxDispatch`
* `ffxQuery`
* `ffxConfigure`

Arguments are passed via a linked list of descriptor structs. Each struct contains a header identifying its type and a pointer to the next struct.

An application using the AMD FSR™ API must use one of the provided signed DLLs. This can be loaded at runtime with `LoadLibrary` and `GetProcAddress` (this is recommended) or at application startup using the dynamic linker via the .lib file.

For convenience in C++ applications, helpers for initializing struct types correctly and linking headers are provided. Simply use the `.hpp` version of each header and replace the `ffx` prefix with the `ffx::` namespace.
Note that the helper functions wrapping API functions only work when linking using the .lib file. Using them with runtime loading will result in linker errors.

<h2>DLLs structure</h2>

Starting with AMD FSR™ SDK 2.0.0, the effects previously combined in amd_fidelityfx_dx12(vk).dll are split into multiple DLLs based on effect type:

* amd_fidelityfx_loader_dx12(vk).dll: A small loader DLL, not containing any effect code. This DLL only manages the loading of effect type DLLs to provide the effects to the calling application. It is interface- and behavior-compatible with amd_fidelityfx_dx12(vk).dll.
* amd_fidelityfx_framegeneration_dx12(vk).dll: This DLL contains providers for the latest AMD FSR™ Frame Generation techniques, [AMD FSR™ Frame Generation 4](../techniques/frame-interpolation-ml.md) and [AMD FSR™ Frame Generation (Swapchain) 4](../techniques/frame-interpolation-swap-chain.md), as well as legacy ones, [AMD FidelityFX™ Super Resolution Frame Generation 3](../techniques/frame-interpolation.md) and [AMD FidelityFX™ Super Resolution Frame Generation Swapchain 3](../techniques/frame-interpolation-swap-chain.md).
* amd_fidelityfx_upscaler_dx12(vk).dll: This DLL contains providers for the latest AMD FSR™ Upscaling technique ([AMD FSR™ Upscaling 4](../techniques/super-resolution-ml.md)) as well as legacy ones ([AMD FidelityFX™ Super Resolution 3](../techniques/super-resolution-upscaler.md) and [AMD FidelityFX™ Super Resolution 2](../techniques/super-resolution-temporal.md)).
* amd_fidelityfx_denoiser_dx12.dll: This DLL contains the provider for the latest AMD FSR™ Ray Regeneration technique [AMD FSR™ Ray Regeneration 1](../techniques/denoising.md).
* amd_fidelityfx_radiancecache_dx12.dll: This DLL contains the provider for the latest AMD FSR™ Radiance Caching technique [AMD FSR™ Radiance Caching (Preview)](../techniques/radiance-cache.md).

This split allows applications to ship only the AMD FSR™ effect types they use.

Applications previously loading amd_fidelityfx_dx12(vk).dll (or linking with amd_fidelityfx_dx12(vk).lib) must now load amd_fidelityfx_loader_dx12(vk).dll (or link to amd_fidelityfx_loader_dx12(vk).lib) instead.

<h3>Effect DLL lookup</h3>

The loader DLL resolves each effect DLL in the following order:

1. A module of that name already loaded into the process is reused, regardless of where it was loaded from;
2. The folder containing the application executable;
3. The folder containing the loader DLL itself.

The last step allows the loader and the effect DLLs to be shipped together in a subfolder, for example as part of a plugin, without placing them next to the application executable.

<h2>Descriptor structs</h2>

Functionality for effects is exposed using descriptor structs. Each struct has a header:

```C
typedef struct ffxApiHeader
{
    ffxStructType_t      type;  ///< The structure type. Must always be set to the corresponding value for any structure (found nearby with a similar name).
    struct ffxApiHeader* pNext; ///< Pointer to next structure, used for optional parameters and extensions. Can be NULL.
} ffxApiHeader;
```

Each descriptor has an associated struct type, usually defined directly before the struct declaration in the header.
The `type` field in the header must be set to that struct type. Failing to do this is undefined behavior and will likely result in crashes. The C++ headers (with extension `.hpp`) expose helpers for automatic initialization.

The `pNext` field is used to specify additional parameters and extensions in a linked list (or "chain"). Some calls require chaining certain other structs.

<h2>Context creation</h2>

Context creation is done by calling `ffxCreateContext`, which is declared as follows:

```C
ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc, const ffxAllocationCallbacks* memCb);
```

The `context` should be initialized to `NULL` before the call. Note that the second argument is a pointer to the struct header, not the struct itself. The third argument is used for custom allocators and may be `NULL`, see the [memory allocation section](#memory-allocation).

Example call:

**DirectX® 12**

```C
struct ffxCreateBackendDX12Desc createBackend = { 0 };
createBackend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
createBackend.device = pD3D12Device;

struct ffxCreateContextDescUpscale createUpscale = { 0 };
createUpscale.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;

// fill struct ...

createUpscale.header.pNext = &createBackend.header;

ffxContext upscaleContext = NULL;
ffxReturnCode_t retCode = ffxCreateContext(&upscaleContext, &createUpscale.header, NULL);
// handle errors
```

**Vulkan®**

`deviceContext` is a [`VkDeviceContext`](../../api/include/vk/ffx_api_vk.h#L51):

```C
struct VkDeviceContext deviceContext = { 0 };
deviceContext.vkDevice = vkDevice;
deviceContext.vkPhysicalDevice = vkPhysicalDevice;
deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;

struct ffxCreateBackendVKDesc createBackend = { 0 };
createBackend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
createBackend.deviceContext = deviceContext;

struct ffxCreateContextDescUpscale createUpscale = { 0 };
createUpscale.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;

// fill struct ...

createUpscale.header.pNext = &createBackend.header;

ffxContext upscaleContext = NULL;
ffxReturnCode_t retCode = ffxCreateContext(&upscaleContext, &createUpscale.header, NULL);
// handle errors
```
Note that in the C++ wrapper, the allocator argument appears before the descriptor arguments to support a variadic descriptor interface.

```C++
// equivalent of above, chain of createUpscale and createBackend will be linked automatically.
ffx::Context upscaleContext = nullptr;
ffx::ReturnCode retCode = ffx::CreateContext(upscaleContext, nullptr, createUpscale, createBackend);
```

<h2>Context destruction</h2>

To destroy a context, call `ffxDestroyContext`:

```C
ffxReturnCode_t ffxDestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb);
```

The context will be `NULL` after the call. The memory callbacks must be compatible with the allocation callbacks used during context creation, see the [memory allocation section](#memory-allocation).

<h2>Query</h2>

To query information or resources from an effect, use `ffxQuery`:

```C
ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc);
```

Output values will be returned by writing through pointers passed in the query descriptor.

Only some query descriptor types support `ffxQuery(nullptr, ...)`. The currently supported public null-context queries are:

* `ffxQueryDescGetVersions`
* `ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode`
* `ffxQueryDescUpscaleGetRenderResolutionFromQualityMode`
* `ffxQueryDescUpscaleGetJitterPhaseCount`
* `ffxQueryDescUpscaleGetJitterOffset`
* `ffxQueryDescUpscaleGetGPUMemoryUsageV2`
* `ffxQueryDescUpscaleGetResourceRequirements`
* `ffxQueryDescDenoiserGetGPUMemoryUsage`
* `ffxQueryDescDenoiserGetVersion`
* `ffxQueryDescFrameGenerationGetGPUMemoryUsageV2`
* `ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2`
* `ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2`

All other query descriptor types require a valid context created by `ffxCreateContext`. 


Example query for the version of an effect after context creation with default version from the SDK sample: 
```C
struct ffxQueryGetProviderVersion getVersion = {0};
getVersion.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;

ffxReturnCode_t retCode_t = ffxQuery(&m_UpscalingContext, &getVersion.header);
```

Queries using a `NULL` context require additional input information like device, flags, and resolution.

For null-context queries that don't embed a `device` field directly (e.g. `ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode`), the device must be provided by chaining a backend creation struct via `header.pNext`. This is necessary for the query to be routed to effect providers embedded in the driver or requiring certain device capabilities. Without it, `ffxQuery` will return `FFX_API_RETURN_NO_PROVIDER` (4) and emit a debug error message.

Note that there is an additional performance cost for `NULL` context queries compared to queries on a created context: they require checking for a driver-provided version and checking device compatibility before the query is executed.

Example chaining a backend description for a null-context query:

**DirectX 12**

```C
ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode ratioDesc = {0};
ratioDesc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
ratioDesc.qualityMode = FFX_UPSCALE_QUALITY_MODE_QUALITY;
ratioDesc.pOutUpscaleRatio = &outRatio;

struct ffxCreateBackendDX12Desc backendDesc = {0};
backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
backendDesc.device = pD3D12Device;
ratioDesc.header.pNext = &backendDesc.header;

ffxReturnCode_t retCode = ffxQuery(nullptr, &ratioDesc.header);
```

**Vulkan**

```C
ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode ratioDesc = {0};
ratioDesc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
ratioDesc.qualityMode = FFX_UPSCALE_QUALITY_MODE_QUALITY;
ratioDesc.pOutUpscaleRatio = &outRatio;

struct VkDeviceContext deviceContext = {0};
deviceContext.vkDevice = vkDevice;
deviceContext.vkPhysicalDevice = vkPhysicalDevice;
deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;

struct ffxCreateBackendVKDesc backendDesc = {0};
backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
backendDesc.deviceContext = deviceContext;
ratioDesc.header.pNext = &backendDesc.header;

ffxReturnCode_t retCode = ffxQuery(nullptr, &ratioDesc.header);
```

Null-context query structs that already embed a `device` field (e.g. `ffxQueryDescUpscaleGetGPUMemoryUsageV2`, `ffxQueryDescGetVersions`) do not need this extra chaining.

Example query for GPU memory usage of upscaler with default version before creating context from the SDK sample. 

`device` is an `ID3D12Device*` on DirectX® 12. On Vulkan® it is a pointer to [`VkDeviceContext`](../../api/include/vk/ffx_api_vk.h#L51).

```C
struct FfxApiEffectMemoryUsage gpuMemoryUsageUpscaler = {0};
struct ffxQueryDescUpscaleGetGPUMemoryUsageV2 upscalerGetGPUMemoryUsageV2 = {0};
upscalerGetGPUMemoryUsageV2.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2;

void* device = NULL;
#if defined(_DX12)
device = GetDevice()->GetImpl()->DX12Device();
#elif defined(_VK)
struct VkDeviceContext deviceContext = {0};
deviceContext.vkDevice = vkDevice;
deviceContext.vkPhysicalDevice = vkPhysicalDevice;
deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
device = &deviceContext;
#endif
upscalerGetGPUMemoryUsageV2.device = device;
upscalerGetGPUMemoryUsageV2.maxRenderSize = { resInfo.RenderWidth, resInfo.RenderHeight };
upscalerGetGPUMemoryUsageV2.maxUpscaleSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
upscalerGetGPUMemoryUsageV2.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE | FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
upscalerGetGPUMemoryUsageV2.gpuMemoryUsageUpscaler = &gpuMemoryUsageUpscaler;

ffxReturnCode_t retCode_t = ffxQuery(nullptr, &upscalerGetGPUMemoryUsageV2.header);

CAULDRON_LOG_INFO(L"Default Upscaler Query GPUMemoryUsageV2 totalUsageInBytes %f ", gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f);
```

If using the C++ helper, you need to omit the context argument if the context is `NULL`.

```C++
// This ffx::Query call is equivalent to the ffxQuery call above
ffx::ReturnCode retCode = ffx::Query(upscalerGetGPUMemoryUsageV2);
```

To query a specific version of the effect, attach a struct of type `ffxOverrideVersion` in the `pNext` field of the main struct header,
and as stated above, attach a backend creation struct if the query struct itself does not specify the device.

Continuing from example above, query the GPU memory usage of upscaler at a specific version:

```C++
ffx::OverrideVersion versionOverride{};
versionOverride.versionId = m_FsrVersionIds[m_FsrVersionIndex];
ffx::ReturnCode retCode = ffx::Query(upscalerGetGPUMemoryUsageV2, versionOverride);
```

Example for querying resource requirements of a specific version:

DirectX 12:

```C++
ffx::QueryDescUpscaleGetResourceRequirements queryResourceRequirements{};
ffx::OverrideVersion versionOverride{};
ffx::CreateBackendDX12Desc backendDesc{};
backendDesc.device = pD3D12Device;
versionOverride.versionId = selectedVersion;
ffx::ReturnCode retCode = ffx::Query(queryResourceRequirements, versionOverride, backendDesc);
```

Vulkan:

```C++
ffx::QueryDescUpscaleGetResourceRequirements queryResourceRequirements{};
ffx::OverrideVersion versionOverride{};
ffx::CreateBackendVKDesc backendDesc{};
backendDesc.deviceContext = deviceContext;
versionOverride.versionId = selectedVersion;
ffx::ReturnCode retCode = ffx::Query(queryResourceRequirements, versionOverride, backendDesc);
```


<h2>Configure</h2>

To configure an effect, use `ffxConfigure`:

```C
ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc);
```

The context must be a valid context created by `ffxCreateContext`, unless documentation for a specific configure descriptor states otherwise.

Many effects have a key-value configure struct used for simple options, for example:

```C
struct ffxConfigureDescUpscaleKeyValue
{
    ffxConfigureDescHeader  header;
    uint64_t                key;        ///< Configuration key, member of the FfxApiConfigureUpscaleKey enumeration.
    uint64_t                u64;        ///< Integer value or enum value to set.
    void*                   ptr;        ///< Pointer to set or pointer to value to set.
};
```

The available keys and constraints on values to pass are specified in the relevant technique's documentation.

<h2>Dispatch</h2>

To dispatch rendering commands or other functionality, use `ffxDispatch`:

```C
ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc);
```

The context must be a valid context created by `ffxCreateContext`.

GPU rendering dispatches encode their commands into a command list/buffer provided in the descriptor.

CPU dispatches will usually execute their function synchronously and immediately.

<h2>Resource structs</h2>

Resources like textures and buffers are passed to the AMD FSR™ API using the `FfxApiResource` structure.

For C++ applications, the backend headers define helper functions for constructing this from a native resource handle. On DirectX® 12, [`ffxApiGetResourceDX12`](../../api/include/dx12/ffx_api_dx12.h#L204) takes an `ID3D12Resource`. On Vulkan®, build an `FfxApiResourceDescription` with [`ffxApiGetImageResourceDescriptionVK`](../../api/include/vk/ffx_api_vk.h#L238) or [`ffxApiGetBufferResourceDescriptionVK`](../../api/include/vk/ffx_api_vk.h#L207), then pass that description and the `VkImage` or `VkBuffer` to [`ffxApiGetResourceVK`](../../api/include/vk/ffx_api_vk.h#L298). Surface formats are converted with [`ffxApiGetSurfaceFormatDX12`](../../api/include/dx12/ffx_api_dx12.h#L68) and [`ffxApiGetSurfaceFormatVK`](../../api/include/vk/ffx_api_vk.h#L87).

<h2>Version selection</h2>

AMD FSR™ API supports overriding the version of each effect on context creation. This is an optional feature. If version overrides are used, they **must** be used consistently:

* Only use version IDs returned by `ffxQuery` in `ffxQueryDescGetVersions` with the appropriate creation struct type
* Do not hard-code version IDs
* If calling `ffxQuery` without a context (`NULL` parameter), use the same version override as with `ffxCreateContext`
* The choice of version should either be left to the default behavior (no override) or the user (displayed in options UI)
* Versions reported by a version query may change based on the parameters. Do not query version names and ids separately, as their order may differ between calls.

Due to a previous bug, some version names are stored in global memory and may be overwritten by later version queries. It is therefore recommended to create a copy of the version names.

Example version query:

`device` is an `ID3D12Device*` on DirectX® 12 and a pointer to [`VkDeviceContext`](../../api/include/vk/ffx_api_vk.h#L51) on Vulkan®. The same pointer is passed to frame-generation version queries.

```C
struct ffxQueryDescGetVersions versionQuery = {0};
versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
#if defined(_DX12)
versionQuery.device = GetDX12Device();
#elif defined(_VK)
struct VkDeviceContext deviceContext = {0};
deviceContext.vkDevice = vkDevice;
deviceContext.vkPhysicalDevice = vkPhysicalDevice;
deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
versionQuery.device = &deviceContext;
#endif
uint64_t versionCount = 0;
versionQuery.outputCount = &versionCount;
// get number of versions for allocation
ffxQuery(nullptr, &versionQuery.header);

std::vector<uint64_t> versionIds;
std::vector<const char*> versionNames;
versionIds.resize(versionCount);
versionNames.resize(versionCount);
versionQuery.versionIds = versionIds.data();
versionQuery.versionNames = versionNames.data();
// fill version ids and names arrays.
ffxQuery(nullptr, &versionQuery.header);
```

To override the version during context creation, attach a struct of type `ffxOverrideVersion` in the `pNext` field of the main struct header.

Example context creation with version override from the SDK sample, using C++ helpers:

```C++
ffx::ReturnCode retCode;
// lifetime of this must last until after CreateContext call
ffx::CreateContextDescOverrideVersion versionOverride{};
if (m_FsrVersionIndex < m_FsrVersionIds.size() && m_overrideVersion)
{
    versionOverride.versionId = m_FsrVersionIds[m_FsrVersionIndex];
    retCode = ffx::CreateContext(m_UpscalingContext, nullptr, createFsr, backendDesc, versionOverride);
}
```

You can query the version of any created context using `ffxQuery` with the `ffxQueryGetProviderVersion` struct.

<h3>Driver-provided versions</h3>

In addition to the versions shipped with the application, the AMD display driver can provide newer implementations of an effect. These driver-provided versions are returned by `ffxQueryDescGetVersions` alongside the versions built into the application, and can be identified in two ways:

* Their version name carries a trailing ` *`, for example `FSR 4.1.2 *`;
* Bit 31 of their version id is set. The remaining bits carry the effect version, so `versionId & 0x7FFFFFFF` can be compared against the ids of versions shipped with the application. This encoding may change in a future release, so prefer the version name where either can be used;

An application can pin an effect to a specific version by supplying an effect version override, as described above. **Supplying `ffxOverrideVersion` disables automatic selection.** The requested version id is matched exactly, so overriding with the id of a version shipped with the application pins the application to its own implementation and opts that context out of driver-provided versions.

When no effect version override is supplied, the AMD FSR™ API selects the newest available implementation, preferring a driver-provided version only when its version is strictly higher than the newest version shipped with the application. This is the default behavior, and it is what allows users to benefit from effect improvements delivered through driver updates.

Applications that expose effect version selection in their options UI should therefore:

* List the driver-provided entries returned by the version query rather than filtering them out, so that users can still select them;
* Preserve the ` *` suffix, or render an equivalent indicator, so that users can tell driver-provided versions apart from the versions shipped with the application;
* Offer a default option that applies no application-side version override, so that automatic selection remains available.

Whether driver-provided versions are offered for a given effect is additionally controlled by the driver itself, so they may not be present on every system even when a newer implementation exists.

<h2>Error handling</h2>

All calls to the AMD FSR™ API return a value of type `ffxReturnCode_t`. If using the C++ wrapper, this is replaced by `ffx::ReturnCode`.

A successful operation will return `FFX_API_RETURN_OK`. All other return codes are errors. Future versions may add new return codes not yet present in the headers.

Applications should be able to handle errors gracefully, even if they are unrecoverable.

<h2>Memory allocation</h2>

To control memory allocations, pass a pointer to `ffxAllocationCallbacks` to `ffxCreateContext` and `ffxDestroyContext`.

If a `NULL` pointer is passed, global `malloc` and `free` will be used.

For custom allocators, two functions are required:

```C
// Memory allocation function. Must return a valid pointer to at least size bytes of memory aligned to hold any type.
// May return NULL to indicate failure. Standard library malloc fulfills this requirement.
typedef void* (*ffxAlloc)(void* pUserData, uint64_t size);

// Memory deallocation function. May be called with NULL pointer as second argument.
typedef void (*ffxDealloc)(void* pUserData, void* pMem);

typedef struct ffxAllocationCallbacks
{
    void* pUserData;
    ffxAlloc alloc;
    ffxDealloc dealloc;
} ffxAllocationCallbacks;
```

`pUserData` is passed through to the callbacks without modification or validation. AMD FSR™ API code will not attempt to dereference it, nor will it store it.

If a custom allocator is used for context creation, a compatible struct must be used for destruction. That means that any pointer allocated with the callbacks and user data during context creation must be deallocatable using the callback and user data passed to `ffxDestroyContext`.
