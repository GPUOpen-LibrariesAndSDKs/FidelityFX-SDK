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

#ifndef SOURCE_BSDF_MICROFACET_BECKMANN
#define SOURCE_BSDF_MICROFACET_BECKMANN

#include "bsdf/BSDF.hlsli"

#define kBeckmannWeightClamp 50.

float D_Beckmann(float3 m, float3 n, float alpha)
{
    alpha *= alpha;
    float cosThetaM = clamp(dot(m, n), -1., 1.);
    float tan2ThetaM = 1. / sqr(cosThetaM) - 1.;
    return step(0.0f, dot(m, n)) / (kPi * alpha * pow4(cosThetaM)) * exp(-tan2ThetaM / alpha);
}

float G1_BeckmannExact(float3 v, float3 m, float3 n, float alpha)
{
    float cosThetaV = max(1e-15, dot(v, n));
    float tan2ThetaV = 1. / sqr(cosThetaV) - 1.;
    float a = 1. / (alpha * tan2ThetaV);
    return step(0.0f, dot(v, m) / cosThetaV) * 2. /
           (1.0f + ErfApprox(a) + 1. / (a * kRootPi) * exp(-a*a));
}

float G1_Beckmann (float3 v, float3 m, float3 n, float alpha)
{
    float cosThetaV = max(1e-15, dot(v, n));
    float tan2ThetaV = 1. / sqr(cosThetaV) - 1.;
    float a = 1. / (alpha * tan2ThetaV);
    float b = step(0.0f, dot(v, m) / cosThetaV);
    if(a > 1.6) { return b; }
    
    return b * a * (3.535 + a * 2.181) / (1. + a * (2.267 + a * 2.577));
}

float G_Beckmann(float3 i, float3 o, float3 m, float3 n, float alpha)
{
     return G1_Beckmann(i, m, n, alpha) * G1_Beckmann(o, m, n, alpha);
}

float Weight_Beckmann(float3 i, float3 o, float3 m, float3 n, float alpha)
{
    return min(kBeckmannWeightClamp, abs(dot(i, m)) * G_Beckmann(i, o, m, n, alpha) / (abs(dot(i, n) * dot(m, n))));
}

float EvaluateMicrofacetReflectorBeckmann(in float3 i, in float3 o, in float3 n, float alpha)
{
    #define kBeckmannPDFClamp 50.

    float3 hr = normalize(sign(dot(i, n)) * (i + o));
                
    float pdf = G_Beckmann(i, o, hr, n, alpha) * D_Beckmann(hr, n, alpha) / (4. * abs(dot(i, n) * dot(o, n)));
    return min(kBeckmannPDFClamp, pdf);
}
        
float SampleMicrofacetReflectorBeckmann(vec2 xi, float3 i, float3 n, float alpha, out float3 o, out float weight)
{        
    // Sample the microsurface normal with the Beckmann distribution
    float thetaM = atan(-alpha*alpha * log(1. - xi.x));
    float sinThetaM = sin(thetaM);
    float phiM = kTwoPi * xi.y;
    float3 m = CreateBasis(n) * float3(cos(phiM) * sinThetaM, sin(phiM) * sinThetaM, cos(thetaM));
    o = reflect(-i, m);       

    weight = Weight_Beckmann(i, o, m, n, alpha);
    
    return (dot(o, n) <= 0.) ? 0. : EvaluateMicrofacetReflectorBeckmann(i, o, m, alpha);    
}

#endif
