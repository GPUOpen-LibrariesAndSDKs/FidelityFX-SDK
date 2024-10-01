<h1>Welcome to the AMD FidelityFX SDK</h1>

![alt text](/docs/media/fidelityfxsdk-logo-rescaled.png)

The FidelityFX SDK is a collection of heavily optimized, open source technologies (shader and runtime code) that can be used by developers to improve their DirectX®12 or Vulkan® applications. 

The FidelityFX SDK includes:

| [FidelityFX SDK](https://gpuopen.com/amd-fidelityfx-sdk/) Technique | [Samples](/docs/samples/index.md) | [GPUOpen](https://gpuopen.com/) page | Description |
| --- | --- | --- | --- |
| [Combined Adaptive Compute Ambient Occlusion (CACAO)](/docs/techniques/combined-adaptive-compute-ambient-occlusion.md) 1.3 | [CACAO sample](/docs/samples/combined-adaptive-compute-ambient-occlusion.md) | [FidelityFX Ambient Occlusion](https://gpuopen.com/fidelityfx-cacao/) | Uses intelligent and adaptive sampling techniques to produce excellent quality ambient occlusion at high performance. |
| [Contrast Adaptive Sharpening (CAS)](/docs/techniques/contrast-adaptive-sharpening.md) 1.1 | [CAS sample](/docs/samples/contrast-adaptive-sharpening.md) | [FidelityFX Contrast Adaptive Sharpening](https://gpuopen.com/fidelityfx-cas/) | Implements a sharpening kernel that reclaims that high-frequency detail lost during rendering. |
| [Denoiser](/docs/techniques/denoiser.md) 1.2 | n/a | [FidelityFX Denoiser](https://gpuopen.com/fidelityfx-denoiser/) | Provides a set of denoising compute shaders which remove artifacts from reflection and shadow rendering. Useful for both raytraced or rasterized content. |
| [Classifier](/docs/techniques/classifier.md) 1.0 | n/a | n/a | Provides a set of tile classification compute shaders which prepare tile metadata to drive indirect workload generation. It's useful for guided and load-balanced ray tracing applications, letting you leverage ray tracing in an efficient manner. |
| [Luminance Preserving Mapper](/docs/techniques/luminance-preserving-mapper.md) 1.3 | [LPM sample](/docs/samples/luminance-preserving-mapper.md) | [FidelityFX HDR Mapper](https://gpuopen.com/fidelityfx-lpm/) | Offers a tone mapping and gamut mapping solution for HDR and wide gamut content. |
| [Parallel Sort](/docs/techniques/parallel-sort.md) 1.2 | [Parallel Sort sample](/docs/samples/parallel-sort.md) | [FidelityFX Parallel Sort](https://gpuopen.com/fidelityfx-parallel-sort/) | Implements GPU-accelerated parallel sorting techniques. The sorts are stable useful for sorting particles or other GPU-side data sets. |
| [Single Pass Downsampler](/docs/techniques/single-pass-downsampler.md) 2.1 | [SPD sample](/docs/samples/single-pass-downsampler.md) | [FidelityFX Downsampler](https://gpuopen.com/fidelityfx-spd/) | Allows you to downsample surfaces - and optionally generate a MIPmap chain - in a single compute dispatch. |
| [Stochastic Screen-Space Reflections](/docs/techniques/stochastic-screen-space-reflections.md) 1.4 | [SSSR sample](/docs/samples/stochastic-screen-space-reflections.md) | [FidelityFX Screen Space Reflections](https://gpuopen.com/fidelityfx-sssr/) | Provides high-fidelity screen-spaced reflections in your scene, without a hefty performance price tag. |
| [Super Resolution (Spatial)](/docs/techniques/super-resolution-spatial.md) 1.1 | [Super Resolution sample](/docs/samples/super-resolution.md) | [FidelityFX Super Resolution](https://gpuopen.com/fidelityfx-superresolution/) | Offers a spatial single-frame solution for producing higher resolution frames from lower resolution inputs. |
| [Super Resolution (Temporal)](/docs/techniques/super-resolution-temporal.md) 2.2.1 | [Super Resolution sample](/docs/samples/super-resolution.md) | [FidelityFX Super Resolution 2](https://gpuopen.com/fidelityfx-superresolution-2/) | Offers both spatial single-frame and temporal multi-frame solutions for producing high resolution frames from lower resolution inputs. |
| [Super Resolution (Interpolation enabled)](/docs/techniques/super-resolution-interpolation.md) 3.1.0 | [Super Resolution sample](/docs/samples/super-resolution.md) | [FidelityFX Super Resolution 3](https://gpuopen.com/fidelityfx-superresolution-3/) | Offers a solution for producing high resolution frames from lower resolution inputs and generating interpolated frames. |
| [Optical Flow](/docs/techniques/optical-flow.md) 1.0 | [Super Resolution sample](/docs/samples/super-resolution.md) | [FidelityFX Super Resolution 3](https://gpuopen.com/fidelityfx-superresolution-3/) | Estimates the motion between the current and the previous scene inputs. |
| [Frame Interpolation](/docs/techniques/frame-interpolation.md) 1.0.1 | [Super Resolution sample](/docs/samples/super-resolution.md) | [FidelityFX Super Resolution 3](https://gpuopen.com/fidelityfx-superresolution-3/) | Generates an image half way between the previous and the current frame. |
| [Frame Interpolation Swap Chain](/docs/techniques/frame-interpolation-swap-chain.md) 1.0.1 | [Super Resolution sample](/docs/samples/super-resolution.md) | [FidelityFX Super Resolution 3](https://gpuopen.com/fidelityfx-superresolution-3/) | Proxy swap chain to replace the DXGI swap chain and handle frame pacing, UI composition and presentation. |
| [Variable Shading](/docs/techniques/variable-shading.md) 1.1 | [Variable Shading sample](/docs/samples/variable-shading.md) | [FidelityFX Variable Shading](https://gpuopen.com/fidelityfx-variable-shading/) | Helps you to drive Variable Rate Shading hardware introduced in RDNA2-based and contemporary GPUs, by analyzing the luminance of pixels in a tile to determine where the shading rate can be lowered to increase performance. |
| [Blur](/docs/samples/blur.md) 1.0 | [Blur sample](/docs/samples/blur.md) | [FidelityFX Blur](https://gpuopen.com/fidelityfx-blur/) | A library of highly optimized functions which perform common blurring operations such as Gaussian blur, radial blurs, and others. |
| [Depth-of-Field](/docs/techniques/depth-of-field.md) 1.0 | [DoF sample](/docs/samples/depth-of-field.md) | [FidelityFX Depth of Field](https://gpuopen.com/fidelityfx-dof/) | Implements a high-quality DOF filter complete with bokeh. |
| [Lens](/docs/samples/lens.md) 1.0 | [Lens sample](/docs/samples/lens.md) | [FidelityFX Lens](https://gpuopen.com/fidelityfx-lens/) | Implements a library of optimized lens effects including chromatic aberration, film grain, and vignetting. |
| [Classifier (Shadows)](/docs/techniques/classifier.md) 1.1 [Denoiser (Shadows)](/docs/techniques/denoiser.md) 1.2 | [Hybrid Shadows sample](/docs/samples/hybrid-shadows.md) 1.1 | [FidelityFX Hybrid Shadows](https://gpuopen.com/fidelityfx-hybrid-shadows/) | An implementation of an example shadowing technique which shows you how you could combine rasterized shadow maps and hardware ray tracing to deliver high quality soft shadows at a reasonable performance cost. |
| [Classifier (Reflections)](/docs/techniques/classifier.md) 1.1 [Denoiser (Reflections)](/docs/techniques/denoiser.md) 1.2 | [Hybrid Reflections sample](/docs/samples/hybrid-reflections.md) 1.1 | [FidelityFX Hybrid Reflections](https://gpuopen.com/fidelityfx-hybrid-reflections/) | An implementation of an an example reflections technique which shows you how you could mix FidelityFX SSSR with ray traced reflections, delivering higher quality reflections than SSSR alone at reasonable performance cost. |
| New: [Breadcrumbs](docs/techniques/breadcrumbs.md) 1.0 | [Breadcrumbs sample](docs/samples/breadcrumbs.md) | [FidelityFX Breadcrumbs](https://gpuopen.com/fidelityfx-breadcrumbs/) | Library aiding with post-mortem GPU crash analysis. |

<h2>Further information</h2>

- [What's new in FidelityFX SDK](/docs/whats-new/index.md)
  - [FidelityFX SDK 1.1](/docs/whats-new/index.md)
  - [FidelityFX SDK 1.0](/docs/whats-new/version_1_0.md)

- [Getting started](/docs/getting-started/index.md)
  - [Overview](/docs/getting-started/index.md)
  - [SDK structure](/docs/getting-started/sdk-structure.md)
  - [Building the samples](/docs/getting-started/building-samples.md)
  - [Running the samples](/docs/getting-started/running-samples.md)
  - [Naming guidelines](/docs/getting-started/naming-guidelines.md)

- [Tools](/docs/tools/index.md)
  - [Shader Precompiler](/docs/tools/ffx-sc.md)
  - [FidelityFX SDK Media Delivery System](/docs/media-delivery.md)

<h2>Known issues</h2>

| FidelityFX SDK Sample | API / Configuration | Problem Description |
| --- | --- | --- |
| FidelityFX LPM | Vulkan / All configurations | When rapidly pressing alt-enter to go to full screen mode and back, the HDR display handle can occasionally become lost leading to a dim screen until another full screen toggle is applied again. |
| FidelityFX Hybrid Shadows / FidelityFX FSR | Vulkan / All configurations | Due to resource view handling in the native Vulkan backend, the ability to change the number of cascades on a directional light in Vulkan samples has been disabled to prevent sample instability. |
| FidelityFX DOF | All APIs / All Configs | Some artifacts may occur on some Intel Arc GPUs. |
| All FidelityFX SDK Samples | All APIs / All Configs | There is a resource leak in the UploadContext used to load glTF content. |
| All FidelityFX SDK Samples | All APIs / All Configs | Windows path length restrictions may cause compile issues. It is recommended to place the SDK close to the root of a drive or use subst or a mklink to shorten the path. |

<h2>Open source</h2>

AMD FidelityFX SDK is open source, and available under the MIT license.

For more information on the license terms please refer to [license](license.txt).

<h2>Disclaimer</h2>

The information contained herein is for informational purposes only, and is subject to change without notice. While every
precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical
errors, and AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no
representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no
liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with
respect to the operation or use of AMD hardware, software or other products described herein. No license, including implied or
arising by estoppel, to any intellectual property rights is granted by this document. Terms and limitations applicable to the purchase
or use of AMD’s products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions
or use of AMD’s products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions
of Sale.

AMD, the AMD Arrow logo, Radeon, Ryzen, CrossFire, RDNA and combinations thereof are trademarks of Advanced Micro Devices, Inc.
Other product names used in this publication are for identification purposes only and may be trademarks of their respective companies.

DirectX is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Vulkan and the Vulkan logo are registered trademarks of the Khronos Group Inc.

OpenCL is a trademark of Apple Inc. used by permission by Khronos Group, Inc.

Microsoft is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Windows is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

© 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
