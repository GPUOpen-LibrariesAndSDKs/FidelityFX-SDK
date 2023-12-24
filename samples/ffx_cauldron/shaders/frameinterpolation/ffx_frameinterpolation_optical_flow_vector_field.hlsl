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

void computeOpticalFlowFieldMvs(uint2 dtID, FfxFloat32x2 fOpticalFlowVector)
{
    float2 fUv = float2(float2(dtID)+0.5f) / GetOpticalFlowSize2();

    const float scaleFactor = 1.0f;
    float2 fMotionVectorHalf = fOpticalFlowVector * 0.5f;

    float2 fPxPrevPos = ((fUv + fMotionVectorHalf) * GetOpticalFlowSize2() * scaleFactor) - float2(0.5, 0.5);
    int2 iPxPrevPos = int2(floor(fPxPrevPos));
    float2 fPxFrac = frac(fPxPrevPos);

    float fDilatedDepth = ConvertFromDeviceDepthToViewSpace(r_dilated_depth[dtID]);

    float prevLuma = 0.001f + RGBToLuma(ffxSrgbToLinear(r_prevBB.SampleLevel(s_LinearClamp, fUv, 0).xyz));
    float currLuma = 0.001f + RGBToLuma(ffxSrgbToLinear(r_currBB.SampleLevel(s_LinearClamp, fUv + fOpticalFlowVector, 0).xyz));

    float fVelocity = length(fOpticalFlowVector * DisplaySize());
    uint uDepthPriorityFactor = (fVelocity > 1.0f) * saturate(fVelocity / length(DisplaySize() * 0.05f)) * PRIORITY_DEPTH_MAX;

    if(uDepthPriorityFactor > 0) {
        uint uColorPriorityFactor = round(pow(MinDividedByMax(prevLuma, currLuma), 1.0f / 1.0f) * PRIORITY_COLOR_MAX)
            * IsUvInside(fUv + fOpticalFlowVector);

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
                    if (IsOnScreen(storePos, GetOpticalFlowSize2() * scaleFactor))
                    {
                        //uColorPriorityFactor = (dtID.x * dtID.y) % PRIORITY_COLOR_MAX;
                        uint2 packedVector = PackVectorFieldEntries(uDepthPriorityFactor, uColorPriorityFactor, fMotionVectorHalf);

                        InterlockedMax(rw_optical_flow_motion_vector_field_x[storePos], packedVector.x);
                        InterlockedMax(rw_optical_flow_motion_vector_field_y[storePos], packedVector.y);
                    }
                }
            }
        }
    }
}

FfxFloat32 ComputeMotionDivergence(FfxInt32x2 iPxPos)
{
    FfxFloat32 minconvergence = 1.0f;

    FfxFloat32x2 fMotionVectorNucleus = LoadOpticalFlow(iPxPos);
    FfxFloat32 fNucleusVelocityLr = length(fMotionVectorNucleus * GetOpticalFlowSize2());
    FfxFloat32 fMaxVelocityUv = length(fMotionVectorNucleus);

    const FfxFloat32 MotionVectorVelocityEpsilon = 1e-02f;

    if (fNucleusVelocityLr > MotionVectorVelocityEpsilon) {
        for (FfxInt32 y = -1; y <= 1; ++y) {
            for (FfxInt32 x = -1; x <= 1; ++x) {

                FfxInt32x2 sp = iPxPos + FfxInt32x2(x, y); //ClampLoad(iPxPos, FfxInt32x2(x, y), GetOpticalFlowSize2());

                FfxFloat32x2 fMotionVector = LoadOpticalFlow(sp);
                FfxFloat32 fVelocityUv = length(fMotionVector);

                if(length(fMotionVector * DisplaySize()) > 1.0f) {
                    fMaxVelocityUv = ffxMax(fVelocityUv, fMaxVelocityUv);
                    fVelocityUv = ffxMax(fVelocityUv, fMaxVelocityUv);
                    minconvergence = ffxMin(minconvergence, dot(fMotionVector / fVelocityUv, fMotionVectorNucleus / fVelocityUv));
                }
            }
        }
    }

    return ffxSaturate(1.0f - minconvergence) * ffxSaturate(fMaxVelocityUv / 0.01f);
}

[numthreads(8, 8, 1)]
void CS(uint2 dtID : SV_DispatchThreadID)
{

    //FfxFloat32 fMotionDivergence = ComputeMotionDivergence(dtID);

    FfxFloat32x2 fOpticalFlowVector = 0;
    FfxFloat32x2 fOpticalFlowVector3x3Avg = 0;
    int size = 1;
    float sw = 0.0f;

    for(int y = -size; y <= size; y++) {
        for(int x = -size; x <= size; x++) {

            int2 samplePos = dtID + int2(x,y);

            float2 vs = LoadOpticalFlow(samplePos);
            float fConfidenceFactor = max(FSR2_EPSILON, LoadOpticalFlowConfidence(samplePos));


            float len = length(vs * DisplaySize());
            float len_factor = max(0.0f, 512.0f - len) * (len > 1.0f);
            float w = len_factor;

            fOpticalFlowVector3x3Avg += vs * w;

            sw += w;
        }
    }

    fOpticalFlowVector3x3Avg /= sw;


    sw = 0.0f;
    for(int y = -size; y <= size; y++) {
        for(int x = -size; x <= size; x++) {

            int2 samplePos = dtID + int2(x,y);

            float2 vs = LoadOpticalFlow(samplePos);

            float fConfidenceFactor = max(FSR2_EPSILON, LoadOpticalFlowConfidence(samplePos));
            float len = length(vs * DisplaySize());
            float len_factor = max(0.0f, 512.0f - len) * (len > 1.0f);


            float w = max(0.0f, pow(dot(fOpticalFlowVector3x3Avg, vs), 1.25f)) * len_factor;

            fOpticalFlowVector += vs * w;
            sw += w;
        }
    }

    if(sw > FSR2_EPSILON) {
        fOpticalFlowVector /= sw;
    }

    computeOpticalFlowFieldMvs(dtID, fOpticalFlowVector);
}
