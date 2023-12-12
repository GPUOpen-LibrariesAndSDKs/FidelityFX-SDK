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


#ifndef FFX_FRAMEINTERPOLATION_H
#define FFX_FRAMEINTERPOLATION_H

struct InterpolationSourceColor
{
    FfxFloat32x3 fRaw;
    FfxFloat32x3 fLinear;
    FfxFloat32   fBilinearWeightSum;
};

InterpolationSourceColor SampleTextureBilinear(FfxInt32x2 iPxPos, Texture2D <FfxFloat32x4>tex, FfxFloat32x2 fUv, FfxFloat32x2 fMotionVector, FfxInt32x2 texSize)
{
    InterpolationSourceColor result         = (InterpolationSourceColor)0;

    FfxFloat32x2 fReprojectedUv = fUv + fMotionVector;
    BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fReprojectedUv, texSize);

    FfxFloat32x3 fColor = 0.0f;
    FfxFloat32 fWeightSum = 0.0f;
    for (FfxInt32 iSampleIndex = 0; iSampleIndex < 4; iSampleIndex++) {

        const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
        const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

        if (IsOnScreen(iSamplePos, texSize)) {

            FfxFloat32 fWeight = bilinearInfo.fWeights[iSampleIndex];

            fColor += tex[iSamplePos].rgb * fWeight;
            fWeightSum += fWeight;
        }
    }

    //normalize colors
    fColor = (fWeightSum != 0.0f) ? fColor / fWeightSum : FfxFloat32x3(0.0f, 0.0f, 0.0f);

    result.fRaw               = fColor;
    result.fLinear            = RawRGBToLinear(fColor);
    result.fBilinearWeightSum = fWeightSum;

    return result;
}

void updateInPaintingWeight(inout FfxFloat32 fInPaintingWeight, FfxFloat32 fFactor)
{
    fInPaintingWeight = ffxSaturate(ffxMax(fInPaintingWeight, fFactor));
}

void computeInterpolatedColor(FfxUInt32x2 iPxPos, out FfxFloat32x3 fInterpolatedColor, inout FfxFloat32 fInPaintingWeight)
{
    const FfxFloat32x2 fUv = (FfxFloat32x2(iPxPos) + 0.5f) / DisplaySize();
    const FfxFloat32x2 fLrUv  = fUv * (FfxFloat32x2(RenderSize()) / GetMaxRenderSize());

    VectorFieldEntry gameMv;
    LoadInpaintedGameFieldMv(fUv, gameMv);

    VectorFieldEntry ofMv;
    SampleOpticalFlowMotionVectorField(fUv, ofMv);

    // Binarize disucclusion factor
    FfxFloat32x2 fDisocclusionFactor = (ffxSaturate(SampleDisocclusionMask(fLrUv).xy) == 1.0f);

    InterpolationSourceColor fPrevColorGame = SampleTextureBilinear(iPxPos, PreviousBackbufferTexture(), fUv, +gameMv.fMotionVector, DisplaySize());
    InterpolationSourceColor fCurrColorGame = SampleTextureBilinear(iPxPos, CurrentBackbufferTexture(), fUv, -gameMv.fMotionVector, DisplaySize());

    InterpolationSourceColor fPrevColorOF = SampleTextureBilinear(iPxPos, PreviousBackbufferTexture(), fUv, +ofMv.fMotionVector, DisplaySize());
    InterpolationSourceColor fCurrColorOF = SampleTextureBilinear(iPxPos, CurrentBackbufferTexture(), fUv, -ofMv.fMotionVector, DisplaySize());

    FfxFloat32 fBilinearWeightSum = 0.0f;
    FfxFloat32 fDisoccludedFactor = 0.0f;

    // Disocclusion logic
    {
        fDisocclusionFactor.x *= !gameMv.bNegOutside;
        fDisocclusionFactor.y *= !gameMv.bPosOutside;

        // Inpaint in bi-directional disocclusion areas
        updateInPaintingWeight(fInPaintingWeight, length(fDisocclusionFactor) <= FFX_FRAMEINTERPOLATION_EPSILON);

        FfxFloat32 t = 0.5f;
        t += 0.5f * (1 - (fDisocclusionFactor.x));
        t -= 0.5f * (1 - (fDisocclusionFactor.y));

        fInterpolatedColor = ffxLerp(fPrevColorGame.fRaw, fCurrColorGame.fRaw, ffxSaturate(t));
        fBilinearWeightSum = ffxLerp(fPrevColorGame.fBilinearWeightSum, fCurrColorGame.fBilinearWeightSum, ffxSaturate(t));

        fDisoccludedFactor = ffxSaturate(1 - ffxMin(fDisocclusionFactor.x, fDisocclusionFactor.y));

        if (fPrevColorGame.fBilinearWeightSum == 0.0f)
        {
            fInterpolatedColor = fCurrColorGame.fRaw;
            fBilinearWeightSum = fCurrColorGame.fBilinearWeightSum;
        }
        else if (fCurrColorGame.fBilinearWeightSum == 0.0f)
        {
            fInterpolatedColor = fPrevColorGame.fRaw;
            fBilinearWeightSum = fPrevColorGame.fBilinearWeightSum;
        }
    }

    {

        FfxFloat32 ofT = 0.5f;

        if (fPrevColorOF.fBilinearWeightSum > 0 && fCurrColorOF.fBilinearWeightSum > 0)
        {
            ofT = 0.5f;
        }
        else if (fPrevColorOF.fBilinearWeightSum > 0)
        {
            ofT = 0;
        } else {
            ofT = 1;
        }

        const FfxFloat32x3 ofColor = ffxLerp(fPrevColorOF.fRaw, fCurrColorOF.fRaw, ofT);

        FfxFloat32 fOF_Sim = NormalizedDot3(fPrevColorOF.fRaw, fCurrColorOF.fRaw);
        FfxFloat32 fGame_Sim = NormalizedDot3(fPrevColorGame.fRaw, fCurrColorGame.fRaw);

        fGame_Sim = ffxLerp(ffxMax(FFX_FRAMEINTERPOLATION_EPSILON, fGame_Sim), 1.0f, ffxSaturate(fDisoccludedFactor));
        FfxFloat32 fGameMvBias = ffxPow(ffxSaturate(fGame_Sim / ffxMax(FFX_FRAMEINTERPOLATION_EPSILON, fOF_Sim)), 1.0f);

        const FfxFloat32 fFrameIndexFactor = FrameIndexSinceLastReset() < 10.0f;
        fGameMvBias = ffxLerp(fGameMvBias, 1.0f, fFrameIndexFactor);

        fInterpolatedColor = ffxLerp(ofColor, fInterpolatedColor, ffxSaturate(fGameMvBias));
    }
}

void computeFrameinterpolation(FfxUInt32x2 iPxPos)
{
    FfxFloat32x3 fColor            = FfxFloat32x3(0, 0, 0);
    FfxFloat32   fInPaintingWeight = 0.0f;

    if (FrameIndexSinceLastReset() > 0)
    {
        computeInterpolatedColor(iPxPos, fColor, fInPaintingWeight);
    }
    else
    {
        fColor = LoadCurrentBackbuffer(iPxPos);
    }

    StoreFrameinterpolationOutput(iPxPos, FfxFloat32x4(fColor, fInPaintingWeight));
}

#endif  // FFX_FRAMEINTERPOLATION_H
