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

Texture2D<float>    r_dilated_depth : register(t0);
Texture2D<float2>   r_dilated_motion_vectors : register(t1);
Texture2D<uint>     r_reconstructed_prev_nearest_depth : register(t2);

RWTexture2D<float>  rw_fiDilatedDepth : register(u0);
RWTexture2D<float2> rw_fiDilated_motion_vectors : register(u1);
RWTexture2D<uint>   rw_fiReconstructedPrevNearestDepth : register(u2);

[numthreads(8, 8, 1)]
void CS(uint2 dtID : SV_DispatchThreadID)
{
    rw_fiDilatedDepth[dtID]                  = r_dilated_depth[dtID];
    rw_fiDilated_motion_vectors[dtID]        = r_dilated_motion_vectors[dtID];
    rw_fiReconstructedPrevNearestDepth[dtID] = r_reconstructed_prev_nearest_depth[dtID];
}
