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

#define FFX_FRAMEINTERPOLATION_BIND_SRV_GAME_MOTION_VECTOR_FIELD_X				0
#define FFX_FRAMEINTERPOLATION_BIND_SRV_GAME_MOTION_VECTOR_FIELD_Y				1
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X		2
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y		3
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW							4
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_CONFIDENCE					5
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_GLOBAL_MOTION				6
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_SCENE_CHANGE_DETECTION		7
#define FFX_FRAMEINTERPOLATION_BIND_SRV_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME		8
#define FFX_FRAMEINTERPOLATION_BIND_SRV_DILATED_MOTION_VECTORS					9
#define FFX_FRAMEINTERPOLATION_BIND_SRV_DILATED_DEPTH							10
#define FFX_FRAMEINTERPOLATION_BIND_SRV_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME	11
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID						12

#define FFX_FRAMEINTERPOLATION_BIND_UAV_OUTPUT									0

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

FfxFloat32x4 ComputeInpaintingLevel(float2 fUv, const FfxInt32 iMipLevel, const FfxInt32x2 iTexSize)
{
    BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fUv, iTexSize);

    FfxFloat32x4 fColor = 0.0f;

    for (FfxInt32 iSampleIndex = 0; iSampleIndex < 4; iSampleIndex++) {

        const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
        const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

        if (IsOnScreen(iSamplePos, iTexSize)) {

            FfxFloat32x4 fSample = r_InpaintingPyramid.mips[iMipLevel][iSamplePos];

            const FfxFloat32 fWeight = bilinearInfo.fWeights[iSampleIndex] * (fSample.w > 0.0f);

            fColor += float4(fSample.rgb * fWeight, fWeight);
        }
    }

    return fColor;
}

FfxFloat32x3 ComputeInpainting(uint2 iPxPos)
{
    float2 fUv = (iPxPos + 0.5f) / (DisplaySize());

    FfxFloat32x4 fColor = 0;
    FfxInt32x2 iTexSize = DisplaySize();

    for (FfxInt32 iMipLevel = 0; iMipLevel < 10 && (fColor.w < 0.01f); iMipLevel++) {

        iTexSize *= 0.5f;

        fColor = ComputeInpaintingLevel(fUv, iMipLevel, iTexSize);
    }

    return fColor.rgb / fColor.w;
}

[numthreads(8, 8, 1)]
void CS(uint2 dtID : SV_DispatchThreadID)
{
    
    float4 fColor = rw_FiOutput[dtID];

    
    if (fColor.w == 0.0f) {
        fColor.rgb = ComputeInpainting(dtID) * (DisplaySize().x > 0);

        rw_FiOutput[dtID] = float4(fColor.rgb, 1.0f);
    }

    /*
    if (fColor.w == 0.0f) {
        // wip, do manual bilinear sampling to stay at as high level as possible
        for (float i = 1; i < 6 && fColor.w < 1.0f; i += 1.0f) {
            fColor = r_InpaintingPyramid.SampleLevel(s_LinearClamp, fUv, i);
        }
    }
    */

    //float2 fUv = (dtID + 0.5f) / (DisplaySize());
    //fColor = r_InpaintingPyramid.SampleLevel(s_LinearClamp, fUv, 0);

    //fColor.rgb = r_currHUDLess[dtID];
}
