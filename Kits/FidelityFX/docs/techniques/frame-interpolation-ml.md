<!-- @page page_techniques_frame-interpolation-ml AMD FSR™ Frame Generation 4.0.2 -->

<h1>AMD FSR™ Frame Generation 4.0.2</h1>

<h2>Table of contents</h2>

- [Introduction](#introduction)
    - [Shading language requirements](#shading-language-requirements)
- [Integration](#integration)
    - [Performance](#performance)
    - [Memory Usage](#memory-usage)
    - [Call order](#call-order)
    - [Camera data](#camera-data)
    - [Debug View Unsupported](#debug-view-unsupported)
- [Limitations](#limitations)
- [Version history](#version-history)
- [See also](#see-also)

<h2>Introduction</h2>

**AMD FSR™ Frame Generation** is an advanced frame-generating solution that leverages state-of-the-art machine learning algorithms to generate a high-quality intermediate frame from two consecutive source images, interpolating the motion of pixels between the start & end images. 

The frame generation context computes the interpolated image. Once this is accomplished, the interpolated and real back buffers still need to be used, i.e. usually sent to the swapchain. On the topic of how to handle presentation and pacing of the back buffers, please refer to the [frame generation swapchain](frame-interpolation-swap-chain.md) documentation.

<h3>Shading language requirements</h3>

- `HLSL`
  - `CS_6_6†` for DirectX® 12
  - `SPIR-V Target: universal 1.5` for Vulkan® 

† `CS_6_6` requires DirectX 12 Agility SDK 1.4.9 or later.

<h2>Integration</h2>

AMD FSR™ Frame Generation should be integrated using the [AMD FSR™ Frame Generation API](frame-interpolation-api.md). This document describes API and other aspects specific to AMD FSR™ Frame Generation 4.0.2.

<h3>Performance</h3>
Depending on your target hardware and operating configuration AMD FSR™ Frame Generation will operate at different performance levels. Below you can find reference performance numbers for RX 9070 XT & RX 9060 XT running at recommended resolutions.

| GPU               | Target Resolution | Microseconds (ms)|
| -----------       | -----------       |------------------|
| RX 9070 XT        | 3840x2160         |  2.2ms           |
| RX 9060 XT        | 2560x1440         |  2.1ms           |

<h3>Memory Usage</h3>

Figures are given to the nearest MB, taken on Radeon RX 9070 XT using DirectX 12, and are subject to change. Does not include frame generation swapchain overheads.

| Output resolution | Total Memory usage(MB)          | Memory aliasable(MB)            |
|-------------------|---------------------------------|---------------------------------|
| 3840x2160         | 410                             | 360                             |
| 2560x1440         | 313                             | 284                             |
| 1920x1080         | 111                             | 90                              |

<h3>Call order</h3>

For AMD FSR™ Frame Generation 4.0.2 it is *required* that the application follow the specified call-order:
1. [`ffxConfigure`](../../api/include/ffx_api.h#L146) with a filled in [`ffxConfigureDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L110).
2. [`ffxDispatch`](../../api/include/ffx_api.h#L157) with a filled in [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225).
3. [`ffxDispatch`](../../api/include/ffx_api.h#L157) with a filled in [`ffxDispatchDescFrameGeneration`](../../framegeneration/include/ffx_framegeneration.h#L92).

With AMD FSR™ Frame Generation 4.0.2 this call order is mandatory as data provided in each call is required for the next to function correctly. Calling these functions out of order may result in incorrect rendering results and *prohibits* future automatic driver upgrades.

<h3>Camera data</h3>

AMD FSR™ Frame Generation 4.0.2 uses the [`cameraPosition`](../../framegeneration/include/ffx_framegeneration.h#L245), [`cameraUp`](../../framegeneration/include/ffx_framegeneration.h#L246), [`cameraRight`](../../framegeneration/include/ffx_framegeneration.h#L247) and [`cameraForward`](../../framegeneration/include/ffx_framegeneration.h#L248) fields within the [`ffxDispatchDescFrameGenerationPrepareV2`](../../framegeneration/include/ffx_framegeneration.h#L225) structure. Failing to specify these fields or providing incorrect or low-precision values may result in incorrect rendering.

<h3>Debug View Unsupported</h3>

The [`FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW`](../../framegeneration/include/ffx_framegeneration.h#L55) setting is not supported by AMD FSR™ Frame Generation 4.0.0+.

All other debug functions are provided.

<h2>Limitations</h2>

AMD FSR™ Frame Generation requires:

- Windows 11
- An AMD Radeon™ RX 9000 Series GPU or later
- One of the following graphics APIs:
  - DirectX® 12 with Agility SDK 1.4.9 or later
  - Vulkan® 1.2 or later

<h2>Version history</h2>

| Version        | Date              |
| ---------------|-------------------|
| **4.0.2**      | 2026-10-28        |
| **4.0.1**      | 2026-06-24        |
| **4.0.0**      | 2025-12-10        |

Refer to changelog for more detail on versions.

<h2>See also</h2>

- [AMD FSR™ Frame Generation API](frame-interpolation-api.md)
- [AMD FidelityFX™ Super Resolution Frame Generation 3.1.6](frame-interpolation.md)
- [AMD FidelityFX™ Super Resolution Frame Generation (Swapchain) 3.1.7](frame-interpolation-swap-chain.md)
- [AMD FSR™ Frame Generation (Swapchain) 4.0.0](frame-interpolation-swap-chain.md)
- [AMD FSR™ Upscaling and Frame Generation Sample](../../../../docs/samples/super-resolution.md)
- [AMD FSR™ Naming guidelines](../getting-started/naming-guidelines.md)
