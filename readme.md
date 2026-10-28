<h1>Welcome to the AMD FSR™ SDK 2.4.0 - "Redstone"</h1>

![alt text](Kits/FidelityFX/docs/media/AMD_FSR_SDK_Lockup_RGB_Wht.png)

The AMD FSR™ SDK is a collection of heavily optimized technologies that can be used by developers to improve their DirectX® 12 or Vulkan® applications. 

The AMD FSR™ SDK includes:

| [AMD FSR™ SDK Technique](https://gpuopen.com/amd-fidelityfx-sdk/) | [Samples](docs/samples/index.md) | [GPUOpen page](https://gpuopen.com/) | Description |
| --- | --- | --- | --- |
| [AMD FidelityFX™ Super Resolution (Temporal)](Kits/FidelityFX/docs/techniques/super-resolution-temporal.md) 2.3.4 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FidelityFX™ Super Resolution 2](https://gpuopen.com/fidelityfx-superresolution-2/) | Offers a temporal (multi-frame accumulation) solution for producing high resolution frames from lower resolution inputs. |
| [AMD FidelityFX™ Super Resolution (Upscaler)](Kits/FidelityFX/docs/techniques/super-resolution-upscaler.md) 3.1.6 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FidelityFX™ Super Resolution 3](https://gpuopen.com/fidelityfx-superresolution-3/) | Offers a temporal (multi-frame accumulation) solution for producing high resolution frames from lower resolution inputs. |
| [AMD FSR™ Upscaling (ML-Upscaler)](Kits/FidelityFX/docs/techniques/super-resolution-ml.md) 4.1.2 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FSR™ Upscaling 4](https://gpuopen.com/fidelityfx-superresolution-4/) | Offers a machine learning-based solution for producing high resolution frames from lower resolution inputs. |
| [AMD FidelityFX™ Super Resolution Frame Generation](Kits/FidelityFX/docs/techniques/frame-interpolation.md) 3.1.6 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FidelityFX™ Super Resolution Frame Generation 3](https://gpuopen.com/fidelityfx-superresolution-3/) | Offers generation of interpolated frames from multiple real input frames, and multiple sources of motion vector data. |
| [AMD FidelityFX™ Super Resolution Frame Generation (Swapchain)](Kits/FidelityFX/docs/techniques/frame-interpolation-swap-chain.md) 3.1.7 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FidelityFX™ Super Resolution Frame Generation (Swapchain) 3](https://gpuopen.com/fidelityfx-superresolution-3/) | A replacement DXGI Swapchain implementation for DX12 which allows for additional frames to be presented along with real game frames, with relevant frame pacing. |
| [AMD FSR™ Frame Generation (Swapchain)](Kits/FidelityFX/docs/techniques/frame-interpolation-swap-chain.md) 4.0.0 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FSR™ Frame Generation 4](https://gpuopen.com/amd-fsr-framegeneration/) | A replacement DXGI Swapchain implementation for DX12 which allows for additional frames to be presented along with real game frames, with relevant frame pacing. |
| [AMD FSR™ Frame Generation (ML)](Kits/FidelityFX/docs/techniques/frame-interpolation-ml.md) 4.0.2 | [AMD FSR™ Upscaling and Frame Generation Sample](docs/samples/super-resolution.md) | [AMD FSR™ Frame Generation 4](https://gpuopen.com/amd-fsr-framegeneration/) | Offers generation of interpolated frames from multiple real input frames, and multiple sources of motion vector data. |
| [AMD FSR™ Ray Regeneration (ML-Denoiser)](Kits/FidelityFX/docs/techniques/denoising.md) 1.2.1 | [AMD FSR™ Ray Regeneration Sample](docs/samples/denoiser.md) | [AMD FSR™ Ray Regeneration](https://gpuopen.com/amd-fsr-rayregeneration/) | Offers a machine learning-based solution for denoising. |
| [AMD FSR™ Radiance Caching (Preview)](Kits/FidelityFX/docs/techniques/radiance-cache.md) | [AMD FSR™ Radiance Caching (Preview) Sample](docs/samples/radiance-cache.md) | [AMD FSR™ Radiance Caching (Preview)](https://gpuopen.com/amd-fsr-radiancecaching/) | Offers a machine learning-based solution for path tracing result caching. |

<h2>Further information</h2>


- [What's new in AMD FSR™ SDK](Kits/FidelityFX/docs/whats-new/index.md)
  - [AMD FSR™ SDK 2.4.0](Kits/FidelityFX/docs/whats-new/index.md)
  - [AMD FSR™ SDK 2.3.0](Kits/FidelityFX/docs/whats-new/version_2_3_0.md)
  - [AMD FSR™ SDK 2.2.0](Kits/FidelityFX/docs/whats-new/version_2_2_0.md)
  - [AMD FSR™ SDK 2.1.0](Kits/FidelityFX/docs/whats-new/version_2_1_0.md)
  - [AMD FidelityFX™ SDK 2.0.0](Kits/FidelityFX/docs/whats-new/version_2_0_0.md)
  - [AMD FidelityFX™ SDK 1.1.4](Kits/FidelityFX/docs/whats-new/version_1_1_4.md)
  - [AMD FidelityFX™ SDK 1.1.3](Kits/FidelityFX/docs/whats-new/version_1_1_3.md)
  - [AMD FidelityFX™ SDK 1.1.2](Kits/FidelityFX/docs/whats-new/version_1_1_2.md)
  - [AMD FidelityFX™ SDK 1.1.1](Kits/FidelityFX/docs/whats-new/version_1_1_1.md)
  - [AMD FidelityFX™ SDK 1.1](Kits/FidelityFX/docs/whats-new/version_1_1.md)
  - [AMD FidelityFX™ SDK 1.0](Kits/FidelityFX/docs/whats-new/version_1_0.md)

- [Getting started](docs/getting-started/index.md)
  - [SDK structure](docs/getting-started/sdk-structure.md)
  - [Building the samples](docs/getting-started/building-samples.md)
  - [Running the samples](docs/getting-started/running-samples.md)

- [Tools](docs/tools/index.md)
  - [AMD FSR™ SDK Media Delivery System](docs/tools/media-delivery.md)

<h2>Known issues</h2>

| AMD FSR™ SDK Sample | API / Configuration | Problem Description |
| --- | --- | --- |
| All AMD FSR™ SDK Samples | All APIs / All Configs | Windows path length restrictions may cause compile issues. It is recommended to place the SDK close to the root of a drive or use subst or a mklink to shorten the path. |
| FSR™ Ray Regeneration and Neural Radiance Caching Samples | Vulkan® / All Configs | Vulkan® is currently not supported for Ray Regeneration and Neural Radiance Caching |
| AMD FSR™ Upscaling/Frame Generation Sample | All APIs / All Configs | Certain UI interactions can cause GPU crashes on Nvidia GPUs |
| AMD FSR™ Upscaling/Frame Generation Sample | All APIs / All Configs | Toggling fullscreen mode can cause mouse to temporarily freeze |
| AMD FSR™ Upscaling/Frame Generation Sample | All APIs / All Configs | Optical Flow Motion Vector Field debug view displays rapid flashing lines while viewing the skybox |

<h2>Open source</h2>

AMD FSR™ Samples are open source, and available under the MIT license.

For more information on the license terms please refer to [license](docs/license.md).

<h2>Disclaimer</h2>

The information contained herein is for informational purposes only, and is subject to change without notice. While every precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical errors, and AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with respect to the operation or use of AMD hardware, software or other products described herein. No license, including implied or arising by estoppel, to any intellectual property rights is granted by this document. Terms and limitations applicable to the purchase or use of AMD’s products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions of Sale.

AMD, the AMD Arrow logo, Radeon, Ryzen, CrossFire, RDNA and combinations thereof are trademarks of Advanced Micro Devices, Inc. Other product names used in this publication are for identification purposes only and may be trademarks of their respective companies.

DirectX is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Microsoft is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Windows is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

© 2022-2026 Advanced Micro Devices, Inc. All rights reserved.
