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

#ifndef SOURCE_TRIPLANAR
#define SOURCE_TRIPLANAR

#include "Math.hlsl"

/*
// Peturb the normal using the tangent space in the hit context
float3 PerturbNormal(inout HitCtx hit, sampler2D sampler, float alpha, float delta)
{
    alpha = sign(alpha) * sqr(alpha);
     
    float dpdu = luminance(texture(sampler, hit.uv + float2(-delta, 0.), 0.).xyz - texture(sampler, hit.uv + float2(delta, 0.), 0.).xyz) / (2. * delta);
    float dpdv = luminance(texture(sampler, hit.uv + float2(0., -delta), 0.).xyz - texture(sampler, hit.uv + float2(0., delta), 0.).xyz) / (2. * delta);

    return normalize(hit.n + (hit.tangent * dpdu + hit.cotangent * dpdv) * alpha);    
}

// Evaluates the texture using a triplanar mapping
float3 Triplanar(float3 p, float3 n, float scale, int type, sampler2D sampler1, sampler2D sampler2, int which)
{
    p *= scale;
    float3 rgbX = (which == 0) ? texture(sampler1, mod(abs(p.yz), float2(1.)), 0.).xyz : texture(sampler2, mod(abs(p.yz), float2(1.)), 0.).xyz;
    float3 rgbY = (which == 0) ? texture(sampler1, mod(abs(p.xz), float2(1.)), 0.).xyz : texture(sampler2, mod(abs(p.xz), float2(1.)), 0.).xyz;
    float3 rgbZ = (which == 0) ? texture(sampler1, mod(abs(p.xy), float2(1.)), 0.).xyz : texture(sampler2, mod(abs(p.xy), float2(1.)), 0.).xyz;
    
    float3 w = pow(abs(n), float3(2.));
    w /= sum(w);    
    
    if(type == 0)
    {
        return rgbX * w.x + rgbY * w.y + rgbZ * w.z;
    }
    else
    {    
        float L = sin01(kTwoPi * 2. * luminance(rgbX * w.x + rgbY * w.y + rgbZ * w.z));
        return kOne * L / (1. + exp(-8. * mix(-1., 1., L)));
    }
}

// Perturbs the surface normal using a triplanar texture
float3 PerturbNormalTriplanar(float3 p, inout HitCtx hit, float alpha, float delta, float scale, int type, sampler2D sampler1, sampler2D sampler2, int which)
{    
    alpha = sign(alpha) * sqr(alpha);
    delta /= scale;
 
    mat3 basis = CreateBasis(hit.n);
    float dpdu = luminance(Triplanar(p + basis[0] * -delta, hit.n, scale, type, sampler1, sampler2, which) - 
                           Triplanar(p + basis[0] * delta, hit.n, scale, type, sampler1, sampler2, which)) / (2. * delta);
    float dpdv = luminance(Triplanar(p + basis[1] * -delta, hit.n, scale, type, sampler1, sampler2, which) - 
                           Triplanar(p + basis[1] * delta, hit.n, scale, type, sampler1, sampler2, which)) / (2. * delta);

    return normalize(hit.n + (basis[0] * dpdu + basis[1] * dpdv) * alpha);    
}
*/

#endif
