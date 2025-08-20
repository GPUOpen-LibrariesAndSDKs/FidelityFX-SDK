<!-- @page page_ffx-api Introduction to FidelityFX API -->

<h1>Introduction to FidelityFX API</h1>

The FidelityFX API is a simple API created for small ABI surface and forwards-compatibility. It is delivered in Dynamic Link Library form and consists of 5 functions, declared in [`ffx_api.h`](../../Kits/FidelityFX/api/include/ffx_api.h):

* `ffxCreateContext`
* `ffxDestroyContext`
* `ffxDispatch`
* `ffxQuery`
* `ffxConfigure`

Arguments are provided in a linked list of structs, each having a header with a struct type and pointer to next struct.

An application using the FidelityFX API must use one of the provided signed DLLs. This can be loaded at runtime with `LoadLibrary` and `GetProcAddress` (this is recommended) or at application startup using the dynamic linker via the .lib file.

Backend-specific functionality (currently only for DirectX 12) is only supported with the respective DLL.

For convenience in C++ applications, helpers for initializing struct types correctly and linking headers are provided. Simply use the `.hpp` version of each header and replace the `ffx` prefix with the `ffx::` namespace.
Note that the helper functions wrapping API functions only work when linking using the .lib file. Using them with runtime loading will result in linker errors.

<h2>DLLs structure</h2>
Starting with AMD FidelityFX™ SDK 2.0.0 the effects, previously combined in amd_fidelityfx_dx12.dll, are split into multiple DLLs based on effect type:

* amd_fidelityfx_loader_dx12.dll: A small loader DLL, not containing any effect code. This DLL only manages the loading of effect type DLLs to provide the effects to the calling application. It is compatible in interface and behaviour to amd_fidelityfx_dx12.dll.
* amd_fidelityfx_framegeneration_dx12.dll: This DLL contains [FidelityFX Frame Generation](../techniques/super-resolution-interpolation.md#enable-fsr3-s-proxy-frame-generation-swapchain) related providers, i.e. [Frame Interpolation](../techniques/frame-interpolation.md) effects and [Frame Interpolation Swapchain](../techniques/frame-interpolation-swap-chain.md).
* amd_fidelityfx_upscaler_dx12.dll: This DLL contains providers for latest FidelityFX Super Resolution technique ([FSR4](../techniques/super-resolution-ml.md )) as well as legacy ones ([FSR2](../techniques/super-resolution-temporal.md) and [FSR3.1 upscaler](../techniques/super-resolution-upscaler.md)).

The benefit of this split is, that applications can ship with only supported AMD FidelityFX™ effect types.

Applications previously loading amd_fidelityfx_dx12.dll (or linking with amd_fidelityfx_dx12.lib) need to load amd_fidelityfx_loader_dx12.dll (or link to amd_fidelityfx_loader_dx12.lib) instead.

<h2>Descriptor structs</h2>

Functionality for effects is exposed using descriptor structs. Each struct has a header:

```C
typedef struct ffxApiHeader
{
    ffxStructType_t      type;  ///< The structure type. Must always be set to the corresponding value for any structure (found nearby with a similar name).
    struct ffxApiHeader* pNext; ///< Pointer to next structure, used for optional parameters and extensions. Can be null.
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

The `context` should be initalized to `null` before the call. Note that the second argument is a pointer to the struct header, not the struct itself. The third argument is used for custom allocators and may be `null`, see the [memory allocation section](#memory-allocation).

Example call:

```C
struct ffxCreateBackendDX12Desc createBackend = /*...*/;

struct ffxCreateContextDescUpscale createUpscale = { 0 };
createUpscale.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;

// fill struct ...

createUpscale.header.pNext = &createBackend.header;

ffxContext upscaleContext = NULL;
ffxReturnCode_t retCode = ffxContextCreate(&upscaleContext, &createUpscale.header, NULL);
// handle errors
```

Note that when using the C++ wrapper, the third argument comes second instead to support a variadic description argument.

```C++
// equivalent of above, chain of createUpscale and createBackend will be linked automatically.
ffxReturnCode_t retCode = ffx::ContextCreate(upscaleContext, nullptr, createUpscale, createBackend);
```

<h2>Context destruction</h2>

To destroy a context, call `ffxDestroyContext`:

```C
ffxReturnCode_t ffxDestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb);
```

The context will be `null` after the call. The memory callbacks must be compatible with the allocation callbacks used during context creation, see the [memory allocation section](#memory-allocation).

<h2>Query</h2>

To query information or resources from an effect, use `ffxQuery`:

```C
ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc);
```

Output values will be returned by writing through pointers passed in the query description.

Some query descriptor types require passing valid context created by `ffxCreateContext`. 


Example query version of effect after context creation with default version from the SDK sample: 
```C
struct ffxQueryGetProviderVersion getVersion = {0};
getVersion.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;

