<!-- @page page_whats-new_index AMD FidelityFX SDK: What's new in FidelityFX SDK 1.1.4 -->

<h1>What's new in the AMD FidelityFX™ SDK 1.1.4?</h1>

Welcome to the AMD FidelityFX SDK. This updated version of the SDK contains fixes to FSR3 and 2 components.

<h2>New effects and features</h2>

None.

<h2>Updated effects</h2>

<h3>AMD FidelityFX Frameinterpolation Swapchain 1.1.3</h3>

<h3>AMD FidelityFX Super Resolution 3.1.4 (FSR 3.1.4)</h3>
Reduces upscaler disocclusion ghosting by setting default fMinDisocclusionAccumulation to -0.333 from FSR 3.1.3 equivalent default of 0.333. <br/>
In small fraction of games, fMinDisocclusionAccumulation of -0.333 results in more white pixel flickering in swaying grass vs 0.333.

<br/>

<h3>AMD FidelityFX Super Resolution (FSR) API</h3>
* Minor non-API breaking additions:<br/>
ffxConfigureDescUpscaleKeyValue to tune upscaler ghosting. <br/>
ffxQueryGetProviderVersion to get version info from created ffx-api context.<br/>
ffxDispatchDescFrameGenerationPrepareCameraInfo is a required input to FSR 3.1.4 and onwards for best quality.<br/>
FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING at context creation to enable frame generation debug checker.<br/>
ffxConfigureDescGlobalDebug1 to assign fpMessage callback for debug checker to output textual messages. If fpMessage is nullptr, debug checker will output messages to debugger TTY.<br/>
<h2>Updated Components</h2>

<h3>FidelityFX Brixelizer GI 1.0.1</h3>
Fixes an assertion when start cascade == end cascade.

<h3>FidelityFX Breadcrumbs 1.0.1</h3>
Fixes an assertion during realloc when copying name string.

<h2>Updated documentation</h2>

<h2>Deprecated effects</h2>

None.

<h2>Deprecated components</h2>

None.

<!-- - @subpage page_whats-new_index_1_1_4 "AMD FidelityFX SDK: What's new in FidelityFX SDK 1.1.4" -->