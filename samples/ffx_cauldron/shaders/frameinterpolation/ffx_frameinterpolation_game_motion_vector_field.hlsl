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
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID                      12
#define FFX_FRAMEINTERPOLATION_BIND_SRV_PREV_BB									13
#define FFX_FRAMEINTERPOLATION_BIND_SRV_CURR_BB									14

#define FFX_FRAMEINTERPOLATION_BIND_UAV_GAME_MOTION_VECTOR_FIELD_X				0
#define FFX_FRAMEINTERPOLATION_BIND_UAV_GAME_MOTION_VECTOR_FIELD_Y				1
#define FFX_FRAMEINTERPOLATION_BIND_UAV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X		2
#define FFX_FRAMEINTERPOLATION_BIND_UAV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y		3

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

void computeGameFieldMvs(uint2 dtID, float2 fGameMotionVector)
{
    float2 fUv = float2(float2(dtID)+0.5f) / RenderSize();

    //fGameMotionVector.x = -100.0f / RenderSize();

    float2 fMotionVectorHalf = fGameMotionVector * 0.5f;

    float2 fPxPrevPos = ((fUv + fMotionVectorHalf) * RenderSize()) - float2(0.5, 0.5);
    int2 iPxPrevPos = int2(floor(fPxPrevPos));
    float2 fPxFrac = frac(fPxPrevPos);

    float fDepthSample = LoadEstimatedInterpolationFrameDepth(dtID);
    fDepthSample = r_dilated_depth[dtID];

    float fViewSpaceDepth = ConvertFromDeviceDepthToViewSpace(fDepthSample); // / 1000.0f;

    uint uDepthPriorityFactor = getPriorityFactorFromViewSpaceDepth(fViewSpaceDepth);

    //rw_debug_out[dtID] = uPriorityFactor / float(maxVal);

    float prevLuma = 0.01f + RGBToLuma(ffxSrgbToLinear(r_prevBB.SampleLevel(s_LinearClamp, fUv, 0).xyz));
    float currLuma = 0.01f + RGBToLuma(ffxSrgbToLinear(r_currBB.SampleLevel(s_LinearClamp, fUv + fGameMotionVector, 0).xyz));

    //uint uColorPriorityFactor = MinDividedByMax(prevLuma, currLuma) * PRIORITY_COLOR_MAX;
    uint uColorPriorityFactor = round(pow(MinDividedByMax(prevLuma, currLuma), 1.0f / 1.0f) * PRIORITY_COLOR_MAX)
    * IsUvInside(fUv + fGameMotionVector);

    uint2 packedVector = PackVectorFieldEntries(uDepthPriorityFactor, uColorPriorityFactor, fMotionVectorHalf);

    //rw_debug_out[dtID] = float4(0, uColorPriorityFactor, 0, 1);

    const float bilinearWeights[2][2] = {
        {
            (1 - fPxFrac.x) * (1 - fPxFrac.y),
            (fPxFrac.x) * (1 - fPxFrac.y)
        },
        {
            (1 - fPxFrac.x) * (fPxFrac.y),
            (fPxFrac.x) * (fPxFrac.y)
        }
    };

    // Project current depth into previous frame locations.
    // Push to all pixels having some contribution if reprojection is using bilinear logic.
    for (int y = 0; y <= 1; ++y) {
        for (int x = 0; x <= 1; ++x) {

            int2 offset = int2(x, y);
            float w = bilinearWeights[y][x];

            if (w > 0.0f) {

                int2 storePos = iPxPrevPos + offset;
                if (IsOnScreen(storePos, RenderSize()))
                {
                    InterlockedMax(rw_game_motion_vector_field_x[storePos], packedVector.x);
                    InterlockedMax(rw_game_motion_vector_field_y[storePos], packedVector.y);
                }
            }
        }
    }
}

[numthreads(8, 8, 1)]
void CS(uint2 dtID : SV_DispatchThreadID)
{
    float2 fGameMotionVector = r_dilated_motion_vectors[dtID];

    computeGameFieldMvs(dtID, fGameMotionVector);
}