ffxReturnCode_t retCode_t = ffxQuery(&m_UpscalingContext, &getVersion.header);
```

Queries of descriptor types with `NULL` context, require providing more input information like device, flags, and resolution.


Example query for GPU memory usage of upscaler with default version before creating context from the SDK sample. 

```C
struct FfxApiEffectMemoryUsage gpuMemoryUsageUpscaler = {0};
struct ffxQueryDescUpscaleGetGPUMemoryUsageV2 upscalerGetGPUMemoryUsageV2 = {0};
upscalerGetGPUMemoryUsageV2.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2;
upscalerGetGPUMemoryUsageV2.device = GetDevice()->GetImpl()->DX12Device();
upscalerGetGPUMemoryUsageV2.maxRenderSize = { resInfo.RenderWidth, resInfo.RenderHeight };
upscalerGetGPUMemoryUsageV2.maxUpscaleSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
upscalerGetGPUMemoryUsageV2.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE | FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
upscalerGetGPUMemoryUsageV2.gpuMemoryUsageUpscaler = &gpuMemoryUsageUpscaler;

ffxReturnCode_t retCode_t = ffxQuery(nullptr, &upscalerGetGPUMemoryUsageV2.header);

CAUDRON_LOG_INFO(L"Default Upscaler Query GPUMemoryUsageV2 totalUsageInBytes %f ", gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f);
```

If using C++ helper, need to omit the context argument if context is `NULL`.
```C++
//this ffx:Query call is equivalent to above ffxQuery call
ffx::ReturnCode retCode = ffx::Query(upscalerGetGPUMemoryUsageV2);
```
To query a specific version of the effect, attach a struct of type `ffxOverrideVersion` in the `pNext` field of the main struct header. 

Continuing from example above, query the GPU memory usage of upscaler at a specific version:

```C++
struct ffxOverrideVersion versionOverride = {0};
versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
versionOverride.versionId = m_FsrVersionIds[m_FsrVersionIndex];
upscalerGetGPUMemoryUsageV2.header.pNext = &versionOverride.header;

ffxReturnCode_t retCode_t = ffxQuery(nullptr, &upscalerGetGPUMemoryUsageV2.header);
```


<h2>Configure</h2>

To configure an effect, use `ffxConfigure`:

```C
ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc);
```

The context must be a valid context created by `ffxCreateContext`, unless documentation for a specific configure description states otherwise.

All effects have a key-value configure struct used for simple options, for example:

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

GPU rendering dispatches will encode their commands into a command list/buffer passed as input.

CPU dispatches will usually execute their function synchronously and immediately.

<h2>Resource structs</h2>

Resources like textures and buffers are passed to the FidelityFX API using the `FfxApiResource` structure.

For C++ applications, the backend headers define helper functions for constructing this from a native resource handle.

<h2>Version selection</h2>

FidelityFX API supports overriding the version of each effect on context creation. This is an optional feature. If version overrides are used, they **must** be used consistently:

* Only use version IDs returned by `ffxQuery` in `ffxQueryDescGetVersions` with the appropriate creation struct type
* Do not hard-code version IDs
* If calling `ffxQuery` without a context (`NULL` parameter), use the same version override as with `ffxCreateContext`
* The choice of version should either be left to the default behavior (no override) or the user (displayed in options UI)
* Versions reported by a version query may change based on the parameters. Do not query version names and ids separately, as their order may change.

Example version query:

```C
struct ffxQueryDescGetVersions versionQuery = {0};
versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
versionQuery.device = GetDX12Device(); // only for DirectX 12 applications
uint64_t versionCount = 0;
versionQuery.outputCount = &versionCount;
// get number of versions for allocation
ffxQuery(nullptr, &versionQuery.header);

std::vector<const char*> versionNames;
std::vector<uint64_t> versionIds;
m_FsrVersionIds.resize(versionCount);
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

<h2>Error handling</h2>

All calls to the FidelityFX API return a value of type `ffxReturnCode_t`. If using the C++ wrapper, this is replaced by `ffx::ReturnCode`.

A successful operation will return `FFX_API_RETURN_OK`. All other return codes are errors. Future versions may add new return codes not yet present in the headers.

Applications should be able to handle errors gracefully, even if it is unrecoverable.

<h2>Memory allocation</h2>

To control memory allocations, pass a pointer to `ffxAllocationCallbacks` to `ffxCreateContext` and `ffxDestroyContext`.

If a null pointer is passed, then global `malloc` and `free` will be used.

For custom allocators, two functions are required:

```C
// Memory allocation function. Must return a valid pointer to at least size bytes of memory aligned to hold any type.
// May return null to indicate failure. Standard library malloc fulfills this requirement.
typedef void* (*ffxAlloc)(void* pUserData, uint64_t size);

// Memory deallocation function. May be called with null pointer as second argument.
typedef void (*ffxDealloc)(void* pUserData, void* pMem);

typedef struct ffxAllocationCallbacks
{
    void* pUserData;
    ffxAlloc alloc;
    ffxDealloc dealloc;
} ffxAllocationCallbacks;
```

`pUserData` is passed through to the callbacks without modification or validation. FidelityFX API code will not attempt to dereference it, nor will it store it.

If a custom allocator is used for context creation, a compatible struct must be used for destruction. That means that any pointer allocated with the callbacks and user data during context creation must be deallocatable using the callback and user data passed to `ffxDestroyContext`.
