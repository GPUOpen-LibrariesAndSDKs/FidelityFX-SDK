<!-- @page page_techniques_frame-interpolation AMD FidelityFX™ Super Resolution Frame Generation 3.1.6 -->

<h1>AMD FidelityFX™ Super Resolution Frame Generation 3.1.6</h1>

<h2>Table of contents</h2>

- [Introduction](#introduction)
    - [Shading language requirements](#shading-language-requirements)
- [Integration](#integration)
    - [Memory Usage](#memory-usage)
- [Limitations](#limitations)
- [Version history](#version-history)
- [See also](#see-also)

<h2>Introduction</h2>

**AMD FidelityFX™ Super Resolution Frame Generation** is a technique that analytically generates an intermediate frame from two consecutive source images, interpolating the motion of pixels between the start & end images. 

The frame generation context computes the interpolated image. Once this is accomplished, the interpolated and real back buffers still need to be used, i.e. usually sent to the swapchain. On the topic of how to handle presentation and pacing of the back buffers, please refer to the [frame generation swapchain](frame-interpolation-swap-chain.md) documentation.

<h3>Shading language requirements</h3>

- `HLSL`
  - `CS_6_2`
  - `CS_6_6†`

† `CS_6_6` is used on some hardware which supports 64-wide wavefronts.

<h2>Integration</h2>

AMD FidelityFX™ Super Resolution Frame Generation should be integrated using the [AMD FSR™ Frame Generation API](frame-interpolation-api.md). This document describes API and other aspects specific to AMD FidelityFX™ Super Resolution Frame Generation 3.1.6.

<h3>Memory Usage</h3>

Figures are given to the nearest MB, taken on Radeon RX 9070 XTX using DirectX 12, and are subject to change. Does not include frame generation swapchain overheads.

| Output resolution | Memory usage(MB)                |
|-------------------|---------------------------------|
| 3840x2160         | 457                             |
| 2560x1440         | 214                             |
| 1920x1080         | 124                             |

<h2>Limitations</h2>

AMD FidelityFX™ Super Resolution requires a GPU with typed UAV load and R16G16B16A16_UNORM support.

<h2>Version history</h2>

| Version        | Date              |
| ---------------|-------------------|
| **3.1.6**      | 2025-12-10        |
| **3.1.5**      | 2025-08-20        |
| **1.1.3**      | 2025-05-08        |
| **1.1.2**      | 2024-06-05        |
| **1.1.1**      | 2023-11-28        |

Refer to changelog for more detail on versions.

<h2>See also</h2>

- [AMD FSR™ Frame Generation API](frame-interpolation-api.md)
- [AMD FSR™ Frame Generation 4.0.2](frame-interpolation-ml.md)
- [AMD FidelityFX™ Super Resolution Frame Generation (Swapchain) 3.1.7](frame-interpolation-swap-chain.md)
- [AMD FSR™ Frame Generation (Swapchain) 4.0.0](frame-interpolation-swap-chain.md)
- [AMD FSR™ Upscaling and Frame Generation Sample](../../../../docs/samples/super-resolution.md)
- [AMD FSR™ Naming guidelines](../getting-started/naming-guidelines.md)
