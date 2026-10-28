<!-- @page page_whats-new_index AMD FSR™ SDK: What's new in AMD FSR™ SDK 2.4.0 -->

<h1>What's new in the AMD FSR™ SDK 2.4.0?</h1>

Welcome to the AMD FSR™ SDK. This revision to the SDK updates a number of AMD FSR™ "Redstone" DLL-based ML technologies. 

<h2>New effects and features</h2>

<h3>AMD FSR™ Frame Generation (Swapchain) 4.0.0</h3>

- Driver-Assisted Frame Pacing on DirectX® 12 and Vulkan®. Requires an AMD Radeon™ RX 9000 Series GPU or later and AMD Software: Adrenalin Edition™ driver version 26.10.1 or later. See the [Frame pacing and presentation](../techniques/frame-interpolation-swap-chain.md#frame-pacing-and-presentation) section for more details;
- Intel® PresentMon integration for Frame Generation (DX12 only);
- Vulkan® support at feature parity with DX12 version;

<h2>Updated effects</h2>

<h3>AMD FSR™ Upscaling 4.1.2</h3>

- Added support for AMD FSR™ Upscaling 4.1 on the following hardware:
  - AMD Radeon™ RX 6000 Series discrete GPUs (AMD RDNA™ 2 architecture);
  - AMD Ryzen™ AI Max 300 Series processors ("Strix Halo");
  - AMD Ryzen™ AI Max 400 Series processors ("Gorgon Halo");
- Added support for highly optimized AMD FSR™ Upscaling 4.1 on the following hardware:
  - AMD Ryzen™ AI 9 300 Series processors ("Strix Point");
  - AMD Ryzen™ AI 9 400 Series processors ("Gorgon Point");
  - AMD Ryzen™ Z2 A processors (AMD RDNA™ 2 architecture iGPU);
  - AMD Ryzen™ AI Z2 Extreme processor ("Strix Point");
  - AMD Ryzen™ Z2 Extreme processor ("Strix Point");
  - These select AMD Ryzen™ processors with Radeon™ graphics use a new, highly optimized ML model that is less than half the size of the original INT8 model and purpose-built for this class of hardware. This lightweight model brings ML-based AMD FSR™ Upscaling to laptops and handhelds powered by these processors, with a balance of image reconstruction quality and performance optimized for these devices. The precise performance and quality characteristics can vary by game, resolution, quality mode, and hardware. AMD has focused on delivering a stable, broadly useful solution across all supported platforms;
- Vulkan® support at feature parity with DX12 version;

<h3>AMD FidelityFX™ Super Resolution 3.1.6</h3>

- Increased precision and refined reprojection in the accumulate pass.
- Vulkan® support at feature parity with DX12 version;

<h3>AMD FidelityFX™ Super Resolution Frame Generation 3.1.6</h3>

- Vulkan® support at feature parity with DX12 version;

<h3>AMD FSR™ Frame Generation 4.0.2</h3>

- Refactoring and stability improvements;
- Vulkan® support at feature parity with DX12 version;

<h3>AMD FSR™ Frame Generation (Swapchain) 3.1.7</h3>

- Vulkan® support at feature parity with DX12 version;

<h3>AMD FSR™ Ray Regeneration 1.2.1</h3>

- Standalone ambient occlusion denoising is supported;
- Standalone specular occlusion denoising is supported;
- Querying GPU memory usage after context creation is fixed;
- Querying GPU memory usage before context creation now includes shader visible descriptors;
- FFX_DENOISER_DISPATCH_NON_ENCODED_ALBEDO flag alias for FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO is added;
- FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO is deprecated and will be removed in a future release;

<h3>AMD FSR™ API</h3>

- Shader blob decompression support added (currently only used on Vulkan®);
- SDK-driver communication improvements;
- Effect DLLs are now also looked up next to the loader DLL, in addition to the application executable folder;
- AMD FSR™ Frame Generation (Swapchain) automatic driver override support for games based on AMD FSR™ SDK 2.1+ or AMD FidelityFX™ Super Resolution Frame Generation (Swapchain) 3.1.6+;

<h2>Updated Components</h2>

Starting with AMD FSR™ SDK 2.0.0 the effects, previously combined in amd_fidelityfx_dx12.dll, are split into multiple DLLs based on effect type. Please see [Introduction to AMD FSR™ API](../getting-started/ffx-api.md#dlls-structure) for details.

<h2>Updated documentation</h2>

* AMD FSR™ Upscaling 4.1.2 documentation.
* AMD FSR™ Frame Generation 4.0.2 documentation.
* AMD FSR™ Frame Generation (Swapchain) 4.0.0 documentation.
* AMD FSR™ Frame Generation (Swapchain) 3.1.7 documentation.
* AMD FSR™ Ray Regeneration 1.2.1 documentation.
* AMD FSR™ API documentation, covering driver-provided versions and version override behavior.

<h2>Deprecated effects</h2>

None.

<h2>Deprecated components</h2>

- ffxConfigureDescGlobalDebug1 is deprecated, use ffxConfigureDescGlobalDebug instead;

<!-- - @subpage page_whats-new_index_2_4_0 "AMD FSR™ SDK: What's new in AMD FSR™ SDK 2.4.0" -->
