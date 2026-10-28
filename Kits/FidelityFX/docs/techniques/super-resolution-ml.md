<!-- @page page_techniques_super-resolution-ml AMD FSR™ Upscaling 4.1.2 -->

<h1>AMD FSR™ Upscaling 4.1.2</h1>

![Screenshot](media/super-resolution-ml/fsr4-sample.jpg "A screenshot showcasing the final output of the effect")

AMD FSR™ Upscaling 4 is an advanced upscaling solution that leverages state-of-the-art machine learning algorithms to generate high-quality, high-resolution frames from lower-resolution inputs.

<h2>Table of contents</h2>

- [Introduction](#introduction)
- [Shading language requirements](#shading-language-requirements)
- [Integration guidelines](#integration-guidelines)
    - [Scaling modes](#scaling-modes)
    - [Performance](#performance)
    - [Memory requirements](#memory-requirements)
    - [New APIs](#new-apis)
    - [Input resources](#input-resources)
    - [Depth buffer configurations](#depth-buffer-configurations)
    - [Providing motion vectors](#providing-motion-vectors)
    - [Optional resources](#optional-resources)
    - [Exposure](#exposure)
    - [Placement in the frame](#placement-in-the-frame)
    - [Temporal Antialiasing](#temporal-antialiasing)
    - [Camera jitter](#camera-jitter)
    - [Camera jump cuts](#camera-jump-cuts)
    - [Mipmap biasing](#mipmap-biasing)
    - [Frame Time Delta Input](#frame-time-delta-input)
    - [HDR support](#hdr-support)
    - [Non-linear Color](#non-linear-color)
    - [Debug Checker](#debug-checker)
    - [Debug view](#debug-view)
- [Limitations](#limitations)
- [Version history](#version-history)
- [References](#references)
- [See also](#see-also)

<h2>Introduction</h2>

**AMD FSR™ Upscaling 4** (or **AMD FSR™ 4** for short) is a cutting-edge upscaling technique for games that leverages machine learning to upscale lower-resolution frames to higher resolutions. The algorithm reconstructs detail and improves image quality by analyzing temporal and spatial information from multiple frames. This approach helps developers achieve higher perceived resolution and visual fidelity without a significant performance cost, making it suitable for real-time applications where both quality and efficiency are important.

<h2>Shading language requirements</h2>

- `HLSL`
  - `CS_6_6` for `DirectX® 12`
  - `SPIR-V Target: universal 1.5` for `Vulkan®`

<h2>Integration guidelines</h2>

AMD FSR™ Upscaling 4 requires integration using the [AMD FSR™ API](../getting-started/ffx-api.md) and use of the signed binary distribution.

For general integration guidelines please refer to [AMD FidelityFX™ Super Resolution 3](super-resolution-upscaler.md) documentation. The following are specific recommendations for AMD FSR™ Upscaling 4.

As stated in the [Input resources](#input-resources) section, it's important to make sure the input color given to AMD FSR™ Upscaling is in LINEAR COLORSPACE. In cases where this is not possible, the AMD FSR™ API allows the [`FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE`](../../upscalers/include/ffx_upscale.h#L57) flag to be set at context creation time. Image quality might be improved in these scenarios.

<h3>Scaling modes</h3>
For the convenience of end users, the AMD FSR™ API provides a number of preset scaling ratios which are named.

| Quality           | Per-dimension scaling factor |
|-------------------|------------------------------|
| NativeAA          | 1.0x                         |
| Quality           | 1.5x                         |
| Balanced          | 1.7x                         |
| Performance       | 2.0x                         |
| Ultra performance | 3.0x                         |

We strongly recommend that applications adopt consistent naming and scaling ratios in their user interface. This is to ensure that user experience is consistent for your application's users who may have experience of other applications using AMD FSR™ Upscaling.

<h3>Performance</h3>
Depending on your target hardware and operating configuration AMD FSR™ Upscaling will operate at different performance levels. Below you can find reference performance numbers for running FSR4 in performance mode for recommended target resolution. Upscaling from a lower resolution is slightly faster than upscaling from a higher one, but performance is mostly bound by the target resolution AMD FSR™ Upscaling upscales to.
Performance numbers are measured with RCAS sharpening disabled and peak GPU clocks.

| AMD Radeon™ GPU        | ML Model  | Target Resolution | Microseconds (us)|
| ---------------------- | --------- | ----------------- | ---------------- |
| AMD Radeon RX 9070 XT  | Full FP8  | 3840x2160         |  1316us          |
| AMD Radeon RX 9070 XT  | Full FP8  | 1920x1080         |   352us          |
| AMD Radeon RX 9060 XT  | Full FP8  | 3840x2160         |  3433us          |
| AMD Radeon RX 9060 XT  | Full FP8  | 1920x1080         |   583us          |
| AMD Radeon RX 7900 XTX | Full INT8 | 3840x2160         |  3070us          |
| AMD Radeon RX 7800 XT  | Full INT8 | 2560x1440         |  2050us          |
| AMD Radeon RX 7600     | Full INT8 | 1920x1080         |  1998us          |
| AMD Radeon RX 6900 XT  | Full INT8 | 3840x2160         |  3190us          |
| AMD Radeon RX 6700 XT  | Full INT8 | 2560x1440         |  2460us          |
| AMD Radeon RX 6600     | Full INT8 | 1920x1080         |  2183us          |
| AMD Radeon RX 6500 XT  | Full INT8 | 1920x1080         |  3130us          |

In addition to discrete AMD Radeon™ GPUs, AMD FSR™ Upscaling supports selected AMD Ryzen™ processors with integrated graphics through optimized INT8 ML models. Model selection is automatic and based on the target hardware. More capable integrated GPUs of the AMD Ryzen™ AI Max 300 and 400 Series ("Strix Halo", "Gorgon Halo") run the original INT8 model, while more constrained integrated graphics run a highly optimized INT8 model tuned to preserve performance.
Because integrated GPUs share system memory instead of using dedicated VRAM, APU upscaling performance may be affected by system memory bandwidth. Performance also depends on the APU's power and thermal configuration, so actual results on a given platform may vary.

| AMD Ryzen™ APU                        | System Memory | ML Model         | Target Resolution | Microseconds (us)|
| ------------------------------------- | ------------- | ---------------- | ----------------- | ---------------- |
| AMD Ryzen AI MAX+ 395 w/ Radeon 8060S | LPDDR5X-8000  | Full INT8        | 2560x1440         |  3022us          |
| AMD Ryzen AI MAX+ 395 w/ Radeon 8060S | LPDDR5X-8000  | Full INT8        | 1920x1080         |  1668us          |
| AMD Ryzen AI 9 HX 370 w/ Radeon 890M  | LPDDR5X-8000  | Lightweight INT8 | 2560x1440         |  4222us          |
| AMD Ryzen AI 9 HX 370 w/ Radeon 890M  | LPDDR5X-8000  | Lightweight INT8 | 1920x1080         |  2423us          |
| AMD Ryzen AI 9 HX 470 w/ Radeon 890M  | DDR5-5600     | Lightweight INT8 | 2560x1440         |  5311us          |
| AMD Ryzen AI 9 HX 470 w/ Radeon 890M  | DDR5-5600     | Lightweight INT8 | 1920x1080         |  3187us          |
| AMD Ryzen Z2 Extreme                  | LPDDR5X-8000  | Lightweight INT8 | 1920x1080         |  2520us          |
| AMD Ryzen Z2 A                        | LPDDR5-6400   | Lightweight INT8 | 1920x1080         |  5620us          |

<h3>Memory requirements</h3>
Using AMD FSR™ Upscaling requires some additional GPU local memory to be allocated for consumption by the GPU. When using the AMD FSR™ API, this memory is allocated when the AMD FSR™ Upscaling context is created, and is done so via the series of callbacks which comprise the backend interface. This memory is used to store intermediate surfaces which are computed by the AMD FSR™ Upscaling algorithm as well as surfaces which are persistent across many frames of the application. The table below includes the amount of memory used by AMD FSR™ Upscaling under various operating conditions.

| Resolution | Working set (MB) |
| -----------|------------------|
| 7680x4320  | 1274MB           |
| 3840x2160  |  318MB           |
| 1920x1080  |   81MB           |

Figures are approximations, rounded to nearest MB using an RX 9070XT GPU, and are subject to change. These do not change significantly between GPUs.


An application can get the amount of GPU local memory required by AMD FSR™ Upscaling after context creation by calling [`ffxQuery`](../../api/include/ffx_api.h#L152) with the valid context and [`ffxQueryDescUpscaleGetGPUMemoryUsage`](../../upscalers/include/ffx_upscale.h#L186).

An application can get the GPU local memory required by the default AMD FSR™ Upscaling version before context creation by calling [`ffxQuery`](../../api/include/ffx_api.h#L152) with `NULL` context and filling out [`ffxQueryDescUpscaleGetGPUMemoryUsageV2`](../../upscalers/include/ffx_upscale.h#L193). To get the memory requirement info for a different upscaler version, additionally link [`ffxOverrideVersion`](../../api/include/ffx_api.h#L101). 

See code examples of how to call [Query](../getting-started/ffx-api.md#query).

<h3>New APIs</h3>

As of SDK 2.1 it is necessary to create and fill in a [`ffxCreateContextDescUpscaleVersion`](../../upscalers/include/ffx_upscale.h#L222), setting the [`version`](../../upscalers/include/ffx_upscale.h#L225) field to [`FFX_UPSCALER_VERSION`](../../upscalers/include/ffx_upscale.h#L36) and link this in the header.pNext field of the above [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure. This is used to ensure compatibility between different versions of the API.

<h3>Input resources</h3>
The following table enumerates the core set of inputs required by AMD FSR™ Upscaling.

The resolution column indicates if the data should be at 'rendered' resolution or 'presentation' resolution. 'Rendered' resolution indicates that the resource should match the resolution at which the application is performing its rendering. Conversely, 'presentation' indicates that the resolution of the target should match that which is to be presented to the user. All resources are from the current rendered frame. DirectX® 12 applications should transition every input resource to `D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE` before calling [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87). Vulkan® applications should transition those images to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.

| Name            | Resolution                   |  Format                            | Type      | Notes                                          |
| ----------------|------------------------------|------------------------------------|-----------|------------------------------------------------|
| Color buffer    | Render                       | `APPLICATION SPECIFIED`            | Texture   | The render resolution color buffer for the current frame provided by the application. The contents of the color buffer should always be in Linear Colorspace, in the case where this is not possible, then the [`FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE`](../../upscalers/include/ffx_upscale.h#L57) flag should be set in  the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure. |
| Depth buffer    | Render                       | `APPLICATION SPECIFIED (1x FLOAT)` | Texture   | The render resolution depth buffer for the current frame provided by the application. The data should be provided as a single floating point value, the precision of which is under the application's control. The configuration of the depth should be communicated to AMD FSR™ Upscaling via the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure. You should set the [`FFX_UPSCALE_ENABLE_DEPTH_INVERTED`](../../upscalers/include/ffx_upscale.h#L52) flag if your depth buffer is inverted (that is [1..0] range). |
| Motion vectors  | Render or presentation       | `APPLICATION SPECIFIED (2x FLOAT)` | Texture   | The 2D motion vectors for the current frame provided by the application in **[<-width, -height> ... <width, height>]** range. If your application renders motion vectors with a different range, you may use the [`motionVectorScale`](../../upscalers/include/ffx_upscale.h#L99) field of the [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87) structure to adjust them to match the expected range for AMD FSR™ Upscaling. Internally, AMD FSR™ Upscaling uses 16-bit quantities to represent motion vectors in many cases, which means that while motion vectors with greater precision can be provided, AMD FSR™ Upscaling will not benefit from the increased precision. The resolution of the motion vector buffer should be equal to the render resolution, unless the [`FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS`](../../upscalers/include/ffx_upscale.h#L50) flag is set in the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure, in which case it should be equal to the presentation resolution. |
| Exposure        | 1x1                          | `R32_FLOAT`                        | Texture   | A 1x1 texture containing the exposure value computed for the current frame. This resource is optional, and may be omitted if the [`FFX_UPSCALE_ENABLE_AUTO_EXPOSURE`](../../upscalers/include/ffx_upscale.h#L54) flag is set in the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure.  |

All inputs that are provided at Render Resolution, except for motion vectors, should be rendered with jitter. Motion vectors should not have jitter applied, unless the `FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION` flag is present.

<h3>Depth buffer configurations</h3>

It is strongly recommended that an inverted depth buffer is used with AMD FSR™ Upscaling. However, alternative depth buffer configurations are supported. An application should inform the AMD FSR™ API of its depth buffer configuration by setting the appropriate flags during the creation of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77). The table below contains the appropriate flags.

| AMD FSR™ Upscaling flag                         | Note                                                                                       |
|----------------------------------|--------------------------------------------------------------------------------------------|
| [`FFX_UPSCALE_ENABLE_DEPTH_INVERTED`](../../upscalers/include/ffx_upscale.h#L52) | A bit indicating that the input depth buffer data provided is inverted [max..0].           |
| [`FFX_UPSCALE_ENABLE_DEPTH_INFINITE`](../../upscalers/include/ffx_upscale.h#L53) | A bit indicating that the input depth buffer data provided is using an infinite far plane. |


<h3>Providing motion vectors</h3>

<h4>Space</h4>

A key part of a temporal algorithm (be it antialiasing or upscaling) is the provision of motion vectors. AMD FSR™ Upscaling accepts motion vectors in 2D which encode the motion from a pixel in the current frame to the position of that same pixel in the previous frame. AMD FSR™ Upscaling expects that motion vectors are provided by the application in [**<-width, -height>**..**<width, height>**] range; this matches screenspace. For example, a motion vector for a pixel in the upper-left corner of the screen with a value of **<width, height>** would represent a motion that traversed the full width and height of the input surfaces, originating from the bottom-right corner.

![alt text](media/super-resolution-temporal/motion-vectors.svg "A diagram showing a 2D motion vector.")

If your application computes motion vectors in another space - for example normalized device coordinate space - then you may use the [`motionVectorScale`](../../upscalers/include/ffx_upscale.h#L99) field of the [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87) structure to instruct AMD FSR™ Upscaling to adjust them to match the expected range for AMD FSR™ Upscaling. The code examples below illustrate how motion vectors may be scaled to screen space. The example HLSL and C++ code below illustrates how NDC-space motion vectors can be scaled using the AMD FSR™ Upscaling host API.

```HLSL
// GPU: Example of application NDC motion vector computation
float2 motionVector = (previousPosition.xy / previousPosition.w) - (currentPosition.xy / currentPosition.w);

// CPU: Matching AMD FSR™ Upscaling motionVectorScale configuration
dispatchParameters.motionVectorScale.x = (float)renderWidth;
dispatchParameters.motionVectorScale.y = (float)renderHeight;
```

<h4>Precision & resolution </h4>

Internally, AMD FSR™ Upscaling uses 16-bit quantities to represent motion vectors in many cases, which means that while motion vectors with greater precision can be provided, AMD FSR™ Upscaling will not currently benefit from the increased precision. The resolution of the motion vector buffer should be equal to the render resolution, unless the [`FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS`](../../upscalers/include/ffx_upscale.h#L50) flag is set in the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure when creating the [`ffxContext`](../../api/include/ffx_api.h#L45), in which case it should be equal to the presentation resolution.

<h4>Coverage</h4>

AMD FSR™ Upscaling will perform better quality upscaling when more objects provide their motion vectors. It is therefore advised that all opaque, alpha-tested and alpha-blended objects should write their motion vectors for all covered pixels. If vertex shader effects are applied - such as scrolling UVs - these calculations should also be factored into the calculation of motion for the best results.

<h3>Optional resources</h3>

FSR4 no longer requires the title to generate either a Reactive mask or a Transparency and Composition mask, though they may still be provided to FSR4. To make it easier to integrate FSR4 in titles that also wish to offer earlier versions of AMD FidelityFX™ Super Resolution as options a new [`ffxQuery`](../../api/include/ffx_api.h#L152) has been added. By using [`ffxQueryDescUpscaleGetResourceRequirements`](../../upscalers/include/ffx_upscale.h#L214) as the query structure, specifying [`FFX_API_QUERY_DESC_TYPE_UPSCALE_GET_RESOURCE_REQUIREMENTS`](../../upscalers/include/ffx_upscale.h#L213) as the query identifier, the caller will receive a bitmap specifying which resources defined in [`FfxApiQueryResourceIdentifiers`](../../upscalers/include/ffx_upscale.h#L203) are required and one for resources that are optional. This allows the caller to identify when the additional work to generate the masks should be performed and when it can be omitted, thereby reducing the performance overhead for FSR4. Calling [`ffxQuery`](../../api/include/ffx_api.h#L152) with valid `context` for this query structure will return the resource requirement info for the passed context version. To get the resource requirement info for a different upscaler version or before upscaler context creation, call [`ffxQuery`](../../api/include/ffx_api.h#L152) with `NULL` context and link a [`ffxOverrideVersion`](../../api/include/ffx_api.h#L101) and backend creation struct (`ffxCreateBackendDX12Desc`). See code examples of how to call [Query](../getting-started/ffx-api.md#query).

<h3>Exposure</h3>

AMD FSR™ Upscaling provides two values which control the exposure used when performing upscaling. They are as follows:

1. **Pre-exposure** a value by which we divide the input signal to get back to the original signal produced by the game before any packing into lower precision render targets.
2. **Exposure** a value which is multiplied against the result of the pre-exposed color value.

The exposure value should match that which the application uses during any subsequent tonemapping passes performed by the application. This means AMD FSR™ Upscaling will operate consistently with what is likely to be visible in the final tonemapped image.

> In various stages of the AMD FSR™ Upscaling algorithm described in this document, AMD FSR™ Upscaling will compute its own exposure value for internal use. It is worth noting that all outputs from AMD FSR™ Upscaling will have this internal tonemapping reversed before the final output is written. Meaning that AMD FSR™ Upscaling returns results in the same domain as the original input signal.

Poorly selected exposure values can have a drastic impact on the final quality of AMD FSR™ Upscaling. Therefore, it is recommended that [`FFX_UPSCALE_ENABLE_AUTO_EXPOSURE`](../../upscalers/include/ffx_upscale.h#L54) is used by the application, unless there is a particular reason not to. When [`FFX_UPSCALE_ENABLE_AUTO_EXPOSURE`](../../upscalers/include/ffx_upscale.h#L54) is set in the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure, the exposure calculation shown in the HLSL code below is used to compute the exposure value, which matches the exposure response of ISO 100 film stock.

```HLSL
float ComputeAutoExposureFromAverageLog(float averageLogLuminance)
{
	const float averageLuminance = exp(averageLogLuminance);
	const float S = 100.0f; // ISO arithmetic speed
	const float K = 12.5f;
	const float exposureIso100 = log2((averageLuminance * S) / K);
	const float q = 0.65f;
	const float luminanceMax = (78.0f / (q * S)) * pow(2.0f, exposureIso100);
	return 1 / luminanceMax;
}
```

<h3>Placement in the frame</h3>

The primary goal of AMD FSR™ Upscaling is to improve application rendering performance by using a temporal upscaling algorithm relying on a number of inputs. Therefore, its placement in the pipeline is key to ensuring the right balance between the highest visual quality and great performance.

![alt text](media/super-resolution-ml/pipeline-placement.svg "A diagram showing the placement of temporal AMD FSR™ Upscaling in the wider rendering pipeline.")

With any image upscaling approach it is important to understand how to place other image-space algorithms with respect to the upscaling algorithm. Placing these other image-space effects before the upscaling has the advantage that they run at a lower resolution, which of course confers a performance advantage onto the application. However, it may not be appropriate for some classes of image-space techniques. For example, many applications may introduce noise or grain into the final image, perhaps to simulate a physical camera. Doing so before an upscaler might cause the upscaler to amplify the noise, causing undesirable artifacts in the resulting upscaled image. The following table divides common real-time image-space techniques into two columns. 'Post processing A' contains all the techniques which typically would run before AMD FSR™ Upscaling, meaning they would all run at render resolution. Conversely, the 'Post processing B' column contains all the techniques which are recommended to run after AMD FSR™ Upscaling, meaning they would run at the larger, presentation resolution.

| Post processing A              | Post processing B    |
|--------------------------------|----------------------|
| Screenspace reflections        | Film grain           |
| Screenspace ambient occlusion  | Chromatic aberration |
| Denoisers (shadow, reflections)| Vignette             |
| Exposure (optional)            | Tonemapping          |
|                                | Bloom                |
|                                | Depth of field       |
|                                | Motion blur          |

Please note that the recommendations here are for guidance purposes only and depend on the precise characteristics of your application's implementation.

<h3>Temporal Antialiasing</h3>

Temporal antialiasing (TAA) is a technique which uses the output of previous frames to construct a higher quality output from the current frame. As AMD FSR™ Upscaling has a similar goal - albeit with the additional goal of also increasing the resolution of the rendered image - there is no longer any need to include a separate TAA pass in your application.

<h3>Camera jitter</h3>

AMD FSR™ Upscaling relies on the application to apply sub-pixel jittering while rendering - this is typically included in the projection matrix of the camera. To make the application of camera jitter simple, the AMD FSR™ API provides a small set of utility functions which compute the sub-pixel jitter offset for a particular frame within a sequence of separate jitter offsets.

Internally, these functions implement a Halton[2,3] sequence [[Halton](#references)]. The goal of the Halton sequence is to provide spatially separated points, which cover the available space.

![alt text](media/super-resolution-temporal/jitter-space.svg "A diagram showing how to map sub-pixel jitter offsets to projection offsets.")

It is important to understand that the values returned from the [`ffxQueryDescUpscaleGetJitterOffset`](../../upscalers/include/ffx_upscale.h#L143) are in unit pixel space, and in order to composite this correctly into a projection matrix we must convert them into projection offsets. The diagram above shows a single pixel in unit pixel space, and in projection space. The code listing below shows how to correctly composite the sub-pixel jitter offset value into a projection matrix.

``` CPP
ffx::ReturnCode                     retCode;
int32_t                             jitterPhaseCount;
ffx::QueryDescUpscaleGetJitterPhaseCount getJitterPhaseDesc{};
getJitterPhaseDesc.displayWidth   = displayWidth;
getJitterPhaseDesc.renderWidth    = renderWidth;
getJitterPhaseDesc.pOutPhaseCount = &jitterPhaseCount;
retCode = ffx::Query(fsrContext, getJitterPhaseDesc);

ffx::QueryDescUpscaleGetJitterOffset getJitterOffsetDesc{};
getJitterOffsetDesc.index                              = jitterIndex;
getJitterOffsetDesc.phaseCount                         = jitterPhaseCount;
getJitterOffsetDesc.pOutX                              = &jitterX;
getJitterOffsetDesc.pOutY                              = &jitterY;
retCode = ffx::Query(fsrContext, getJitterOffsetDesc);

// Calculate the jittered projection matrix.
const float jitterX = 2.0f * jitterX / (float)renderWidth;
const float jitterY = -2.0f * jitterY / (float)renderHeight;
const Matrix4 jitterTranslationMatrix = translateMatrix(Matrix3::identity, Vector3(jitterX, jitterY, 0));
const Matrix4 jitteredProjectionMatrix = jitterTranslationMatrix * projectionMatrix;
```

Jitter should be applied to *all* rendering. This includes opaque, alpha transparent, and raytraced objects. For rasterized objects, the sub-pixel jittering values calculated by the [`ffxQueryDescUpscaleGetJitterOffset`](../../upscalers/include/ffx_upscale.h#L143) function can be applied to the camera projection matrix which is ultimately used to perform transformations during vertex shading. For raytraced rendering, the sub-pixel jitter should be applied to the ray's origin - often the camera's position.

Whether you elect to use the recommended [`ffxQueryDescUpscaleGetJitterOffset`](../../upscalers/include/ffx_upscale.h#L143) query or your own sequence generator, you must set the [`jitterOffset`](../../upscalers/include/ffx_upscale.h#L98) field of the [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87) structure to inform AMD FSR™ Upscaling of the jitter offset that has been applied in order to render each frame. Moreover, if not using the recommended [`ffxQueryDescUpscaleGetJitterOffset`](../../upscalers/include/ffx_upscale.h#L143) query, care should be taken that your jitter sequence never generates a null vector; that is, a value of 0 in both the X and Y dimensions.

The table below shows the jitter sequence length for each of the default quality modes.

 | Quality mode      | Scaling factor          | Sequence length |
 |-------------------|-------------------------|-----------------|
 | Quality           | 1.5x (per dimension)    | 18              |
 | Balanced          | 1.7x (per dimension)    | 23              |
 | Performance       | 2.0x (per dimension)    | 32              |
 | Ultra performance | 3.0x (per dimension)    | 72              |
 | Custom            | [1..n]x (per dimension) | `ceil(8 * n^2)` |

<h3>Camera jump cuts</h3>

Most applications with real-time rendering have a large degree of temporal consistency between any two consecutive frames. However, there are cases where a change to a camera's transformation might cause an abrupt change in what is rendered. In such cases, AMD FSR™ Upscaling is unlikely to be able to reuse any data it has accumulated from previous frames, and should clear this data so as to exclude it from consideration in the compositing process. In order to indicate to AMD FSR™ Upscaling that a jump cut has occurred with the camera you should set the [`reset`](../../upscalers/include/ffx_upscale.h#L106) field of the [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87) structure to `true` for the first frame of the discontinuous camera transformation.

Rendering performance may be slightly less than typical frame-to-frame operation when using the reset flag, as AMD FSR™ Upscaling will clear some additional internal resources.

<h3>Mipmap biasing</h3>

Applying a negative mipmap biasing will typically generate an upscaled image with better texture detail. We recommend applying the following formula to your Mipmap bias:

``` CPP
mipBias = log2(renderResolution/displayResolution) - 1.0;
```

It is suggested that applications adjust the MIP bias for specific high-frequency texture content which is susceptible to showing temporal aliasing issues.

The following table illustrates the mipmap biasing factor which results from evaluating the above pseudocode for the scaling ratios matching the suggested quality modes that applications should expose to end users.

 | Quality mode      | Scaling factor        | Mipmap bias |
 |-------------------|-----------------------|-------------|
 | Quality           | 1.5X (per dimension)  | -1.58       |
 | Balanced          | 1.7X (per dimension)  | -1.76       |
 | Performance       | 2.0X (per dimension)  | -2.0        |
 | Ultra performance | 3.0X (per dimension)  | -2.58       |

<h3>Frame Time Delta Input</h3>

The AMD FSR™ API requires [`frameTimeDelta`](../../upscalers/include/ffx_upscale.h#L104) be provided by the application through the [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87) structure. This value is in __milliseconds__: if running at 60fps, the value passed should be around __16.6f__.

The value is used within the temporal component of the AMD FSR™ Upscaling auto-exposure feature.

<h3>HDR support</h3>

High dynamic range images are supported in AMD FSR™ Upscaling. To enable this, you should set the [`FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE`](../../upscalers/include/ffx_upscale.h#L49) bit in the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure. For best results, images should be provided to AMD FSR™ Upscaling in linear color space.

<h3>Non-linear Color</h3>

For applications that cannot provide images in linear color space the [`FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE`](../../upscalers/include/ffx_upscale.h#L57) flag should be set in the [`flags`](../../upscalers/include/ffx_upscale.h#L80) field of the [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) structure.

In addition, two optional flags have been added to [`FfxApiDispatchFsrUpscaleFlags`](../../upscalers/include/ffx_upscale.h#L61) to specify the specific colorspace in use.

These flags are mutually exclusive and are set in the [`flags`](../../upscalers/include/ffx_upscale.h#L111) field of the [`ffxDispatchDescUpscale`](../../upscalers/include/ffx_upscale.h#L87) struct and then inform FSR4 of the colorspace used for the input image. This may improve visual quality in some circumstances above and beyond specifying [`FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE`](../../upscalers/include/ffx_upscale.h#L57) alone. 


| Flag                                                                                                  | Description                                                              |
|-------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------|
| [`FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB`](../../upscalers/include/ffx_upscale.h#L64) | Indicates that the input color resource contains perceptual sRGB colors. |
| [`FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_PQ`](../../upscalers/include/ffx_upscale.h#L65)   | Indicates that the input color resource contains perceptual PQ colors. |


> Support for additional color spaces might be provided in a future revision of AMD FSR™ Upscaling.

<h3>Debug Checker</h3>
Enable debug checker to validate application supplied inputs at dispatch upscale. This feature can be enabled in any build configuration of the runtime (IE. release binaries from PrebuiltSignedDll folder or debug build). It is recommended this is enabled only in development builds of the game.

Passing the [`FFX_UPSCALE_ENABLE_DEBUG_CHECKING`](../../upscalers/include/ffx_upscale.h#L56) flag within the flags member of [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) will output textual warning messages from upscaler to debugger TTY by default. Application can set the callback function for runtime to pass the messages to the underlying application. Application can assign `fpMessage` from [`ffxCreateContextDescUpscale`](../../upscalers/include/ffx_upscale.h#L77) to suitable function. `fpMessage` is of type `ffxApiMessage` which is a function pointer for passing string messages of various types.

An example of the kind of output that can occur when the checker observes possible issues is below:

```
FSR_API_DEBUG_WARNING: FFX_FSR_ENABLE_DEPTH_INFINITE and FFX_FSR_ENABLE_DEPTH_INVERTED present, cameraFar value is very low which may result in depth separation artifacting
FSR_API_DEBUG_WARNING: frameTimeDelta is less than 1.0f - this value should be milliseconds (~16.6f for 60fps)
```

<h3>Debug view</h3>

When `FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW` is set in the flags attribute of `ffxDispatchDescUpscale`, an additional debug output pass will be executed to render debug data from internal surfaces onto the upscaled frame. On the top left motion vectors are displayed, this could be used to make sure your animated objects report motion correctly to AMD FSR™ Upscaling. On the top right you can inspect the blend factor computed by AMD FSR™ Upscaling, the bright purple parts highlight areas of low blend factor with history texture, signaling a detected reactivity area.

![Frame interpolation debug overlay](media/super-resolution-upscaler/upscaler-debug-fsr4.png "A diagram showing the debug overlay")

<h2>Limitations</h2>

FSR4 requires an AMD 7000 series discrete GPU or AMD 9000 series GPU or later.

<h2>Version history</h2>

| Version        | Date              |
| ---------------|-------------------|
| **4.1.2**      | 2026-10-28        |
| **4.1.1**      | 2026-06-24        |
| **4.1.0**      | 2026-03-18        |
| **4.0.3**      | 2025-12-10        |
| **4.0.2**      | 2025-08-20        |

Refer to changelog for more detail on versions.

<h2>References</h2>

[**Halton**] Halton sequence, <strong>"Halton sequence"</strong>, (See Halton Sequence on Wikipedia)

<h2>See also</h2>

- [AMD FSR™ Upscaling and Frame Generation Sample](../../../../docs/samples/super-resolution.md)
- [AMD FSR™ Naming guidelines](../getting-started/naming-guidelines.md)
