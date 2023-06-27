// AMD Cauldron code
// 
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#if __cplusplus
    #pragma once
    #include "misc/math.h"
#endif // __cplusplus

#if __cplusplus
struct SkydomeCBData
{
    Mat4 ClipToWorld;
};
#else 
// Fullscreen.hlsl binds to b0, so contants need to start at b1 if using fullscreen.hlsl
cbuffer SkydomeCBData : register(b1)
{
    matrix WorldClip;
}
#endif // __cplusplus

#if __cplusplus

struct ProceduralCBData
{
    Mat4 ClipToWorld;
    Vec3 SunDirection;

    float Rayleigh;
    float Turbidity;
    float MieCoefficient;
    float Luminance;

    float MieDirectionalG;
    float Padding[3];
};

#else
// Fullscreen.hlsl binds to b0, so contants need to start at b1 if using fullscreen.hlsl
cbuffer ProceduralCBData : register(b1)
{
    matrix ClipToWorld;
    float4 SunDirection;    // Vector3 are really Vector4s

    float Rayleigh;
    float Turbidity;
    float MieCoefficient;
    float Luminance;

    float MieDirectionalG;
    float3 Padding;
};
#endif // __cplusplus
