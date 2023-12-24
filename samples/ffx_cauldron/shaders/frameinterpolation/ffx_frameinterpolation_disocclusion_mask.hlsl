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

#define FFX_FRAMEINTERPOLATION_BIND_UAV_DISOCCLUSION_MASK						0

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

// #define FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH 1

static const float DepthClipBaseScale = 1.0f;

float TanHalfFoV()
{
    return 1.777777;
}

float ComputeSampleDepthClip(int2 iPxSamplePos, float fPreviousDepth, float fPreviousDepthBilinearWeight, float fCurrentDepthViewSpace)
{
    float fPrevNearestDepthViewSpace = ConvertFromDeviceDepthToViewSpace(fPreviousDepth);

    // Depth separation logic ref: See "Minimum Triangle Separation for Correct Z-Buffer Occlusion"
    // Intention: worst case of formula in Figure4 combined with Ksep factor in Section 4
    // TODO: check intention and improve, some banding visible
    const float fHalfViewportWidth = renderSize.x * 0.5f;
    float       fDepthThreshold    = max(fCurrentDepthViewSpace, fPrevNearestDepthViewSpace);

    // WARNING: Ksep only works with reversed-z with infinite projection.
    const float Ksep                     = 1.37e-05f;
    float       fRequiredDepthSeparation = Ksep * fDepthThreshold * TanHalfFoV() * fHalfViewportWidth;
    float       fDepthDiff               = fCurrentDepthViewSpace - fPrevNearestDepthViewSpace;

    float fDepthClipFactor = (fDepthDiff > 0) ? saturate(fRequiredDepthSeparation / fDepthDiff) : 1.0f;

#ifdef _DEBUG
    //rw_debug_out[iPxSamplePos] = float4(fCurrentDepthViewSpace, fPrevNearestDepthViewSpace, fDepthDiff, fDepthClipFactor);
#endif

    return fPreviousDepthBilinearWeight * fDepthClipFactor * lerp(1.0f, DepthClipBaseScale, saturate(fDepthDiff * fDepthDiff));
}

float LoadEstimatedDepth(uint estimatedIndex, int2 iSamplePos)
{
    if (estimatedIndex == 0)
    {
        return asfloat(r_reconstructed_depth_previous_frame[iSamplePos]);
    }
    else if (estimatedIndex == 1)
    {
        return r_dilated_depth[iSamplePos];
    }

    return 0;
}

float ComputeDepthClip(uint estimatedIndex, float2 fUvSample, float fCurrentDepthViewSpace)
{
    float2 fPxSample = fUvSample * RenderSize() - 0.5f;
    int2   iPxSample = int2(floor(fPxSample));
    float2 fPxFrac   = frac(fPxSample);

    const float fBilinearWeights[2][2] = {{(1 - fPxFrac.x) * (1 - fPxFrac.y), (fPxFrac.x) * (1 - fPxFrac.y)},
                                          {(1 - fPxFrac.x) * (fPxFrac.y), (fPxFrac.x) * (fPxFrac.y)}};

    float fDepth     = 0.0f;
    float fWeightSum = 0.0f;
    for (int y = 0; y <= 1; ++y)
    {
        for (int x = 0; x <= 1; ++x)
        {
            int2 iSamplePos = iPxSample + int2(x, y);
            if (IsOnScreen(iSamplePos, RenderSize()))
            {
                float fBilinearWeight = fBilinearWeights[y][x];

                if (fBilinearWeight > fReconstructedDepthBilinearWeightThreshold)
                {
                    float estimatedDepth = LoadEstimatedDepth(estimatedIndex, iSamplePos);

                    //if (fCurrentDepthViewSpace < 1000) //ConvertFromDeviceDepthToViewSpace(0))
                    {
                        fDepth += ComputeSampleDepthClip(iSamplePos, estimatedDepth, fBilinearWeight, fCurrentDepthViewSpace);
                        fWeightSum += fBilinearWeight;
                    }
                }
            }
        }
    }

    return (fWeightSum > 0) ? fDepth / fWeightSum : DepthClipBaseScale;
}

