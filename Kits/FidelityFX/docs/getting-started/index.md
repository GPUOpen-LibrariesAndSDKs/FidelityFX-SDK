<!-- @page page_getting-started_index Introduction to the AMD FSR™ SDK -->

<h1>Introduction to the AMD FSR™ SDK</h1>

The AMD FSR™ SDK is a collection of highly optimized effects (headers and DLLs) that developers can integrate into DirectX® 12 or Vulkan® applications to enhance performance and visual quality. The AMD FSR™ SDK includes:

- [AMD FidelityFX™ Super Resolution 2.3.4](../techniques/super-resolution-temporal.md)
- [AMD FidelityFX™ Super Resolution 3.1.6](../techniques/super-resolution-upscaler.md)
- [AMD FSR™ Upscaling 4.1.2](../techniques/super-resolution-ml.md)
- [AMD FidelityFX™ Super Resolution Frame Generation 3.1.6](../techniques/frame-interpolation.md)
- [AMD FidelityFX™ Super Resolution Frame Generation Swapchain 3.1.7](../techniques/frame-interpolation-swap-chain.md)
- [AMD FSR™ Frame Generation (Swapchain) 4.0.0](../techniques/frame-interpolation-swap-chain.md)
- [AMD FSR™ Frame Generation 4.0.2](../techniques/frame-interpolation-ml.md)
- [AMD FSR™ Ray Regeneration 1.2.1](../techniques/denoising.md)
- [AMD FSR™ Radiance Caching (Preview)](../techniques/radiance-cache.md)

<h2>Supported ecosystems</h2>

This version of the AMD FSR™ SDK comes with samples that run on the following APIs:

- DirectX® 12 (Agility SDK 1.616.1)
- Vulkan® 1.2

New APU support and Vulkan® support are provided by the SDK DLLs integrated with the application, and do not require a specific driver version.

Driver-Assisted Frame Pacing requires AMD Software: Adrenalin Edition™ driver version 26.10.1, an AMD Radeon™ RX 9000 Series GPU (AMD RDNA™ 4 architecture), and AMD FSR™ Frame Generation (Swapchain) 4.0.0, or newer in each case. The swapchain is included in this SDK, and titles integrated with AMD FSR™ SDK 2.1 or AMD FidelityFX™ Super Resolution Frame Generation (Swapchain) 3.1.6 and newer can also receive it through an automatic driver override.

Driver-Assisted Frame Pacing operates inside the frame generation swapchain's own present call, so the application must present through that swapchain. Integrations that dispatch frame generation manually and present through their own swapchain do not use it, for example if the AMD FSR™ Unreal® Engine plugin has `OverrideSwapChainDX12` set to `false`. Driver-Assisted Frame Pacing is optimized for Windows® 11 with `Hardware-accelerated GPU scheduling` enabled. See the [Frame pacing and presentation](../techniques/frame-interpolation-swap-chain.md#frame-pacing-and-presentation) section for details.

The shader code used by our sample framework is written in HLSL, and can easily be ported to other platforms which support modern shader models.

If you are a registered Xbox developer, you can find AMD FSR™ features available as part of the Microsoft Game Development Kit (GDK).

<h2>Samples</h2>

Most samples are written in C++, and use the [Cauldron Framework](../../../Cauldron2/framework/) for [DirectX® 12](../../../Cauldron2/dx12/) and [Vulkan®](../../../Cauldron2/vk/).

For sample build and run instructions, see the [Samples documentation](../../../../docs/getting-started/index.md).

<h2>Open source</h2>

The AMD FSR™ SDK is open source, and distributed under the MIT license.

For more information on the license terms please refer to the [license](../license.md).

<h2>Support</h2>

AMD maintains the AMD FSR™ SDK with regular updates, new features, bug fixes, and compatibility and performance testing across a wide range of hardware.

If you encounter issues or have feature requests, please consider opening an issue.

<!-- - @subpage page_getting-started_sdk-structure "SDK Structure" -->
<!-- - @subpage page_getting-started_naming-guidelines "AMD FSR™ naming guidelines for game applications" -->
<!-- - @subpage page_ffx-api "Introduction to AMD FSR™ API" -->
