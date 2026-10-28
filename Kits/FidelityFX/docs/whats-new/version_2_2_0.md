<!-- @page page_whats-new_index AMD FSR™ SDK: What's new in AMD FSR™ SDK 2.2.0 -->

<h1>What's new in the AMD FSR™ SDK 2.2.0?</h1>

Welcome to the AMD FSR™ SDK. This revision to the SDK updates a number of AMD FSR™ Redstone DLL-based ML technologies. 

<h2>New effects and features</h2>

- None

<h2>Updated effects</h2>

<h3>AMD FSR™ Upscaling 4.1.0</h3>

- Updated inference to improve quality in some titles.
- Performance optimizations to several shaders.

<h3>AMD FSR™ Ray Regeneration 1.1.0</h3>

- Quality and memory improvements
- Addition of debug view modes using ffxDispatchDescDenoiserDebugView (ffx::DispatchDescDenoiserDebugView)
- <b>API breaking additions</b>:
  - Dynamic configuration is now at the granularity of individual parameters:
    - FfxApiDenoiserSettings is replaced with FfxApiConfigureDenoiserKey
    - ffxConfigureDescDenoiserSettings (ffx::ConfigureDescDenoiserSettings) is replaced with ffxConfigureDescDenoiserKeyValue (ffx::ConfigureDescDenoiserKeyValue)
    - ffxQueryDescDenoiserGetDefaultSettings (ffx::QueryDescDenoiserGetDefaultSettings) is replaced with ffxQueryDescDenoiserGetDefaultKeyValue (ffx::QueryDescDenoiserGetDefaultKeyValue)
  - Type of ffxDispatchDescDenoiser::motionVectorScale is now FfxApiFloatCoords3D instead of FfxApiFloatCoords2D.
  - Alpha channel of ffxDispatchDescDenoiser::diffuseAlbedo is now unused.
  - Alpha channel of ffxDispatchDescDenoiser::specularAlbedo is now unused.
  - Alpha channel of ffxDispatchDescDenoiserInput1Signal::fusedAlbedo is now unused.
  - Alpha channel of ffxDispatchDescDenoiserInput4Signals::indirectSpecularRadiance::input, ffxDispatchDescDenoiserInput2Signals::specularRadiance::input, and ffxDispatchDescDenoiserInput1Signal::radiance::input now contain the specular ray length.

<h3>AMD FSR™ API</h3>
* Minor non-API breaking additions:<br/>

<h2>Updated Components</h2>

Starting with AMD FidelityFX™ SDK 2.0.0 the effects, previously combined in amd_fidelityfx_dx12.dll, are split into multiple DLLs based on effect type. Please see [Introduction to AMD FSR™ API](../getting-started/ffx-api.md#dlls-structure) for details.
<br/>
<br/>
PDBs are provided for the loader DLL.

<h2>Updated documentation</h2>

* AMD FSR™ Upscaling 4.1 documentation.
* AMD FSR™ Ray Regeneration documentation.

<h2>Deprecated effects</h2>

None.

<h2>Deprecated components</h2>

All SDK version 1 effects are now deprecated to that version of the SDK. 
For any pre-existing AMD FidelityFX™ features (including the legacy AMD FidelityFX™ Super Resolution sample), please refer to AMD FidelityFX™ SDK 1.1.4.

<!-- - @subpage page_whats-new_index_2_2_0 "AMD FSR™ SDK: What's new in AMD FSR™ SDK 2.2.0" -->