FfxFloat32 ComputeDepthClip2(uint estimatedIndex, FfxFloat32x2 fUvSample, FfxFloat32 fCurrentDepthSample)
{
    FfxFloat32 fCurrentDepthViewSpace = GetViewSpaceDepth(fCurrentDepthSample);
    BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fUvSample, RenderSize());

    FfxFloat32 fDilatedSum = 0.0f;
    FfxFloat32 fDepth = 0.0f;
    FfxFloat32 fWeightSum = 0.0f;
    for (FfxInt32 iSampleIndex = 0; iSampleIndex < 4; iSampleIndex++) {

        const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
        const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

        if (IsOnScreen(iSamplePos, RenderSize())) {
            const FfxFloat32 fWeight = bilinearInfo.fWeights[iSampleIndex];
            if (fWeight > fReconstructedDepthBilinearWeightThreshold) {

                //const FfxFloat32 fPrevDepthSample = LoadReconstructedPrevDepth(iSamplePos);
                const FfxFloat32 fPrevDepthSample = LoadEstimatedDepth(estimatedIndex, iSamplePos);
                const FfxFloat32 fPrevNearestDepthViewSpace = GetViewSpaceDepth(fPrevDepthSample);

                const FfxFloat32 fDepthDiff = fCurrentDepthViewSpace - fPrevNearestDepthViewSpace;

                if (fDepthDiff >= 0.0f) {

#if FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH
                    const FfxFloat32 fPlaneDepth = ffxMin(fPrevDepthSample, fCurrentDepthSample);
#else
                    const FfxFloat32 fPlaneDepth = ffxMax(fPrevDepthSample, fCurrentDepthSample);
#endif

                    const FfxFloat32x3 fCenter = GetViewSpacePosition(FfxInt32x2(RenderSize() * 0.5f), RenderSize(), fPlaneDepth);
                    const FfxFloat32x3 fCorner = GetViewSpacePosition(FfxInt32x2(0, 0), RenderSize(), fPlaneDepth);

                    const FfxFloat32 fHalfViewportWidth = length(FfxFloat32x2(RenderSize()));
                    const FfxFloat32 fDepthThreshold = ffxMax(fCurrentDepthViewSpace, fPrevNearestDepthViewSpace);

                    const FfxFloat32 Ksep = 1.37e-05f;
                    const FfxFloat32 Kfov = length(fCorner) / length(fCenter);
                    const FfxFloat32 fRequiredDepthSeparation = Ksep * Kfov * fHalfViewportWidth * fDepthThreshold;

                    const FfxFloat32 fResolutionFactor = ffxSaturate(length(FfxFloat32x2(RenderSize())) / length(FfxFloat32x2(1920.0f, 1080.0f)));
                    const FfxFloat32 fPower = ffxLerp(1.0f, 3.0f, fResolutionFactor);
                    //fDepth += ffxPow(ffxSaturate(FfxFloat32(fRequiredDepthSeparation / fDepthDiff)) == 1.0f, fPower) * fWeight;

                    fDepth += FfxFloat32((fRequiredDepthSeparation / fDepthDiff) >= 1.0f) * fWeight;
                    fWeightSum += fWeight;
                }
            }
        }
    }

    return (fWeightSum > 0.0f) ? ffxSaturate(1.0f - fDepth / fWeightSum) : 0.0f;
}

[numthreads(8, 8, 1)] 
void CS(uint2 iPxPos
                                              : SV_DispatchThreadID) {
    float fDilatedDepth = LoadEstimatedInterpolationFrameDepth(iPxPos);

    float2 fDepthUv = (iPxPos + 0.5f) / RenderSize();
    float fCurrentDepthViewSpace = ConvertFromDeviceDepthToViewSpace(fDilatedDepth);

    VectorFieldEntry gameMv;
    LoadGameFieldMv2(fDepthUv, gameMv);

    float fDepthClip = 1.0f;
    float fDepthClip2 = 1.0f;

    fDepthClip = 1 - ComputeDepthClip2(0, fDepthUv + gameMv.fMotionVector, fDilatedDepth);
    fDepthClip2 = 1 - ComputeDepthClip2(1, fDepthUv - gameMv.fMotionVector, fDilatedDepth);

    /*
    if (fDepthClip > fDepthClip2) {
        fDepthClip = 1.0f;
    }

    if (fDepthClip2 > fDepthClip) {
        fDepthClip2 = 1.0f;
    }
    */

    float t = 1; // max(fDepthClip, fDepthClip2);

    //fDepthClip = fDepthClip > 0.1f;
    //fDepthClip2 = fDepthClip2 > 0.1f;
    //fDepthClip2 = fDepthClip;

    //fDepthClip = lerp(fDepthClip, 1.0f, 1.0f - fDepthClip2);
    //fDepthClip2 = lerp(fDepthClip2, 1.0f, 1.0f - fDepthClip);

    float2 fDisocclusionMask = float2(fDepthClip, fDepthClip2);

    if(fDisocclusionMask.x < 1.0f && fDisocclusionMask.y < 1.0f) {
        //fDisocclusionMask = 1;
    }

    //fDisocclusionMask = 1;
    rw_disocclusion_mask[iPxPos] = fDisocclusionMask;

    //rw_debug_out[iPxPos] = ConvertFromDeviceDepthToViewSpace(LoadEstimatedDepth(1, iPxPos)); //float4(fDepthClip2, fDepthClip2, 0, 1);
    //rw_debug_out[iPxPos] = ConvertFromDeviceDepthToViewSpace(LoadEstimatedInterpolationFrameDepth(iPxPos)); //float4(fDepthClip2, fDepthClip2, 0, 1);

    //rw_debug_out[iPxPos] = float4(float2(fDepthClip / t, (fDepthClip2 / t)), 0, 1);
}
