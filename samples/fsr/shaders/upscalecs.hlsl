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


#include "upscalecommon.h"

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
RWTexture2D<float4> OutputTexture : register(u0);
Texture2D<float4> InputTexture : register(t0);
SamplerState Samplers[] : register(s0);

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

[numthreads(NUM_THREAD_X, NUM_THREAD_Y, 1)]
void MainCS(uint3 dtID : SV_DispatchThreadID)
{
    OutputTexture[dtID.xy].xyz = InputTexture.SampleLevel(
        Samplers[Type],
        (dtID.xy + 0.5f) * float2(invDisplayWidth, invDisplayHeight),
        0.0f
    ).xyz;
}


/**********************************************************************
MIT License

Copyright(c) 2019 MJP

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************/
float4 SampleHistoryCatmullRom(in float2 uv, in float2 texelSize)
{
    // Source: https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
    // License: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae

    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = uv / texelSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1.0f;
    float2 texPos3 = texPos1 + 2.0f;
    float2 texPos12 = texPos1 + offset12;

    texPos0 *= texelSize;
    texPos3 *= texelSize;
    texPos12 *= texelSize;

    float4 result = 0.f;

    result += InputTexture.SampleLevel(Samplers[0], float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += InputTexture.SampleLevel(Samplers[0], float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += InputTexture.SampleLevel(Samplers[0], float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += InputTexture.SampleLevel(Samplers[0], float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += InputTexture.SampleLevel(Samplers[0], float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += InputTexture.SampleLevel(Samplers[0], float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += InputTexture.SampleLevel(Samplers[0], float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += InputTexture.SampleLevel(Samplers[0], float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += InputTexture.SampleLevel(Samplers[0], float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    return max(result, 0.0f);
}

[numthreads(NUM_THREAD_X, NUM_THREAD_Y, 1)]
void BicubicMainCS(uint3 dtID : SV_DispatchThreadID)
{
    const float2 texelSize = float2(1.f, -1.f) * float2(invRenderWidth, invRenderHeight);
    const float2 uv = (dtID.xy + 0.5f) * float2(invDisplayWidth, invDisplayHeight);

    OutputTexture[dtID.xy].xyz = SampleHistoryCatmullRom(uv, texelSize).xyz;
}
