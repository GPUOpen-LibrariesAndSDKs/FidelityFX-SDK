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
#define FFX_FRAMEINTERPOLATION_BIND_SRV_PREV_BB									12
#define FFX_FRAMEINTERPOLATION_BIND_SRV_CURR_BB									13
#define FFX_FRAMEINTERPOLATION_BIND_SRV_PREV_BB_DOWN                            14
#define FFX_FRAMEINTERPOLATION_BIND_SRV_CURR_BB_DOWN                            15
#define FFX_FRAMEINTERPOLATION_BIND_SRV_HUD_LESS                                16
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID                      17

#define FFX_FRAMEINTERPOLATION_BIND_UAV_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME	0
#define FFX_FRAMEINTERPOLATION_BIND_UAV_STATIC_CONTENT_MASK						1

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

void StoreReconstructedDepthInterpolatedFrame(int2 iPxSample, float fDepth)
{
    uint uDepth = asuint(fDepth);

#if FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH
    InterlockedMax(rw_reconstructed_depth_interpolated_frame[iPxSample], uDepth);
#else
    InterlockedMin(rw_reconstructed_depth_interpolated_frame[iPxSample], uDepth);  // min for standard, max for inverted depth
#endif
}

/*
void StoreReconstructedDepthPreviousFrame(int2 iPxSample, float fDepth)
{
    uint uDepth = asuint(fDepth);

#if FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH
    InterlockedMax(rw_reconstructed_depth_previous_frame[iPxSample], uDepth);
#else
    InterlockedMin(rw_reconstructed_depth_previous_frame[iPxSample], uDepth);  // min for standard, max for inverted depth
#endif
}
*/

void ReconstructPrevDepth(int2 iPxPos, uint depthTarget, float fDepth, float2 fMotionVector, int2 iPxDepthSize)
{
    float2 fDepthUv   = (iPxPos + float(0.5)) / iPxDepthSize;
    float2 fPxPrevPos = (fDepthUv + fMotionVector) * iPxDepthSize - float2(0.5, 0.5);
    int2   iPxPrevPos = int2(floor(fPxPrevPos));
    float2 fPxFrac    = frac(fPxPrevPos);

    const float bilinearWeights[2][2] = {{(1 - fPxFrac.x) * (1 - fPxFrac.y), (fPxFrac.x) * (1 - fPxFrac.y)},
                                         {(1 - fPxFrac.x) * (fPxFrac.y), (fPxFrac.x) * (fPxFrac.y)}};

    // Project current depth into previous frame locations.
    // Push to all pixels having some contribution if reprojection is using bilinear logic.
    for (int y = 0; y <= 1; ++y)
    {
        for (int x = 0; x <= 1; ++x)
        {
            int2  offset = int2(x, y);
            float w      = bilinearWeights[y][x];

            if (w > fReconstructedDepthBilinearWeightThreshold)
            {
                int2 storePos = iPxPrevPos + offset;
                if (IsOnScreen(storePos, RenderSize()))
                {
                    if (depthTarget == 0) {
                        //StoreReconstructedDepthPreviousFrame(storePos, fDepth);
                    }
                    else {
                        StoreReconstructedDepthInterpolatedFrame(storePos, fDepth);
                    }
                }
            }
        }
    }
}

float CalcSpatialDiff(uint2 dtID)
{
    float3 c0 = Tonemap(r_prevBB_downsampled[dtID].rgb);
    float m0 = RGBToPerceivedLuma(c0);
    float3 c1 = Tonemap(r_currBB_downsampled[dtID].rgb);
    float m1 = RGBToPerceivedLuma(c1);
    float delta = abs(m0 - m1);

    return all(abs(c0 - c1) > 0.025f);

    return saturate(delta) < 0.025f;
}

[numthreads(8, 8, 1)] void CS(uint2 dtID : SV_DispatchThreadID) {
    float2 fMotionVector = r_dilated_motion_vectors[dtID];
    float  fDilatedDepth = r_dilated_depth[dtID];

    if (all(dtID < RenderSize())) {
        //ReconstructPrevDepth(dtID, 0, fDilatedDepth, fMotionVector, DisplaySize());
        ReconstructPrevDepth(dtID, 1, fDilatedDepth, fMotionVector * 0.5f, RenderSize());
    }

    //StoreReconstructedDepth(dtID, fDilatedDepth);

    float m0 = RGBToPerceivedLuma(r_currBB[dtID].rgb);
    float m1 = RGBToPerceivedLuma(r_currHUDLess[dtID].rgb);

    float ml = max(length(r_currBB[dtID].rgb), length(r_currHUDLess[dtID].rgb));

    float2 maskVal = rw_static_content_mask[dtID];

    // TODO: Have current and previous R8 resource to temporally manage static content mask!
    maskVal[STATIC_CONTENT_MASK_PREVIOUS] = maskVal[STATIC_CONTENT_MASK_CURRENT];
    maskVal[STATIC_CONTENT_MASK_CURRENT] = ffxSaturate((1.0f - NormalizedDot3(r_currBB[dtID].rgb, r_currHUDLess[dtID].rgb)) / 0.1f);

    //rw_debug_out[dtID] = maskVal * HUDLessAttachedFactor;
    rw_static_content_mask[dtID] = maskVal * HUDLessAttachedFactor;
}
