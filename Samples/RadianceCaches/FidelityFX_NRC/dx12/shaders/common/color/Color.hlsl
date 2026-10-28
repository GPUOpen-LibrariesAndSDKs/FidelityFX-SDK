// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
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

#ifndef SOURCE_COLOR
#define SOURCE_COLOR

#include "Math.hlsl"

float3 Hue(float phi)
{
    float phiColour = 6.0 * phi;
    int i = int(phiColour);
    float3 c0 = float3(((i + 4) / 3) & 1, ((i + 2) / 3) & 1, ((i + 0) / 3) & 1);
    float3 c1 = float3(((i + 5) / 3) & 1, ((i + 3) / 3) & 1, ((i + 1) / 3) & 1);             
    return mix(c0, c1, phiColour - float(i));
}

float4 Blend(float4 rgba1, float3 rgb2, float w2)
{
    // Assume that RGB values are premultiplied so that when alpha-composited, they don't need to be renormalised
    return float4(mix(rgba1.xyz * rgba1.w, rgb2, w2) / max(1e-15, rgba1.w + (1. - rgba1.w) * w2),
                    rgba1.w + (1. - rgba1.w) * w2);
}

float4 Blend(float4 rgba1, float4 rgba2)
{               
    // Assume that RGB values are premultiplied so that when alpha-composited, they don't need to be renormalised
    return float4(mix(rgba1.xyz * rgba1.w, rgba2.xyz, rgba2.w) / max(1e-15, rgba1.w + (1. - rgba1.w) * rgba2.w),
                    rgba1.w + (1. - rgba1.w) * rgba2.w);
}

#endif
