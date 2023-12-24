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


#ifndef FFX_FRAMEINTERPOLATION_SETUP_H
#define FFX_FRAMEINTERPOLATION_SETUP_H

void setupFrameinterpolationResources(FfxInt32x2 iPxPos)
{
    // Update reset counters
    if (all(iPxPos == 0))
    {
        if(Reset() || HasSceneChanged()) {
            StoreCounter(FfxInt32x2(COUNTER_FRAME_INDEX_SINCE_LAST_RESET, 0), 0);
        } else {
            FfxUInt32 counter = RWLoadCounter(FfxInt32x2(COUNTER_FRAME_INDEX_SINCE_LAST_RESET, 0));
            StoreCounter(FfxInt32x2(COUNTER_FRAME_INDEX_SINCE_LAST_RESET, 0), counter + 1);
        }
    }

    // Reset resources
    StoreGameMotionVectorFieldX(iPxPos, 0);
    StoreGameMotionVectorFieldY(iPxPos, 0);

    StoreOpticalflowMotionVectorFieldX(iPxPos, 0);
    StoreOpticalflowMotionVectorFieldY(iPxPos, 0);

    StoreDisocclusionMask(iPxPos, 0);
}

#endif // FFX_FRAMEINTERPOLATION_SETUP_H
