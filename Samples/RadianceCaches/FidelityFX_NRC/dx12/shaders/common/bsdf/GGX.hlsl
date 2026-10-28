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

#ifndef SOURCE_BSDF_MICROFACET_GGX
#define SOURCE_BSDF_MICROFACET_GGX

#include "bsdf/BSDF.hlsli"

#define kGGXWeightClamp 1e3
#define kGGXPDFClamp 50.

float D_GGX(float3 m, float3 n, float alpha)
{
    alpha *= alpha;
    float cosThetaM = clamp(abs(dot(m, n)), -1., 1.);
    float tan2ThetaM = 1. / sqr(cosThetaM) - 1.;
    return alpha * step(0.0f, cosThetaM) / 
           (kPi * pow4(cosThetaM) * sqr(alpha + tan2ThetaM));
}

float G1_GGX(float3 v, float3 m, float3 n, float alpha)
{
    float cosThetaV = max(1e-10, abs(dot(v, n)));
    //float tan2ThetaV = sqr(tan(acos(cosThetaV)));
    float tan2ThetaV = 1. / sqr(cosThetaV) - 1.;
    return step(0.0f, abs(dot(v, m)) / cosThetaV) * 2. /
           (1.0f + sqrt(1.0 + sqr(alpha) * tan2ThetaV));
}

float G_GGX(float3 i, float3 o, float3 m, float3 n, float alpha)
{
     return G1_GGX(i, m, n, alpha) * G1_GGX(o, m, n, alpha);
}

float Weight_GGX(float3 i, float3 o, float3 m, float3 n, float alpha)
{
    return min(kGGXWeightClamp, abs(dot(i, m)) * G_GGX(i, o, m, n, alpha) / (abs(dot(i, n) * dot(m, n))));
}

float EvaluateMicrofacetReflectorGGX(in float3 i, in float3 o, in float3 n, float alpha)
{    
    float3 hr = normalize(sign(dot(i, n)) * (i + o));
            
    float pdf = G_GGX(i, o, hr, n, alpha) * D_GGX(hr, n, alpha) / (4. * abs(dot(i, n) * dot(o, n)));
    return min(kGGXPDFClamp, pdf);
}

float SampleMicrofacetReflectorGGX(float2 xi, float3 i, float3 n, float alpha, out float3 o, out float weight)
{        
    // Sample the microsurface normal with the GGX distribution
    float thetaM = atan(alpha * sqrt(xi.x) / sqrt(1.0 - xi.x));
    float phiM = kTwoPi * xi.y;
    float sinThetaM = sin(thetaM);
    float3 m = mul(CreateBasis(n), float3(cos(phiM) * sinThetaM, sin(phiM) * sinThetaM, cos(thetaM)));    
    o = reflect(-i, m);

    weight = Weight_GGX(i, o, m, n, alpha);
    
    return (dot(o, n) <= 0.) ? 0. : EvaluateMicrofacetReflectorGGX(i, o, m, alpha);    
}

float EvaluateMicrofacetDielectricGGX(in float3 i, in float3 o, in float3 n, float alpha, float2 eta, float F)
{
    #define kGGXPDFClamp 50.    
    
    float3 ht = -normalize(eta.x * i + eta.y * o);
            
    float pdf = abs(dot(i, ht)) * abs(dot(o, ht)) * sqr(eta.y) * (1. - F) * (G_GGX(i, o, ht, n, alpha) * D_GGX(ht, n, alpha)) / 
                (abs(dot(i, n) * dot(o, n)) * sqr(eta.x * dot(i, ht) + eta.y * dot(o, ht)));
    return min(kGGXPDFClamp, pdf);
}
        
float SampleMicrofacetDielectricGGX(float3 xi, float3 i, float3 n, float alpha, float2 eta, out float3 o, out float weight, out float kickoff)
{        
    // Sample the microsurface normal with the GGX distribution
    float thetaM = atan(alpha * sqrt(xi.x) / sqrt(1.0f - xi.x));
    float phiM = kTwoPi * xi.y;
    float sinThetaM = sin(thetaM);
    float3 m = mul(CreateBasis(n), float3(cos(phiM) * sinThetaM, sin(phiM) * sinThetaM, cos(thetaM)));
    
    // Calculate the Fresnel coefficient and associated vectors. 
    float F = Fresnel(dot(i, m), eta.x, eta.y);       
    if(xi.z > F)
    {
        o = refract(-i, m, eta.x / eta.y);
        kickoff *= -1.;
    }
    else
    {
       o = reflect(-i, m);
    }    

    weight = Weight_GGX(i, o, m, n, alpha);
    
    return EvaluateMicrofacetDielectricGGX(i, o, m, alpha, eta, F);    
}

#endif
