<!-- @page page_techniques_frame-interpolation-api AMD FSR™ Frame Generation API -->

<h1>AMD FSR™ Frame Generation API</h1>

<h2>Table of contents</h2>

- [Introduction](#introduction)
- [Integration](#integration)
    - [Memory Usage](#memory-usage)
    - [New APIs](#new-apis)
    - [Create frame generation context](#create-frame-generation-context)
    - [Configure frame generation](#configure-frame-generation)
    - [Distortion Field](#distortion-field)
    - [UI Composition](#ui-composition)
    - [Dispatch frame generation preparation](#dispatch-frame-generation-preparation)
    - [Dispatch frame generation](#dispatch-frame-generation)
    - [Input color requirements](#input-color-requirements)
    - [Letterboxing or pillarboxing](#letterboxing-or-pillarboxing)
    - [Shutdown](#shutdown)
    - [Thread safety](#thread-safety)
    - [Resource Lifetime](#resource-lifetime)
    - [Debug Checker](#debug-checker)
    - [Debug output](#debug-output)
- [See also](#see-also)

<h2>Introduction</h2>

This document describes the AMD FSR™ API constructs used to integrate **AMD FSR™ Frame Generation** and related swapchain functionality. Use it together with the [Introduction to the AMD FSR™ API](../getting-started/ffx-api.md) for context creation, querying, dispatching, and version selection.

<h2>Integration</h2>

**AMD FSR™ Frame Generation** should be integrated using the [AMD FSR™ API](../getting-started/ffx-api.md). The sections below describe API constructs specific to AMD FSR™ Frame Generation. This API is common across all versions of AMD FSR™ Frame Generation, with any differences noted on the version-specific pages.

<h3>Memory Usage</h3>

An application can get the amount of GPU local memory required by AMD FSR™ Frame Generation after context creation by calling [`ffxQuery`](../../api/include/ffx_api.h#L152) with the valid context and [`ffxQueryDescFrameGenerationGetGPUMemoryUsage`](../../framegeneration/include/ffx_framegeneration.h#L169).

An application can get GPU local memory required by default AMD FSR™ Frame Generation version before context creation by calling [`ffxQuery`](../../api/include/ffx_api.h#L152) with `NULL` context and filling out [`ffxQueryDescFrameGenerationGetGPUMemoryUsageV2`](../../framegeneration/include/ffx_framegeneration.h#L209). To get the memory requirement info for a different AMD FSR™ Frame Generation version, additionally link [`ffxOverrideVersion`](../../api/include/ffx_api.h#L101). 

See code examples of how to call [Query](../getting-started/ffx-api.md#query).

<h3>New APIs</h3>

As of SDK 2.1 it is necessary to create and fill in a [`ffxCreateContextDescFrameGenerationVersion`](../../framegeneration/include/ffx_framegeneration.h#L268), setting the [`version`](../../framegeneration/include/ffx_framegeneration.h#L271) field to [`FFX_FRAMEGENERATION_VERSION`](../../framegeneration/include/ffx_framegeneration.h#L33) and link this in the header.pNext field of the above [`ffxCreateContextDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L69) structure. This is used to ensure compatibility between different versions of the API.

As of SDK 2.1 both [`ffxDispatchDescFrameGenerationPrepare`](../../framegeneration/include/ffx_framegeneration.h#L131) and [`ffxDispatchDescFrameGenerationPrepareCameraInfo`](../../framegeneration/include/ffx_framegeneration.h#L197) are deprecated. Use a [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225) structure instead.

The [`reset`](../../framegeneration/include/ffx_framegeneration.h#L237) field of [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225) is now used by the `AMD FSR™ Frame Generation` implementations and behaves the same as providing disjoint `frameID`, unlike the previous `unused_reset` field which was ignored.

<h3>Create frame generation context</h3>

In order to use frame generation first call [`ffxCreateContext`](../../api/include/ffx_api.h#L134) with a description for frame generation and a backend description.

The [`ffxCreateContextDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L69) structure contains configuration data:
 * A set of initialization flags
 * The maximum resolution the rendering will be performed at
 * The resolution of the resources that will get interpolated
 * The format of the resources that will get interpolated

The initialization flags are provided through the [`FfxApiCreateContextFramegenerationFlags`](../../framegeneration/include/ffx_framegeneration.h#L40) enumeration:

| Flag                                            | Note                                                                                                                         |
|-------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------|
| [`FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT`](../../framegeneration/include/ffx_framegeneration.h#L42) | A bit indicating whether the context supports async. dispatch for frame-generation workloads. |
| [`FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS`](../../framegeneration/include/ffx_framegeneration.h#L43) | A bit indicating if the motion vectors are rendered at display resolution. |
| [`FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION`](../../framegeneration/include/ffx_framegeneration.h#L44) | A bit indicating that the motion vectors have the jittering pattern applied to them. |
| [`FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED`](../../framegeneration/include/ffx_framegeneration.h#L45) | A bit indicating that the input depth buffer data provided is inverted [1..0]. |
| [`FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE`](../../framegeneration/include/ffx_framegeneration.h#L46) | A bit indicating that the input depth buffer data provided is using an infinite far plane.                                   |
| [`FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE`](../../framegeneration/include/ffx_framegeneration.h#L47) | A bit indicating if the input color data provided to all inputs is using a high-dynamic range.                                                                    |
| [`FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING`](../../framegeneration/include/ffx_framegeneration.h#L48) | A bit indicating that the runtime should check some API values and report issues.                                             |

The frame-generation description is the same on both APIs. The backend description is [`ffxCreateBackendDX12Desc`](../../api/include/dx12/ffx_api_dx12.h#L49) on DirectX® 12 and [`ffxCreateBackendVKDesc`](../../api/include/vk/ffx_api_vk.h#L58) on Vulkan®.

Example using the C++ helpers, DirectX® 12:

```C++
ffx::Context frameGenContext;
ffx::CreateBackendDX12Desc backendDesc{};
backendDesc.device = GetDevice()->GetImpl()->DX12Device();

ffx::CreateContextDescFrameGenerationVersion createFgVersion{};
createFgVersion.version = FFX_FRAMEGENERATION_VERSION;

ffx::CreateContextDescFrameGeneration createFg{};
createFg.displaySize = {resInfo.DisplayWidth, resInfo.DisplayHeight};
createFg.maxRenderSize = {resInfo.DisplayWidth, resInfo.DisplayHeight};
createFg.flags = FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;

if (m_EnableAsyncCompute)
    createFg.flags |= FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT;

createFg.backBufferFormat = SDKWrapper::GetFfxSurfaceFormat(GetFramework()->GetSwapChain()->GetSwapChainFormat());
ffx::ReturnCode retCode = ffx::CreateContext(frameGenContext, nullptr, createFg, createFgVersion, backendDesc);
```

Example using the C++ helpers, Vulkan®. [`deviceContext`](../../api/include/vk/ffx_api_vk.h#L61) is a [`VkDeviceContext`](../../api/include/vk/ffx_api_vk.h#L51):

```C++
ffx::Context frameGenContext;
ffx::CreateBackendVKDesc backendDesc{};
VkDeviceContext deviceContext{};
deviceContext.vkDevice         = GetDevice()->GetImpl()->VKDevice();
deviceContext.vkPhysicalDevice = GetDevice()->GetImpl()->VKPhysicalDevice();
deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
backendDesc.deviceContext      = deviceContext;

ffx::CreateContextDescFrameGenerationVersion createFgVersion{};
createFgVersion.version = FFX_FRAMEGENERATION_VERSION;

ffx::CreateContextDescFrameGeneration createFg{};
createFg.displaySize = {resInfo.DisplayWidth, resInfo.DisplayHeight};
createFg.maxRenderSize = {resInfo.DisplayWidth, resInfo.DisplayHeight};
createFg.flags = FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;

if (m_EnableAsyncCompute)
    createFg.flags |= FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT;

createFg.backBufferFormat = SDKWrapper::GetFfxSurfaceFormat(GetFramework()->GetSwapChain()->GetSwapChainFormat());
ffx::ReturnCode retCode = ffx::CreateContext(frameGenContext, nullptr, createFg, createFgVersion, backendDesc);
```

<h3>Configure frame generation</h3>

Configure frame generation by filling out the [`ffxConfigureDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L110) structure with the required arguments and calling [`ffxConfigure`](../../api/include/ffx_api.h#L146).

This must be called once per frame. The frame ID must increment by exactly 1 each frame. Any other difference between consecutive frames will reset frame generation logic.

| [`ffxConfigureDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L110) member | Note                                                                                                                               |
|---------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| [`swapChain`](../../framegeneration/include/ffx_framegeneration.h#L113)                                           | The swapchain to use with frame generation. `IDXGISwapChain` on DirectX® 12, `VkSwapchainKHR` on Vulkan®.                                                                             |
| [`presentCallback`](../../framegeneration/include/ffx_framegeneration.h#L114)                                           | A UI composition callback to call when finalizing the frame image.                                                                             |
| [`presentCallbackUserContext`](../../framegeneration/include/ffx_framegeneration.h#L115)                                           | A pointer to be passed to the UI composition callback.                                                                             |
| [`frameGenerationCallback`](../../framegeneration/include/ffx_framegeneration.h#L116)                                           | The frame generation callback to use to generate a frame.                                                                             |
| [`frameGenerationCallbackUserContext`](../../framegeneration/include/ffx_framegeneration.h#L117)                                           | A pointer to be passed to the frame generation callback.                                                                             |
| [`frameGenerationEnabled`](../../framegeneration/include/ffx_framegeneration.h#L118)                                           | Sets the state of frame generation. Set to false to disable frame generation.                                                                             |
| [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119)                                           | Sets the state of async workloads. Set to true to enable generation work on async compute.                                                                             |
| [`HUDLessColor`](../../framegeneration/include/ffx_framegeneration.h#L120)                                           | The hudless back buffer image to use for UI extraction from backbuffer resource. May be empty.                                                                             |
| [`flags`](../../framegeneration/include/ffx_framegeneration.h#L121)                                           | Zero or combination of flags from [`FfxApiDispatchFramegenerationFlags`](../../framegeneration/include/ffx_framegeneration.h#L51).                                                                             |
| [`onlyPresentGenerated`](../../framegeneration/include/ffx_framegeneration.h#L122)                                           | Set to true to only present generated frames.                                                                             |
| [`generationRect`](../../framegeneration/include/ffx_framegeneration.h#L123)                                           | The area of the backbuffer that should be used for generation in case only a part of the screen is used e.g. due to movie bars                                                                             |
| [`frameID`](../../framegeneration/include/ffx_framegeneration.h#L124)                                           | Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.                                                                             |


```C++
// Update frame generation config
FfxApiResource hudLessResource = SDKWrapper::ffxGetResourceApi(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), FFX_API_RESOURCE_STATE_COMPUTE_READ);

m_FrameGenerationConfig.frameGenerationEnabled = m_FrameInterpolation;
m_FrameGenerationConfig.flags                  = 0;
m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugTearLines ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugResetIndicators ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS : 0;
m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
m_FrameGenerationConfig.HUDLessColor = (s_uiRenderMode == 3) ? hudLessResource : FfxApiResource({});
m_FrameGenerationConfig.allowAsyncWorkloads = m_AllowAsyncCompute && m_EnableAsyncCompute;
// assume symmetric letterbox
m_FrameGenerationConfig.generationRect.left   = (resInfo.DisplayWidth - resInfo.UpscaleWidth) / 2;
m_FrameGenerationConfig.generationRect.top    = (resInfo.DisplayHeight - resInfo.UpscaleHeight) / 2;
m_FrameGenerationConfig.generationRect.width  = resInfo.UpscaleWidth;
m_FrameGenerationConfig.generationRect.height = resInfo.UpscaleHeight;
// For sample purposes only. Most applications will use one or the other.
if (m_UseCallback)
{
    m_FrameGenerationConfig.frameGenerationCallback = [](ffxDispatchDescFrameGeneration* params, void* pUserCtx) -> ffxReturnCode_t
    {
        return ffxDispatch(reinterpret_cast<ffxContext*>(pUserCtx), &params->header);
    };
    m_FrameGenerationConfig.frameGenerationCallbackUserContext = &m_FrameGenContext;
}
else
{
    m_FrameGenerationConfig.frameGenerationCallback = nullptr;
    m_FrameGenerationConfig.frameGenerationCallbackUserContext = nullptr;
}
m_FrameGenerationConfig.onlyPresentGenerated = m_PresentInterpolatedOnly;
m_FrameGenerationConfig.frameID = m_FrameID;

#if defined(_DX12)
m_FrameGenerationConfig.swapChain = GetSwapChain()->GetImpl()->DX12SwapChain();
#elif defined(_VK)
m_FrameGenerationConfig.swapChain = GetSwapChain()->GetImpl()->VKSwapChain();
#endif

ffx::ReturnCode retCode = ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);
CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Configuring FSR FG failed: %d", (uint32_t)retCode);
```

If using the frame generation callback, the swapchain will call the callback with appropriate parameters.
Otherwise, the application is responsible for calling the frame generation dispatch and setting parameters itself.
In that case, the frame ID must be equal to the frame ID used in configuration. The command list and output texture can be queried from the frame generation context using [`ffxQuery`](../../api/include/ffx_api.h#L152). See the [sample code](../../../../Samples/Upscalers/FidelityFX_FSR/internal/fsrapirendermodule.cpp#L1897) for an example.

The user context pointers will only be passed into the respective callback functions. AMD FSR™ code will not attempt to dereference them.

When [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is set to `false` the main graphics queue will be used to execute the Optical Flow and Frame Generation workloads. It is strongly advised to profile, if significant performance benefits can be gained from asynchronous compute usage. Not using asynchronous compute will result in a lower memory overhead.

Note that UI composition and presents will always get executed on an async queue, so they can be paced and injected into the middle of the workloads generating the next frame.

![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/02_FSR3_Flow.svg "A diagram showing the AMD FSR™ Frame Generation workflow when not using async compute")

When [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is set to `true`, the Optical Flow and Frame Generation workloads will run on an asynchronous compute queue and overlap with workloads of the next frame on the main game graphics queue. This can improve performance depending on the GPU and workloads.

![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/03_FSR3_FlowAsyncQueue.svg "A diagram showing the AMD FSR™ Frame Generation workflow when using async compute")

<h3>Distortion Field</h3>

When an application applies post-processing distortion effects to the color buffer, the distorted color data no longer aligns spatially with the corresponding motion vectors and depth data. This would result in "ghosting" artifacts in the interpolated frame.

To address this, the application can configure the frame generation context with [`distortionField`](../../framegeneration/include/ffx_framegeneration.h#L180) linked after [`ffxConfigureDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L110) structure and calling [`ffxConfigure`](../../api/include/ffx_api.h#L146). 

**Distortion Field Format:**
- **2-component (R,G) texture** containing UV offset data
- **Each pixel encodes**: `[UV after distortion] - [UV before distortion]`
- **Frame Generation shaders** use these offsets to correct depth and motion vector lookups

<h3>UI Composition</h3>

For AMD FSR™ Frame Generation the user interface will require some special treatment, otherwise very noticeable artifacts will be generated which can impact readability of the interface. 

To prohibit those artifacts frame-generation supports various options to handle the UI:

The preferred method is to use the [`presentCallback`](../../framegeneration/include/ffx_framegeneration.h#L114). The function provided in this parameter will get called once for every frame presented and allows the application to schedule the GPU workload required to render the UI. By using this function the application can reduce UI input latency and render effects that do not work well with frame generation (e.g. film grain).

The UI composition callback function will be called for every frame (real or generated) to allow rendering the UI separately for each presented frame, so the UI can get rendered at presentation rate to achieve smooth UI animations.

[`commandList`](../../framegeneration/include/ffx_framegeneration.h#L95) is an `ID3D12GraphicsCommandList` on DirectX® 12 and a `VkCommandBuffer` on Vulkan®. [`currentBackBuffer.resource`](../../framegeneration/include/ffx_framegeneration.h#L84) and [`outputSwapChainBuffer.resource`](../../framegeneration/include/ffx_framegeneration.h#L86) are `ID3D12Resource` on DirectX® 12 and `VkImage` on Vulkan®. [`device`](../../framegeneration/include/ffx_framegeneration.h#L82) is an `ID3D12Device` on DirectX® 12 and a `VkDevice` on Vulkan®.

DirectX 12:

```C++
ffxReturnCode_t FSR3RenderModule::UiCompositionCallback(ffxCallbackDescFrameGenerationPresent* params, void* userCtx)
{
    ID3D12GraphicsCommandList2* pDxCmdList  = reinterpret_cast<ID3D12GraphicsCommandList2*>(params->commandList);
    ID3D12Resource*             pRtResource = reinterpret_cast<ID3D12Resource*>(params->outputSwapChainBuffer.resource);
    ID3D12Resource*             pBbResource = reinterpret_cast<ID3D12Resource*>(params->currentBackBuffer.resource);

    // Use pDxCmdList to copy pBbResource and render UI into the outputSwapChainBuffer.
    // The backbuffer is provided as SRV so postprocessing (e.g. adding a blur effect behind the UI) can easily be applied

    return FFX_API_RETURN_OK;
}
```

Vulkan:

```C++
ffxReturnCode_t FSR3RenderModule::UiCompositionCallback(ffxCallbackDescFrameGenerationPresent* params, void* userCtx)
{
    VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(params->commandList);
    VkImage         rtImage    = reinterpret_cast<VkImage>(params->outputSwapChainBuffer.resource);
    VkImage         bbImage    = reinterpret_cast<VkImage>(params->currentBackBuffer.resource);

    // Use pCmdBuffer to copy bbImage and render UI into the output swapchain image.
    // The backbuffer is provided in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL so postprocessing
    // (e.g. adding a blur effect behind the UI) can easily be applied.

    return FFX_API_RETURN_OK;
}
```

![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/04_FSR3_UiCallback.svg "A diagram showing the AMD FSR™ Frame Generation workflow when using async compute")

If frame generation is disabled [`presentCallback`](../../framegeneration/include/ffx_framegeneration.h#L114) will still get called on present.
![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/05_FSR3_NoFG_UiCallback.svg "A diagram showing the AMD FSR™ Frame Generation workflow when using async compute")

The second option to handle the UI is to render the UI into a dedicated surface that will be blended onto the interpolated and real backbuffer before present. Composition of this surface can be done automatically by the proxy swapchain or manually in the [`presentCallback`](../../framegeneration/include/ffx_framegeneration.h#L114). This method allows to present a UI unaffected by AMD FSR™ Frame Generation, however the UI will only be rendered at render rate. For applications with a largely static UI this might be a good solution without the additional overhead of rendering the UI at presentation rate.

![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/06_FSR3_UiTex.svg "A diagram showing the AMD FSR™ Frame Generation workflow when using async compute")

If frame generation is disabled and the UI Texture is provided, UI composition will still get executed by the AMD FSR™ Frame Generation (Swapchain).
![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/07_FSR3_NoFG_UiTex.svg "A diagram showing the AMD FSR™ Frame Generation workflow when using async compute")

In that case the surface needs to be registered to the swapchain by calling [`ffxConfigure`](../../api/include/ffx_api.h#L146) with a [`ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12`](../../framegeneration/include/dx12/ffx_api_framegeneration_dx12.h#L64) structure on DirectX 12, or a [`ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceVK`](../../framegeneration/include/vk/ffx_api_framegeneration_vk.h#L49) structure on Vulkan.

Flags in that structure control the following:

| [`FfxApiUiCompositionFlags`](../../framegeneration/include/ffx_framegeneration.h#L62) member | Note                                                                                                                               |
|---------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| [`FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_USE_PREMUL_ALPHA`](../../framegeneration/include/ffx_framegeneration.h#L64)                                           | A bit indicating that we use premultiplied alpha for UI composition.                                                                             |
| [`FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING`](../../framegeneration/include/ffx_framegeneration.h#L65)                                           | When set, the SDK manages the UI texture lifetime — the application can safely reuse the UI texture immediately after `Present()` returns. When not set, the application must keep the UI texture valid until async composition of the current frame is complete (see [Resource Lifetime](#resource-lifetime)). |

```C++
FfxResource uiColor = ffxGetResource(m_pUiTexture[m_curUiTextureIndex]->GetResource(), L"FSR3_UiTexture", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
#if defined(_DX12)
ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 uiConfig{};
#elif defined(_VK)
ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceVK uiConfig{};
#endif
uiConfig.uiResource = uiColor;
uiConfig.flags      = m_DoublebufferInSwapchain ? FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING : 0;
ffx::Configure(m_SwapChainContext, uiConfig);
```

The final method to handle the UI is to provide a [`HUDLessColor`](../../framegeneration/include/ffx_framegeneration.h#L289) surface in the [`FfxFrameGenerationConfig`](../../framegeneration/include/ffx_framegeneration.h#L278). This surface will get used during AMD FSR™ Frame Generation to detect the UI and avoid distortion on UI elements. This method has been added for compatibility with engines that can not apply either of the other two options for UI rendering.

![AMD FSR™ Frame Generation non async workflow](media/super-resolution-interpolation/08_FSR3_HUDLess.svg "A diagram showing the AMD FSR™ Frame Generation workflow when using async compute")

<h4>Different HUDless Formats</h4>

An optional structure [`ffxCreateContextDescFrameGenerationHudless`](../../framegeneration/include/ffx_framegeneration.h#L185) can be linked to the [`pNext`](../../api/include/ffx_api.h#L57) of the [`ffxCreateContextDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L69) used at context-creation time to enable the application to use a different [`hudlessBackBufferFormat`](../../framegeneration/include/ffx_framegeneration.h#L188) (e.g. RGBA8_UNORM) from [`backBufferFormat`](../../framegeneration/include/ffx_framegeneration.h#L75) (e.g. BGRA8_UNORM).


<h3>Dispatch frame generation preparation</h3>

Since version 3.1.0, frame generation runs independently of AMD FSR™ Upscaling. To replace the resources previously shared with the upscaler, a new frame generation prepare pass is required.

After the call to [`ffxConfigure`](../../api/include/ffx_api.h#L146), fill out a [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225) structure. Then provide the structure and frame generation context in a call to [`ffxDispatch`](../../api/include/ffx_api.h#L157).

| [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225) member | Note                                                                                                                               |
|---------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| [`frameID`](../../framegeneration/include/ffx_framegeneration.h#L229)                                           | Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic. Set the frameID to the same value as in the [`ffxConfigureDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L110) structure.                                                                             |
| [`flags`](../../framegeneration/include/ffx_framegeneration.h#L230)                                           | Zero or combination of values from [`FfxApiDispatchFramegenerationFlags`](../../framegeneration/include/ffx_framegeneration.h#L51).                                                                             |
| [`commandList`](../../framegeneration/include/ffx_framegeneration.h#L231)                                           | A command list to record frame generation commands into. `ID3D12GraphicsCommandList` on DirectX® 12, `VkCommandBuffer` on Vulkan®.                                                                             |
| [`renderSize`](../../framegeneration/include/ffx_framegeneration.h#L232)                                           | The dimensions used to render game content, dilatedDepth, dilatedMotionVectors are expected to be of this size.                                                                             |
| [`jitterOffset`](../../framegeneration/include/ffx_framegeneration.h#L233)                                           | The subpixel jitter offset applied to the camera.                                                                             |
| [`motionVectorScale`](../../framegeneration/include/ffx_framegeneration.h#L234)                                           | The scale factor to apply to motion vectors.                                                                             |
| [`frameTimeDelta`](../../framegeneration/include/ffx_framegeneration.h#L236)                                           | Time elapsed in milliseconds since the last frame.                                                                            |
| [`reset`](../../framegeneration/include/ffx_framegeneration.h#L237)                                           | A boolean value which when set to true, indicates FrameGeneration will be called in reset mode.                                                                             |
| [`cameraNear`](../../framegeneration/include/ffx_framegeneration.h#L238)                                           | The distance to the near plane of the camera.                                                                             |
| [`cameraFar`](../../framegeneration/include/ffx_framegeneration.h#L239)                                           | The distance to the far plane of the camera. This is used only in case of non-infinite depth.                                                                             |
| [`cameraFovAngleVertical`](../../framegeneration/include/ffx_framegeneration.h#L240)                                           | The camera angle field of view in the vertical direction (expressed in radians).                                                                             |
| [`viewSpaceToMetersFactor`](../../framegeneration/include/ffx_framegeneration.h#L241)                                           | The scale factor to convert view space units to meters.                                                                             |
| [`depth`](../../framegeneration/include/ffx_framegeneration.h#L242)                                           | The depth buffer data.                                                                             |
| [`motionVectors`](../../framegeneration/include/ffx_framegeneration.h#L243)                                           | The motion vector data.                                                                             |
| [`cameraPosition`](../../framegeneration/include/ffx_framegeneration.h#L245)                                           | The camera position in world space.                                                                             |
| [`cameraUp`](../../framegeneration/include/ffx_framegeneration.h#L246)                                           | The camera up normalized vector in world space.                                                                             |
| [`cameraRight`](../../framegeneration/include/ffx_framegeneration.h#L247)                                           | The camera right normalized vector in world space.                                                                             |
| [`cameraForward`](../../framegeneration/include/ffx_framegeneration.h#L248)                                           | The camera forward normalized vector in world space.                                                                             |

For fields also found in [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87), the same input requirements and recommendations apply here.

Set the frameID to the same value as in the configure description.

It is required to specify [`cameraPosition`](../../framegeneration/include/ffx_framegeneration.h#L245), [`cameraUp`](../../framegeneration/include/ffx_framegeneration.h#L246), [`cameraRight`](../../framegeneration/include/ffx_framegeneration.h#L247) and [`cameraForward`](../../framegeneration/include/ffx_framegeneration.h#L248) which must contain valid information about the camera position and orientation within the scene.

<h3>Dispatch frame generation</h3>

In order to optimally invoke [`ffxDispatch`](../../api/include/ffx_api.h#L157) for frame-generation it is *strongly* recommended that the caller supply a callback to [`frameGenerationCallback`](../../framegeneration/include/ffx_framegeneration.h#L116) along with any necessary context pointer in [`frameGenerationCallbackUserContext`](../../framegeneration/include/ffx_framegeneration.h#L117). This callback should execute [`ffxDispatch`](../../api/include/ffx_api.h#L157) using the provided [`ffxDispatchDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L92) structure pointer. This will then be written into the proper command-list during the swap-chain presentation and when [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is enabled, overlapped with other work.

It is possible, but *highly* discouraged to dispatch frame-generation manually. This infrastructure exists to support specific game-engine integrations such as the Unreal Engine plugin where dispatching during swapchain presentation is unsafe. In this case the caller must acquire an interpolation command buffer using the [`ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12`](../../framegeneration/include/dx12/ffx_api_framegeneration_dx12.h#L72) structure on DirectX® 12, or [`ffxQueryDescFrameGenerationSwapChainInterpolationCommandListVK`](../../framegeneration/include/vk/ffx_api_framegeneration_vk.h#L57) on Vulkan®, on the swap-chain context. `pOutCommandList` receives an `ID3D12GraphicsCommandList` on DirectX® 12 and a `VkCommandBuffer` on Vulkan®. The destination image is queried with [`ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12`](../../framegeneration/include/dx12/ffx_api_framegeneration_dx12.h#L79) or [`ffxQueryDescFrameGenerationSwapChainInterpolationTextureVK`](../../framegeneration/include/vk/ffx_api_framegeneration_vk.h#L64). Supply the command buffer as the [`commandList`](../../framegeneration/include/ffx_framegeneration.h#L95) in a [`ffxDispatchDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L92) which can then be used to dispatch via [`ffxDispatch`](../../api/include/ffx_api.h#L157). In this case the caller is responsible for understanding whether it is safe to enable [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119).

| [`ffxDispatchDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L92) member | Note                                                                                                                               |
|---------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| [`commandList`](../../framegeneration/include/ffx_framegeneration.h#L95)                                           | The command list on which to register render commands. `ID3D12GraphicsCommandList` on DirectX® 12, `VkCommandBuffer` on Vulkan®.                                                                             |
| [`presentColor`](../../framegeneration/include/ffx_framegeneration.h#L96)                                           | The current presentation color, this will be used as source data.                                                                            |
| [`outputs`](../../framegeneration/include/ffx_framegeneration.h#L97)                                           | Destination targets (1 for each frame in numGeneratedFrames).                                                                             |
| [`numGeneratedFrames`](../../framegeneration/include/ffx_framegeneration.h#L98)                                           | The number of frames to generate from the passed in color target.                                                                            |
| [`reset`](../../framegeneration/include/ffx_framegeneration.h#L99)                                           | A boolean value which when set to true, indicates the camera has moved discontinuously.                                                                            |
| [`backbufferTransferFunction`](../../framegeneration/include/ffx_framegeneration.h#L100)                                           | The transfer function used to convert frame generation source color data to linear RGB. One of the values from [`FfxApiBackbufferTransferFunction`](../../api/include/ffx_api_types.h#L132).                                                                             |
| [`minMaxLuminance`](../../framegeneration/include/ffx_framegeneration.h#L101)                                           | Min and max luminance values, used when converting HDR colors to linear RGB.                                                                             |
| [`generationRect`](../../framegeneration/include/ffx_framegeneration.h#L102)                                           | The area of the backbuffer that should be used for generation in case only a part of the screen is used e.g. due to movie bars.                                                                             |
| [`frameID`](../../framegeneration/include/ffx_framegeneration.h#L103)                                           | Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.                                                                             |

<h3>Input color requirements</h3>

Input color dimension to AMD FSR™ Frame Generation needs to be [`displaySize`](../../framegeneration/include/ffx_framegeneration.h#L73). Recall that input color comes from swapchain back buffer if using [`frameGenerationCallback`](../../framegeneration/include/ffx_framegeneration.h#L116) or [`presentColor`](../../framegeneration/include/ffx_framegeneration.h#L96) if manually dispatching frame generation.

If the upscaled (or rendered, if no upscaling) image is not the same dimension as `displaySize`, it must be composited before passing as input color to AMD FSR™ Frame Generation. 
 
The upscaled image composited in the back buffer must have the same aspect ratio as [`renderSize`](../../framegeneration/include/ffx_framegeneration.h#L232). 

<h3>Letterboxing or pillarboxing</h3>

Recall that the interpolated pixel color comes from lookup where the interpolated pixel would be in the previous and current back buffers using motion vector. When an application presents an image with cinematic bars (i.e. letterboxing or pillarboxing), the lookup pixel position in the back buffer can be inside the black bars region resulting in image artifacts in the interpolated image, i.e. color region spills into the black bar region, or black lines appear inside the color region. To fix this, the application needs to fill out [`generationRect`](../../framegeneration/include/ffx_framegeneration.h#L123), to tell AMD FSR™ Frame Generation, the area of back buffer containing rendered content without black bars. At each frame, the `left` and `top` values can vary. But, current API does not support `width` and `height` values different than the upscaler output size. AMD FSR™ Frame Generation assumes the top-left pixel of `generationRect` maps to the top-left texel in [`depth`](../../framegeneration/include/ffx_framegeneration.h#L242) and [`motionVectors`](../../framegeneration/include/ffx_framegeneration.h#L243). The current API does not describe how to offset into the depth and motion vector textures when `generationRect` size is smaller than upscaler output size.

> **Example supported usage**  
> Suppose upscaler output is 3840x2160 (16:9) without black bars and is displayed on 3840x2160 (16:9) sized back buffer. Then, it is expected to set `generationRect` to cover the 3840x2160 region.

> **Example supported usage**  
> Suppose upscaler output is 3840x1080 (32:9) without black bars. Upscaler output is composed to a 3840x2160 (16:9) sized back buffer and centered vertically, leaving black bars at the top and bottom. Then it is correct to set `generationRect` to only cover the 3840x1080 region. AMD FSR™ Frame Generation has correct mapping where top-left pixel of `generationRect` with UV coordinate (0, 0.25) in backbuffer maps to UV coordinate (0, 0) in the [`depth`](../../framegeneration/include/ffx_framegeneration.h#L242) and [`motionVectors`](../../framegeneration/include/ffx_framegeneration.h#L243). 

> **Example unsupported usage**  
> Suppose upscaler output is 3840x2160 (16:9) without black bar. Black bar is drawn over the upscaler output color such that the 3840x2160 (16:9) sized back buffer has now a 32:9 image (3840x1080) centered vertically, with black bars at the top and bottom. If set `generationRect` to be 3840x1080 colored region, then the assumed mapping of `generationRect` to depth and motion vector is wrong for every pixel, resulting in "ghosting" artifacts in the non-black region. If set `generationRect` to be 3840x2160, then the interpolated frame will have image artifacts due to lookup inside the black region.

<h3>Shutdown</h3>

Before destroying the Frame Generation context:
- Disable frame generation and UI composition on the proxy swap chain via `ffx::Configure`.
- This configure call waits for the proxy swap chain to complete GPU work (interpolation and UI composition) that references Frame Generation resources.
- Then destroy the Frame Generation context with [`ffxDestroyContext`](../../api/include/ffx_api.h#L140) to release all resources it created.

```C++
if (m_FrameGenContext != nullptr)
{
    // Disable frame generation before destroying context.
    // Internally calls waitForPresents(), which flushes interpolation and UI composition GPU work.
    // On DirectX 12 this avoids D3D12 debug layer error #921: OBJECT_DELETED_WHILE_STILL_IN_USE.
    ffx::ConfigureDescFrameGeneration m_FrameGenerationConfig;
    m_FrameGenerationConfig.frameGenerationEnabled = false;
#if defined(_DX12)
    m_FrameGenerationConfig.swapChain              = GetSwapChain()->GetImpl()->DX12SwapChain();
#elif defined(_VK)
    m_FrameGenerationConfig.swapChain              = GetSwapChain()->GetImpl()->VKSwapChain();
#endif
    m_FrameGenerationConfig.presentCallback        = nullptr;
    m_FrameGenerationConfig.HUDLessColor           = FfxApiResource({});
    ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);

#if defined(_VK)
    // Drop the registered UI image so the proxy stops referencing it.
    ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceVK uiConfig{};
    uiConfig.uiResource = {};
    ffx::Configure(m_SwapChainContext, uiConfig);
#endif

    ffx::DestroyContext(m_FrameGenContext);
    m_FrameGenContext = nullptr;
}
```

Failure to flush the interpolation GPU work from submitted frames can result in D3D12 debug layer error EXECUTION ERROR #921: OBJECT_DELETED_WHILE_STILL_IN_USE on frame generation created resources during the callstack of `ffxDestroyContext` on the frame generation context. On Vulkan®, the same missing flush is reported by validation as images or command buffers still in use. Before recreating the Vulkan® swapchain context, dispatch [`ffxDispatchDescFrameGenerationSwapChainWaitForPresentsVK`](../../framegeneration/include/vk/ffx_api_framegeneration_vk.h#L71). 

<h3>Thread safety</h3>

The underlying context is not guaranteed to be thread safe. Several public APIs that operate on `FrameGenContext` and `SwapChainContext` are not thread-safe and must be externally synchronized. Race condition symptoms can include access violation crashes, interpolation visual artifacts, and infinite waits in the `Dx12CommandPool` destructor on DirectX® 12, or the `VulkanCommandPool` destructor on Vulkan®, when releasing the swapchain.

When `AMD FSR™ Frame Generation (Swapchain)` `::present` is configured with frame generation callback, it accesses both `SwapChainContext` and `FrameGenContext`. A race can occur if application threads call `::present` concurrently with either `Dispatch(m_FrameGenContext, DispatchDescFrameGenerationPrepareV2)` or `DestroyContext(SwapChainContext)`.

APIs that require a mutex guard:
- [`ffxCreateContext`](../../api/include/ffx_api.h#L134) for the Frame Generation context
- [`ffxDestroyContext`](../../api/include/ffx_api.h#L140) for the Frame Generation context
- [`ffxDispatch`](../../api/include/ffx_api.h#L157) [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225) structure
- [`ffxDispatch`](../../api/include/ffx_api.h#L157) [`ffxDispatchDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L92) structure if `AMD FSR™ Frame Generation (Swapchain)` `::present` is not configured with [`frameGenerationCallback`](../../framegeneration/include/ffx_framegeneration.h#L116).
- `AMD FSR™ Frame Generation (Swapchain)` `::present` if app supplied a valid [`frameGenerationCallback`](../../framegeneration/include/ffx_framegeneration.h#L116) within [`ffxConfigureDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L110) structure.
- `AMD FSR™ Frame Generation (Swapchain)` swapchain destructor. See [`Shutdown`](../techniques/frame-interpolation-swap-chain.md#shutdown).


```C++
std::mutex g_FGContextMutex;

struct FGContextScopeLock
{
    explicit FGContextScopeLock(bool doLock) : m_locked(doLock)
    {
        if (m_locked) g_FGContextMutex.lock();
    }
    ~FGContextScopeLock()
    {
        if (m_locked) g_FGContextMutex.unlock();
    }
private:
    bool m_locked;
};

ffx::Context m_FrameGenContext  = nullptr;
ffx::Context m_SwapChainContext = nullptr;
ffx::ConfigureDescFrameGeneration m_FrameGenerationConfig{};

{
    // Acquire lock before creating FG context
    cauldron::FGContextScopeLock lock(true);
    ffx::CreateContextDescFrameGeneration createFg{};
#if defined(_DX12)
    ffx::CreateBackendDX12Desc backendDesc{};
    backendDesc.device = GetDevice()->GetImpl()->DX12Device();
#elif defined(_VK)
    ffx::CreateBackendVKDesc backendDesc{};
    VkDeviceContext deviceContext{};
    deviceContext.vkDevice         = GetDevice()->GetImpl()->VKDevice();
    deviceContext.vkPhysicalDevice = GetDevice()->GetImpl()->VKPhysicalDevice();
    deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
    backendDesc.deviceContext      = deviceContext;
#endif
    ffx::CreateContextDescFrameGenerationVersion headerVersion{};
    headerVersion.version = FFX_FRAMEGENERATION_VERSION;
    ffx::CreateContext(m_FrameGenContext, nullptr, createFg, backendDesc, headerVersion);
}
{
    // Acquire lock before dispatching FG Prepare
    cauldron::FGContextScopeLock lock(true);
    ffx::DispatchDescFrameGenerationPrepareV2 dispatchFgPrep{};
    ffx::Dispatch(m_FrameGenContext, dispatchFgPrep);
}
{
    // Acquire lock if app dispatches frame generation directly
    cauldron::FGContextScopeLock lock(true);
    ffx::DispatchDescFrameGeneration dispatchFg{};
    ffx::Dispatch(m_FrameGenContext, dispatchFg);
}
{
    // Acquire lock only if the proxy swapchain present operation uses the FG callback to dispatch frame generation.
    const bool bPresentWillDispatchFG = cauldron::GetFramework()->IsFrameGenerationSwapchain() && cauldron::GetFramework()->FrameInterpolationEnabled() && cauldron::GetFramework()->IsSwapchainUsingFrameGenerationCallback();
    cauldron::FGContextScopeLock fgLock(bPresentWillDispatchFG);
#if defined(_DX12)
    pSwapChain->GetImpl()->m_pSwapChain->Present(1, 0);
#elif defined(_VK)
    // Present through pOutQueuePresentKHR from ffxQueryDescSwapchainReplacementFunctionsVK.
    pOutQueuePresentKHR(presentQueue, &presentInfo);
#endif
}
{
    // Acquire lock before destroying FG context
    cauldron::FGContextScopeLock lock(true);
    m_FrameGenerationConfig.frameGenerationEnabled = false;
    ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);
    ffx::DestroyContext(m_FrameGenContext);
}
{
    // Acquire lock before destroying proxy swapchain
    cauldron::FGContextScopeLock lock(true);
#if defined(_DX12)
    IDXGISwapChain4*     pSwapChain4 = pSwapchain->GetImpl()->DX12SwapChain();
    // Release final reference triggers proxy swap chain destructor when refCount reaches 0.
    ULONG refCount = pSwapChain4->Release();
#elif defined(_VK)
    // Drop the engine handle, destroy the proxy context, clear the replacement
    // functions, then recreate a VkSwapchainKHR. See the Vulkan shutdown path
    // in the swapchain document.
    pSwapchain->GetImpl()->SetVKSwapChain(VK_NULL_HANDLE);
    ffx::DestroyContext(m_SwapChainContext);
    GetDevice()->GetImpl()->SetSwapchainContext(nullptr, nullptr, nullptr);
#endif
}

```

<h3>Resource Lifetime</h3>

<h4>When UiTexture composition mode is used</h4> 

<h5>If FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING is set:</h5>

The SDK takes ownership of the [`currentUI`](../../framegeneration/include/ffx_framegeneration.h#L85) texture at `Present()` time.
The application can safely reuse the UI texture **immediately after `Present()` returns** — no additional synchronization is required.

<h5>If FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING is not set:</h5>

The [`currentUI`](../../framegeneration/include/ffx_framegeneration.h#L85) texture remains in use by the SDK after `Present()` returns, until async composition of the current frame is complete.
This typically overlaps with the start of the next frame, so the application must not modify the UI texture during this period. The application must double-buffer the UI texture itself.

<h4>When HUDLess composition mode is used:</h4>

The HUDLess texture will be used during frame interpolation.
The application is responsible for ensuring it persists until frame interpolation is complete.
If [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is true:
Frame interpolation happens on an async compute queue, so the HUDLess texture needs to be double buffered by the application.
If [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is false:
Frame interpolation happens on the game GFX queue, so the app can safely modify the HUDLess texture in the next frame.

<h4>When distortionField texture is registered to frame interpolation:</h4>

The application is responsible for ensuring the [`distortionField`](../../framegeneration/include/ffx_framegeneration.h#L180) texture persists until frame interpolation is complete.
If [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is true:
Frame interpolation happens on an async compute queue, so the [`distortionField`](../../framegeneration/include/ffx_framegeneration.h#L180) texture needs to be double buffered by the application.
If [`allowAsyncWorkloads`](../../framegeneration/include/ffx_framegeneration.h#L119) is false:
Frame interpolation happens on the game GFX queue, so the app can safely modify the [`distortionField`](../../framegeneration/include/ffx_framegeneration.h#L180) texture in the next frame.

<h3>Debug Checker</h3>
Enable debug checker to validate application supplied inputs at dispatch upscale. It is recommended this is enabled only in development builds of the game.

Passing 
[`FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING`](../../framegeneration/include/ffx_framegeneration.h#L48) flag within [`FfxApiCreateContextFramegenerationFlags`](../../framegeneration/include/ffx_framegeneration.h#L40) will output textual warnings from frame generation to debugger TTY by default. Calling [`ffxConfigure`](../../api/include/ffx_api.h#L146) with [`fpMessage`](../../api/include/ffx_api.h#L85) within a [`ffxConfigureDescGlobalDebug1`](../../api/include/ffx_api.h#L204) structure to a suitable function allows the application to receive the debug messages issued. 

An example of the kind of output that can occur when debug checker observes possible issues is below:

FSR_API_DEBUG_WARNING: ffxDispatchDescFrameGenerationPrepareCameraInfo needs to be passed as linked struct. This is a required input to FSR3.1.4 and onwards for best quality.

<h3>Debug output</h3>

The AMD FSR™ Frame Generation API supports several debug visualization options:

When [`FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES`](../../framegeneration/include/ffx_framegeneration.h#L53) is set in the flags attribute of [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225), the inpainting pass will add bars of changing color on the left and right border of the interpolated image. This will assist visualizing if interpolated frames are getting presented and if the frames are presented with tearing enabled.

When [`FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS`](../../framegeneration/include/ffx_framegeneration.h#L54) is set in the flags attribute of [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225), the debug reset indicators will be drawn to the generated output.

When [`FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW`](../../framegeneration/include/ffx_framegeneration.h#L55) is set in the flags attribute of [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225), the [FrameInterpolationSwapChain](frame-interpolation-swap-chain.md) will only present interpolated frames and execute an additional pass to render debug data from internal surfaces onto the interpolated frame, to allow you to debug.

When [`FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES`](../../framegeneration/include/ffx_framegeneration.h#L57) is set in the flags attribute of [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225), the debug pacing lines will be drawn to the generated output.

![AMD FSR™ Frame Generation debug overlay](media/frame-interpolation/frame-interpolation-debug-overlay.svg "A diagram showing the debug overlay")

<h2>See also</h2>

- [AMD FSR™ Frame Generation 4.0.2](frame-interpolation-ml.md)
- [AMD FidelityFX™ Super Resolution Frame Generation 3.1.6](frame-interpolation.md)
- [AMD FSR™ Upscaling and Frame Generation Sample](../../../../docs/samples/super-resolution.md)
- [AMD FSR™ Naming guidelines](../getting-started/naming-guidelines.md)
