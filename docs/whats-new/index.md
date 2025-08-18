<!-- @page page_whats-new_index AMD FidelityFX SDK: What's new in FidelityFX SDK 2.0.0 -->

<h1>What's new in the AMD FidelityFX™ SDK 2.0.0?</h1>

Welcome to the AMD FidelityFX SDK. This major revision to the SDK serves as a launching pad for all of our new and upcoming DLL-based ML technologies. We are pleased to present the first of these with the public release of FidelityFX FSR4.

<h2>New effects and features</h2>

<h3>AMD FidelityFX Super Resolution 4 (FSR 4.0.2)</h3>
Introducing our new inference-based FidelityFX Super Resolution 4, an advanced upscaling solution that leverages state-of-the-art machine learning algorithms to generate high-quality, high-resolution frames from lower-resolution inputs.

<h2>Updated effects</h2>

<h3>AMD FidelityFX Frameinterpolation Swapchain 3.1.5</h3>
Fixes out of command lists crash if create swapchain >2 frames of latency and using non-async Frame Interpolation.<br/>
Fixes an alt-tab deadlock scenario when Frame Generation is enabled.<br/>

<h3>AMD FidelityFX Super Resolution 3.1.5 (FSR 3.1.5)</h3>
Fix for possible negative rcas output.<br/>

<h3>AMD FidelityFX Super Resolution 2.3.4 (FSR 2.3.4)</h3>
Fix for possible negative rcas output.<br/>

<h3>AMD FidelityFX Super Resolution (FSR) API</h3>
* Minor non-API breaking additions:<br/>
ffxQueryDescUpscaleGetResourceRequirements for any upscaler provider to get info on what resources are required or optional. <br/>
ffxQueryDescUpscaleGetGPUMemoryUsageV2, ffxQueryDescFrameGenerationGetGPUMemoryUsageV2, and ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2 to get GPU memory requirement before effect context is created.<br/>

<h2>Updated Components</h2>

Starting with AMD FidelityFX™ SDK 2.0.0 the effects, previously combined in amd_fidelityfx_dx12.dll, are split into multiple DLLs based on effect type. Please see [Introduction to FidelityFX API](../getting-started/ffx-api.md#dlls-structure) for details.
<br/>
<br/>
PDBs are provided for the effects DLLs.

<h2>Updated documentation</h2>

* Simplified FSR 2, FSR 3 & Frame Generation documentation - previous documentation remains in AMD FidelityFX SDK 1.1.4.

<h2>Deprecated effects</h2>

None.

<h2>Deprecated components</h2>

All SDK version 1 effects are now deprecated to that version of the SDK. 
For any pre-existing FidelityFX features (including the legacy FidelityFX Super Resolution sample), please refer to FidelityFX SDK 1.1.4.

<!-- - @subpage page_whats-new_index_2_0_0 "AMD FidelityFX SDK: What's new in FidelityFX SDK 2.0.0" -->