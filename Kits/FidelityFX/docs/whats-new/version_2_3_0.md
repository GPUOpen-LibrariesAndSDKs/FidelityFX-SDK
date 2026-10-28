<!-- @page page_whats-new_version_2_3_0 AMD FSR™ SDK: What's new in AMD FSR™ SDK 2.3.0 -->

<h1>What's new in the AMD FSR™ SDK 2.3.0?</h1>

Welcome to the AMD FSR™ SDK. This revision to the SDK updates a number of AMD FSR™ Redstone DLL-based ML technologies. 

<h2>New effects and features</h2>

- None

<h2>Updated effects</h2>

<h3>AMD FSR™ Ray Regeneration 1.2.0</h3>

- Quality improvements
- Optional add-on ambient occlusion denoising is added;
- Optional add-on specular occlusion denoising is added;
- FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS is added;
- FFX_DENOISER_ENABLE_VALIDATION flag is added;
- <b>API breaking additions</b>:
  - FfxApiDenoiserSignal::_reserved is removed;
  - ffxDispatchDescDenoiser::deltaTime is removed;
  - ffxQueryDescDenoiserGetVersion is removed:
    - API version is DLL version;
    - Provider version is embedded in provider version name;
  - Fuse mode API is removed:
    - FfxApiDenoiserMode is removed;
    - ffxCreateContextDescDenoiser::mode is removed;
    - ffxDispatchDescDenoiserInput1Signal (incl. Fused Albedo) is removed;
    - ffxDispatchDescDenoiserInput2Signals is removed;
    - ffxDispatchDescDenoiserInput4Signals is removed;
    - ffxQueryDescDenoiserGetGPUMemoryUsage::mode is removed;
  - Logging API is changed:
    - ffxCreateContextDescDenoiser::fpMessage is replaced with ffxConfigureDescGlobalDebug;
  - Matrix API is added:
    - ffxDispatchDescDenoiser::cameraRight, ffxDispatchDescDenoiser::cameraUp, ffxDispatchDescDenoiser::cameraForward, ffxDispatchDescDenoiser::cameraAspectRatio, ffxDispatchDescDenoiser::cameraNear, ffxDispatchDescDenoiser::cameraFar, and ffxDispatchDescDenoiser::cameraFovAngleVertical are replaced with ffxDispatchDescDenoiser::view and ffxDispatchDescDenoiser::projection;
    - ffxDispatchDescDenoiser::linearDepth now contains the signed linear depth instead of absolute linear depth;
    - ffxDispatchDescDenoiser::motionVectors::z now contains the signed linear depth delta instead of absolute linear depth delta;
    - ffxDispatchDescDenoiser::motionVectorScale::z now contains the signed linear depth delta scaling factor instead of absolute linear depth delta scaling factor;
  - Passthrough API is added:
    - ffxDispatchDescDenoiser::linearDepthBounds is added;
  - Reconstruction API is added:
    - FfxApiDenoiserSignal::checkerboardOrigin is added;
    - ffxCreateContextDescDenoiser::checkerboardSignalFlags is added;
    - ffxQueryDescDenoiserGetGPUMemoryUsage::checkerboardSignalFlags is added;
  - Signal API is added:
    - FFX_DENOISER_ENABLE_DOMINANT_LIGHT is replaced with FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY;
    - FfxApiDenoiserSignalFlags is added;
    - ffxCreateContextDescDenoiser::signalFlags is added;
    - ffxDispatchDescDenoiserAmbientOcclusion is added;
    - ffxDispatchDescDenoiserDirectDiffuse is added;
    - ffxDispatchDescDenoiserDirectSpecular is added;
    - ffxDispatchDescDenoiserIndirectDiffuse is added;
    - ffxDispatchDescDenoiserIndirectSpecular is added;
    - ffxDispatchDescDenoiserInputDominantLight is replaced with ffxDispatchDescDenoiserDominantLight;
    - ffxDispatchDescDenoiserSpecularOcclusion is added;
    - ffxQueryDescDenoiserGetGPUMemoryUsage::signalFlags is added;

<h3>AMD FSR™ Upscaling 4.1.1</h3>

- Added support for AMD FSR™ Upscaling 4.1 on AMD Radeon RX 7000 Series (AMD RDNA™ 3 architecture) discrete GPUs

<h3>AMD FSR™ Frame Generation 4.0.1</h3>

- Version updates for ML-based frame generation and Frame Generation Swapchain
- Fix motion vector pre-processing for generation rectangle
- Fix binding of camera data for motion vector pre-processing
- Redundant version check removals

<h3>AMD FidelityFX™ Super Resolution Frame Generation (SwapChain) 3.1.7</h3>

- Fix race condition on shutdown/disabling of Frame Generation with semaphore waitable object
- Fix to shutdown handling to prevent hanging resource pointers

<h3>AMD FSR™ API</h3>

* Minor non-API breaking additions:<br/>
- Added `ffxConfigureDescGlobalDebug` (type 7) — a new debug configuration struct with an `effectId` field enabling per-effect debug message routing.<br/>

<h2>Updated Components</h2>

Starting with AMD FidelityFX™ SDK 2.0.0 the effects, previously combined in amd_fidelityfx_dx12.dll, are split into multiple DLLs based on effect type. Please see [Introduction to AMD FSR™ API](../getting-started/ffx-api.md#dlls-structure) for details.
<br/>
<br/>
PDBs are provided for the loader DLL.

<h2>Updated documentation</h2>

* AMD FSR™ Ray Regeneration 1.2.0 documentation.
* AMD FSR™ Upscaling 4.1.1 documentation.
* AMD FSR™ Frame Generation 4.0.1 documentation.
* AMD FidelityFX™ Super Resolution Frame Generation 3.1.6 API documentation.
* AMD FidelityFX™ Super Resolution Frame Generation Swapchain 3.1.7 documentation.

<h2>Deprecated effects</h2>

None.

<h2>Deprecated components</h2>

All SDK version 1 effects are now deprecated to that version of the SDK. 
For any pre-existing AMD FidelityFX™ features (including the legacy AMD FidelityFX™ Super Resolution sample), please refer to AMD FidelityFX™ SDK 1.1.4.

<!-- - @subpage page_whats-new_index_2_3_0 "AMD FSR™ SDK: What's new in AMD FSR™ SDK 2.3.0" -->