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
#define FFX_FRAMEINTERPOLATION_BIND_SRV_DISOCCLUSION_MASK						14
#define FFX_FRAMEINTERPOLATION_BIND_SRV_STATIC_CONTENT_MASK                     15
#define FFX_FRAMEINTERPOLATION_BIND_SRV_HUD_LESS                                16
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID                      17
#define FFX_FRAMEINTERPOLATION_BIND_SRV_COUNTERS                                18

#define FFX_FRAMEINTERPOLATION_BIND_UAV_OUTPUT									0

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

float4 SampleTextureBilinear(uint2 iPxPos, Texture2D <float4>tex, float2 fUv, float2 fMotionVector, int2 texSize, int staticMaskChannel)
{
    float2 fReprojectedUv = fUv + fMotionVector;
    BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fReprojectedUv, texSize);

    float3 fColor = 0.0f;
    float fWeightSum = 0.0f;
    for (FfxInt32 iSampleIndex = 0; iSampleIndex < 4; iSampleIndex++) {

        const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
        const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

        if (IsOnScreen(iSamplePos, texSize)) {

            FfxFloat32 fWeight = bilinearInfo.fWeights[iSampleIndex];

            fWeight *= ffxSaturate(1.0f - r_static_content_mask[iSamplePos][staticMaskChannel]);

            fColor += tex[iSamplePos].rgb * fWeight;
            fWeightSum += fWeight;
        }
    }

    //normalize colors
    fColor = (fWeightSum != 0.0f) ? fColor / fWeightSum : tex[iPxPos].rgb;
    return float4(fColor, fWeightSum);
}

void drawDebug(uint2 iPxPos, inout float3 fColor, inout bool bColorInPainting) {

    if (iPxPos.y < 64 && iPxPos.x < 64 && HasSceneChanged())
        fColor.r = 1.f;

    if (iPxPos.x < 16) {
        fColor.g = 1.f;
    }
    if (iPxPos.x > DisplaySize().x - 16) {
        fColor += debugBarColor;
    }

#if 0
    if (length(iPxPos - DisplaySize() * 0.5f) < 25.0f) {
    //if (iPxPos.x % 4 < 2) {
        bColorInPainting = true;
        fColor = 0;
    }
#endif

    // OF verification for FFX_INTERNAL disabled
    if (0)
    {
        float2 of = r_optical_flow[iPxPos / opticalFlowBlockSize];
        float2 fof = -of;
        float2 n = normalize(of.x == 0 && of.y == 0 ? float2(1.0, 0.0) : fof);
        float len = length(fof);
        float phi = n.y < 0.0 ? acos(n.x) : -acos(n.x) + 2 * FFX_PI;
        float h = phi / (2 * FFX_PI);
        float s = len * 0.04;
        float v = 0.5;
        float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        float3 p = abs(frac(h.xxx + K.xyz) * 6.0 - K.www);
        float3 hsv2rgbr = v * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), s);
        fColor.rgb = hsv2rgbr;
    }
}

float staticComparison(uint2 iPxPos)
{
    float fPrev = RGBToLuma(ffxSrgbToLinear(r_prevBB[iPxPos].rgb));
    float fCurr = RGBToLuma(ffxSrgbToLinear(r_currBB[iPxPos].rgb));

    return MinDividedByMax(fPrev, fCurr);
}

