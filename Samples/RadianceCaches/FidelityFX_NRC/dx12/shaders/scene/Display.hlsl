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

Texture2D<float4> accumTexture : register(t0);
sampler linearSampler : register(s0);

#include "FrameCtx.hlsl"

/*cbuffer constants : register(b0)
{
    float kTime;
    int kFrameIdx;
    uint2 kViewportRes;
    uint kMaxTrainSamples;
    float kTrainingRatio;
    uint kRenderFlags;
    int kSplitScreenPartitionX;
    float kAccumMotionBlur;
}*/

#include "Math.hlsl"
 
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VSMain(float4 position : POSITION, float4 uv : TEXCOORD)
{
    PSInput result;
    result.position = position;
    result.uv = uv.xy;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    if(int(input.uv.x * kViewportRes.x) == kSplitScreenPartitionX)
    {
        return 1;
    }
    
    float4 texel = accumTexture.Load(int3(int2(input.uv * kViewportRes), 0));  
    float3 L = texel.xyz / max(1., texel.w);

    //L = atanh(L);
    #define kGamma 2.2
    #define kGain 3.
    L = pow(L, 1. / kGamma) * kGain;

    //L = mix(L, ApplyRedGrade(L), 0.7);

    return float4(L, 1.);
}
