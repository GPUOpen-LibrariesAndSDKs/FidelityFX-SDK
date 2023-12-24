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


#ifndef FFX_FRAMEINTERPOLATION_GAME_MOTION_VECTOR_FIELD_H
#define FFX_FRAMEINTERPOLATION_GAME_MOTION_VECTOR_FIELD_H

FfxUInt32 getPriorityFactorFromViewSpaceDepth(FfxFloat32 fViewSpaceDepthInMeters)
{
    fViewSpaceDepthInMeters = ffxPow(fViewSpaceDepthInMeters, 0.33f);

    FfxUInt32 uPriorityFactor = FfxFloat32(1 - (fViewSpaceDepthInMeters * (1.0f / (1.0f + fViewSpaceDepthInMeters)))) * PRIORITY_HIGH_MAX;

    return ffxMax(1, uPriorityFactor);
}

void computeGameFieldMvs(FfxInt32x2 iPxPos)
{
    const FfxFloat32x2 fUv = FfxFloat32x2(FfxFloat32x2(iPxPos) + 0.5f) / RenderSize();
    const FfxFloat32 fDepthSample = LoadDilatedDepth(iPxPos);
    const FfxFloat32x2 fGameMotionVector = LoadDilatedMotionVector(iPxPos);
    const FfxFloat32x2 fMotionVectorHalf = fGameMotionVector * 0.5f;
    const FfxFloat32x2 fInterpolatedLocationUv = fUv + fMotionVectorHalf;

    const FfxFloat32 fViewSpaceDepth = ConvertFromDeviceDepthToViewSpace(fDepthSample);
    const FfxUInt32 uHighPriorityFactorPrimary = getPriorityFactorFromViewSpaceDepth(fViewSpaceDepth);

    FfxFloat32x3 prevBackbufferCol = SamplePreviousBackbuffer(fUv).xyz;
    FfxFloat32x3 curBackbufferCol  = SamplePreviousBackbuffer(fUv + fGameMotionVector).xyz;
    FfxFloat32   prevLuma          = 0.001f + RawRGBToLuminance(prevBackbufferCol);
    FfxFloat32   currLuma          = 0.001f + RawRGBToLuminance(curBackbufferCol);

    FfxUInt32 uLowPriorityFactor = round(ffxPow(MinDividedByMax(prevLuma, currLuma), 1.0f / 1.0f) * PRIORITY_LOW_MAX)
    * IsUvInside(fUv + fGameMotionVector);

    // Update primary motion vectors
    {
        const FfxUInt32x2 packedVectorPrimary = PackVectorFieldEntries(true, uHighPriorityFactorPrimary, uLowPriorityFactor, fMotionVectorHalf);

        BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fInterpolatedLocationUv, RenderSize());
        for (FfxInt32 iSampleIndex = 0; iSampleIndex < 4; iSampleIndex++)
        {
            const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
            const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

            if (IsOnScreen(iSamplePos, RenderSize()))
            {
                UpdateGameMotionVectorField(iSamplePos, packedVectorPrimary);
            }
        }
    }

    // Update secondary vectors
    // Main purpose of secondary vectors is to improve quality of inpainted vectors
    const FfxBoolean bWriteSecondaryVectors = length(fMotionVectorHalf * RenderSize()) > FFX_FRAMEINTERPOLATION_EPSILON;
    if (bWriteSecondaryVectors)
    {
        FfxBoolean bWriteSecondary = true;
        FfxUInt32 uNumPrimaryHits = 0;
        const FfxFloat32 fSecondaryStepScale = length(1.0f / RenderSize());
        const FfxFloat32x2 fStepMv = normalize(fGameMotionVector);
        const FfxFloat32 fBreakDist = length(fMotionVectorHalf);

        for (FfxFloat32 fMvScale = fSecondaryStepScale; fMvScale <= fBreakDist && bWriteSecondary; fMvScale += fSecondaryStepScale)
        {
            const FfxFloat32x2 fSecondaryLocationUv = fInterpolatedLocationUv - fStepMv * fMvScale;
            BilinearSamplingData bilinearInfo = GetBilinearSamplingData(fSecondaryLocationUv, RenderSize());

            // Reverse depth prio for secondary vectors
            FfxUInt32 uHighPriorityFactorSecondary = ffxMax(1, PRIORITY_HIGH_MAX - uHighPriorityFactorPrimary);

            const FfxFloat32x2 fToCenter = normalize(FfxFloat32x2(0.5f, 0.5f) - fSecondaryLocationUv);
            uLowPriorityFactor = ffxMax(0.0f, dot(fToCenter, fStepMv)) * PRIORITY_LOW_MAX;
            const FfxUInt32x2 packedVectorSecondary = PackVectorFieldEntries(false, uHighPriorityFactorSecondary, uLowPriorityFactor, fMotionVectorHalf);

            // Only write secondary mvs to single bilinear location
            for (FfxInt32 iSampleIndex = 0; iSampleIndex < 1; iSampleIndex++)
            {
                const FfxInt32x2 iOffset = bilinearInfo.iOffsets[iSampleIndex];
                const FfxInt32x2 iSamplePos = bilinearInfo.iBasePos + iOffset;

                bWriteSecondary &= IsOnScreen(iSamplePos, RenderSize());

                if (bWriteSecondary)
                {
                    const FfxUInt32 uExistingVectorFieldEntry = UpdateGameMotionVectorFieldEx(iSamplePos, packedVectorSecondary);

                    uNumPrimaryHits += PackedVectorFieldEntryIsPrimary(uExistingVectorFieldEntry);
                    bWriteSecondary &= (uNumPrimaryHits <= 3);
                }
            }
        }
    }
}

#endif // FFX_FRAMEINTERPOLATION_GAME_MOTION_VECTOR_FIELD_H
