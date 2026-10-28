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

#ifndef SOURCE_BSDF
#define SOURCE_BSDF

#define kBxDFMaxPDF 1e6

float Fresnel(float cosI, float eta1, float eta2)
{
    float sinI = sqrt(1.0 - cosI * cosI);
    float beta = 1.0 - sqr(sinI * eta1 / eta2);
   
    if(beta < 0.0) { return 1.0; }
    
    float alpha = sqrt(beta);
    return (sqr((eta1 * cosI - eta2 * alpha) / (eta1 * cosI + eta2 * alpha)) +
            sqr((eta1 * alpha - eta2 * cosI) / (eta1 * alpha + eta2 * cosI))) * 0.5;
}

float3 SampleUnitSphere(float2 xi)
{
    xi.x = xi.x * 2.0 - 1.0;
    xi.y *= kTwoPi;

    float sinTheta = sqrt(1.0 - xi.x * xi.x);
    return float3(cos(xi.y) * sinTheta, xi.x, sin(xi.y) * sinTheta);
}

float2 SampleUnitDisc(float2 xi)
{
    float phi = xi.y * kTwoPi;   
    return float2(sin(phi), cos(phi)) * sqrt(xi.x);   
}

#endif