void computeInterpolatedColor(uint2 iPxPos, out float3 fInterpolatedColor, out bool bColorInPainting)
{
    float2 fUv = (float2(iPxPos) + 0.5f) / DisplaySize();

    fInterpolatedColor = 0;
    bColorInPainting = false;

    float2 fLrUv  = fUv * (float2(renderSize) / maxRenderSize);
    uint2  uLrPos  = round(fUv * renderSize);


    VectorFieldEntry gameMv;
    LoadGameFieldMv2(fUv, gameMv);

    VectorFieldEntry ofMv;
    SampleOpticalFlowMotionVectorField2(fUv, ofMv);

    bool bUseGameMvs = true;

    float2 fDisocclusionFactor = saturate(r_disocclusion_mask.SampleLevel(s_LinearClamp, fLrUv, 0).xy);

    if (!gameMv.bValid && !gameMv.bNegOutside && !gameMv.bPosOutside) {
        gameMv.fMotionVector *= fDisocclusionFactor.x;
    }

    //gameMv.fMotionVector *= bUseGameMvs;

    float4 fPrevColorGame = SampleTextureBilinear(iPxPos, r_prevBB, fUv, +gameMv.fMotionVector, DisplaySize(), STATIC_CONTENT_MASK_PREVIOUS);
    float4 fCurrColorGame = SampleTextureBilinear(iPxPos, r_currBB, fUv, -gameMv.fMotionVector, DisplaySize(), STATIC_CONTENT_MASK_CURRENT);

    float4 fPrevColorOF = SampleTextureBilinear(iPxPos, r_prevBB, fUv, +ofMv.fMotionVector, DisplaySize(), STATIC_CONTENT_MASK_PREVIOUS);
    float4 fCurrColorOF = SampleTextureBilinear(iPxPos, r_currBB, fUv, -ofMv.fMotionVector, DisplaySize(), STATIC_CONTENT_MASK_CURRENT);

    float fBilinearWeightSum = 0.0f;
    
    if (bUseGameMvs)
    {
        fDisocclusionFactor.x *= !gameMv.bNegOutside;
        fDisocclusionFactor.y *= !gameMv.bPosOutside;

        float t = 0.5f;
        t += 0.5f * (1 - (fDisocclusionFactor.x));
        t -= 0.5f * (1 - (fDisocclusionFactor.y));

        // bias towards current frame depending on color similarity
        if(abs(t - 0.5f) < FSR2_EPSILON) {
            float fSim = NormalizedDot3(fPrevColorGame.rgb, fCurrColorGame.rgb);
            fSim = pow(fSim, 6.0f);
            t = lerp(1.0f, t, fSim);
        }

        fInterpolatedColor = lerp(fPrevColorGame.xyz, fCurrColorGame.xyz, ffxSaturate(t));
        fBilinearWeightSum = lerp(fPrevColorGame.w, fCurrColorGame.w, ffxSaturate(t));
        
        if (gameMv.bNegOutside && gameMv.bPosOutside) {
            bColorInPainting = true;
            fInterpolatedColor = 1;
        }
        else if (fPrevColorGame.w == 0.0f) {
            fInterpolatedColor = fCurrColorGame.xyz;
            fBilinearWeightSum = fCurrColorGame.w;
        }
        else if (fCurrColorGame.w == 0.0f) {
            fInterpolatedColor = fPrevColorGame.xyz;
            fBilinearWeightSum = fPrevColorGame.w;
        }
    }

    
    //fInterpolatedColor = lerp(fInterpolatedColor.rgb, r_currBB[iPxPos].rgb, fBilinearWeightSum <= 0.01f);
    /*
    if (r_static_content_mask[iPxPos] > FSR2_EPSILON)
    {
        fInterpolatedColor = r_currBB[iPxPos].rgb;
    } else
    */

    #if 1
    {

        float ofT = 0.5f;

        if(fPrevColorOF.w > 0 && fCurrColorOF.w > 0) {
            ofT = 0.5f;
        } else if(fPrevColorOF.w > 0) {
            ofT = 0;
        } else {
            ofT = 1;
        }

        float fOF_Sim = NormalizedDot3(fPrevColorOF.rgb, fCurrColorOF.rgb);
        fOF_Sim = pow(fOF_Sim, 6.0f);

        // bias towards current frame depending on color similarity
        if(abs(ofT - 0.5f) < FSR2_EPSILON) {
            ofT = lerp(1.0f, ofT, fOF_Sim);
        }

        float3 ofColor = lerp(fPrevColorOF.rgb, fCurrColorOF.rgb, ofT);

        float fMvCmp = NormalizedDot2(gameMv.fMotionVector, ofMv.fMotionVector);
        float t = pow(max(0, fMvCmp), 1.0f / 6.0f);
        t = lerp(1.0f - t, 0.0f, saturate(length(gameMv.fMotionVector * DisplaySize()) / 256.0f));
        if(gameMv.bInPainted) {
            t = 0;
        }

        t = lerp(t, 0.0f, pow(MinDividedByMax(0.01f + gameMv.fVelocity, 0.01f + ofMv.fVelocity), 6.0f));

        float fStatic = pow(staticComparison(iPxPos), 6.0f);

        fStatic *= fOF_Sim;
        t = lerp(t, 0.0f, fStatic); //use gameMvs based on static comparison

        const float fFrameIndexFactor = ffxSaturate(FrameIndexSinceLastReset() > 5.0f);
        t *= fFrameIndexFactor;

        fInterpolatedColor = lerp(fInterpolatedColor, ofColor, ffxSaturate(t));
    }
#endif

    //fInterpolatedColor = lerp(fPrevColorGame.xyz, fCurrColorGame.xyz, 0.5f);
    
    // Do InPainting in non-static areas where no valid samples where interpolated
    bColorInPainting |= (max(fPrevColorGame.w, fCurrColorGame.w) == 0.0f) && (r_static_content_mask[iPxPos][STATIC_CONTENT_MASK_CURRENT] < 0.1f);
}

[numthreads(8,8,1)]
void CS(uint2 iPxPos : SV_DispatchThreadID)
{
    float3 fColor = float3(0,0,0);
    bool bColorInPainting = false;

    if(FrameIndexSinceLastReset() > 0) {
        computeInterpolatedColor(iPxPos, fColor, bColorInPainting);
    } else {
        fColor = r_currBB[iPxPos].rgb;
    }
    
    drawDebug(iPxPos, fColor, bColorInPainting);

    rw_FiOutput[iPxPos] = float4(fColor, !bColorInPainting);
}
