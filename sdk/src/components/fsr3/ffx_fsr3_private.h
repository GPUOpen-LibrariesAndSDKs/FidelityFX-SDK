// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#pragma once
#include <FidelityFX/gpu/fsr3/ffx_fsr3_resources.h>
#include <FidelityFX/host/ffx_fsr3upscaler.h>
#include <FidelityFX/host/ffx_frameinterpolation.h>
#include <FidelityFX/host/ffx_opticalflow.h>
#include <FidelityFX/host/ffx_fsr3.h>

// max queued frames for descriptor management
#define FSR3_MAX_QUEUED_FRAMES 2

// FfxFsr3Context_Private
// The private implementation of the FSR3 context.
// Actually this is only a container for Upscaler+Frameinterpolation+OpticalFlow
typedef struct FfxFsr3Context_Private {
    FfxFsr3ContextDescription           description;
    FfxInterface                        backendInterfaceSharedResources;
    FfxInterface                        backendInterfaceUpscaling;
    FfxInterface                        backendInterfaceFrameInterpolation;
    FfxFsr3UpscalerContext              upscalerContext;
    FfxOpticalflowContext               ofContext;
    FfxFrameInterpolationContext        fiContext;
    FfxResourceInternal                 upscalerResources[FFX_FSR3_RESOURCE_IDENTIFIER_COUNT];
    FfxUInt32                           effectContextIdSharedResources;
    FfxUInt32                           effectContextIdUpscaling;
    FfxUInt32                           effectContextIdFrameGeneration;
    float                               deltaTime;
    bool                                upscalingOnly;
    bool                                asyncWorkloadSupported;
    FfxUInt32                           sharedResourceCount;
    FfxUInt32                           frameIndexUpscaling;
    FfxUInt32                           frameIndexFrameGeneration;
    FfxUInt32                           skippedPresentsCount;

    FfxResource                         HUDLess_color;

    bool                                frameGenerationEnabled;
    int32_t                             frameGenerationFlags;
    FfxFsr3DispatchUpscaleDescription   upscaleDescriptions[FSR3_MAX_QUEUED_FRAMES];

} FfxFsr3Context_Private;
