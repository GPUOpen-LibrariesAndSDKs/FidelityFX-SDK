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

#define FfxFloat32x3 float3
#define ffxMax max

FfxFloat32x3 Tonemap(FfxFloat32x3 fRgb)
{
    return fRgb / (ffxMax(ffxMax(0.f, fRgb.r), ffxMax(fRgb.g, fRgb.b)) + 1.f).xxx;
}

#define FAR_DEPTH 0.5f

Texture2D<float4> r_currBB : register(t0);
Texture2D<float4> r_uiTexture : register(t1);

static const float4 FullScreenVertsPos[3] = { float4(-1, 1, FAR_DEPTH, 1), float4(3, 1, FAR_DEPTH, 1), float4(-1, -3, FAR_DEPTH, 1) };
// static const float2 FullScreenVertsUVs[3] = { float2(0, 0), float2(2, 0), float2(0, 2) };

//Texture2D        r_currBB       : register(t0);

struct VERTEX_OUT
{
//    float2 vTexture : TEXCOORD;
    float4 vPosition : SV_POSITION;
};

cbuffer cbComposition : register(b0)
{
    int uiPresent;
};

float4 mainPS(VERTEX_OUT Input) : SV_Target
{
    float3 color = r_currBB[Input.vPosition.xy].rgb;
    float4 guiColor = r_uiTexture[Input.vPosition.xy];

    //guiColor.rgb = Tonemap(guiColor.rgb);
    if (uiPresent == 1) {
        color = lerp(color, guiColor.rgb, pow(1.0f - guiColor.a, 3.0f));
    }
    
    return float4(color, 1);
}
