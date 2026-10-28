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

#ifndef SOURCE_HATCH
#define SOURCE_HATCH

#include "Math.hlsl"

float Hatch(float2 uvView, float brightness, float scale, float2 direction, float waveScale, float waveFrequency, float AA)
{    
    #define kCrossHashOrigin asfloat2(0.)
    
    float kCrossHashdPdXY = AA * (2. / (scale * kViewportRes.y));     
    brightness *= 1. + kCrossHashdPdXY;    
    
    float t = (dot(uvView, direction) - dot(direction, kCrossHashOrigin)) / dot(direction, direction);
    float f = length(direction * t - uvView + kCrossHashOrigin) / scale;
    if(waveScale > 0.)
    {
        f += waveScale * sin(sign(dot(uvView, float2(direction.y, -direction.x)))*t*waveFrequency/scale)/(1e-10 + waveFrequency);
    }
    return clamp((2. * abs(frac(f) - 0.5) - (1. - brightness)) / kCrossHashdPdXY, 0., 1.);
}

float Crosshatch(float2 uvView, float brightness, float scale, float waveScale, float waveFrequency, float AA)
{    
    return Hatch(uvView, clamp(2. * brightness, 0., 1.), scale, float2(-1, 1), waveScale, waveFrequency, AA) *
           Hatch(uvView, clamp(brightness, 0.5, 1.0), scale, float2(1, 1), waveScale, waveFrequency, AA);
}

#endif
