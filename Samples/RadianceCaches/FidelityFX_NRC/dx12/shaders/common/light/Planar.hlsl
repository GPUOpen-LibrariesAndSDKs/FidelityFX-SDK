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

#ifndef SOURCE_LIGHT_QUAD
#define SOURCE_LIGHT_QUAD

#include "color/Color.hlsl"
#include "FrameCtx.hlsl"

#define kQuadEmitterSca .1
#define kQuadEmitterPower 2.
#define kQuadEmitterRadiance (kOne * kQuadEmitterPower / sqr(kQuadEmitterSca))

float EvaluateSpotCone(float cosPhi)
{
    float phi = acos(cosPhi);

    #define kSpotConeInnerPhi 20.
    #define kSpotConeOuterPhi 30.
    #define kSpotConeFalloff 2.

    return pow(saturate((phi - toRad(kSpotConeOuterPhi)) / (toRad(kSpotConeInnerPhi) - toRad(kSpotConeOuterPhi))), kSpotConeFalloff);
}

float SampleQuadLight(in Ray incident, out Ray extant, inout HitCtx hit, in Transform emitterTrans, in float2 xi, in bool isDisc)
{
    // Sample a point on the light 
    float3 hitPos = PointAt(incident);

    //vec2 xi = vec2(0.0);
    //uint hash = HashOf(uint(gFragCoord.x), uint(gFragCoord.y));
    //vec2 xi = vec2(HaltonBase2(hash + uint(sampleIdx)), HaltonBase3(hash + uint(sampleIdx))) - 0.5;
    
    xi -= 0.5;
    if(isDisc && length2(xi.xy) > 0.25) { return 0.; }

    float3 lightPos = mul(emitterTrans.rot, float3(xi, 0.)) * emitterTrans.sca + emitterTrans.trans;    
    //lightPos = emitterTrans.trans;

    //if(dot(hitPos - emitterTrans.trans, emitterTrans.rot[2]) < 0) return 0.;

    // Compute the normalised extant direction based on the light position local to the shading point
    float3 outgoing = lightPos - hitPos;
    float lightDist = length(outgoing);
    outgoing /= lightDist;

    // Test if the emitter is behind the shading point
    if (dot(outgoing, hit.n) <= 0.) { return 0.0; }
      
    float cosPhi = dot(normalize(hitPos - lightPos), emitterTrans.rot[2]);
        
    // Test if the emitter is rotated away from the shading point
    if (cosPhi < 0.) { return 0.0; }

    // Compute the projected solid angle of the light        
    float solidAngle = cosPhi * sqr(emitterTrans.sca) / max(1e-10, sqr(lightDist));   
        
    // Create the ray from the sampled BRDF direction
    CreateRay(extant, hitPos,
                    outgoing, 
                    hit.n * hit.bias,
                    //hit.n * 1e-4,
                    solidAngle,
                    incident.depth + 1,
                    kFlagsDirectSampleLight);      
                    
    return 1.0 / max(1e-10, solidAngle);
}

float EvaluateQuadLight(inout Ray ray, inout HitCtx hit, in Transform emitterTrans, in bool isDisc) 
{    
    RayBasic localRay = RayToObjectSpace(ray.od, emitterTrans);
    if(abs(localRay.d.z) < 1e-10) { return 0.0; } 
    
    float t = localRay.o.z / -localRay.d.z;
    
    float2 uv = (localRay.o.xy + localRay.d.xy * t);
    //if(minf2(uv) < 0.0 || maxf2(uv) > 1.0) { return 0.0; } 
    //uv -= 0.5;
    if(isDisc && length2(uv) > 0.25) { return 0.; }
    
    float3 lightPos = PointAtT(ray.od, t);
    
    float cosPhi = dot(normalize(ray.od.o - lightPos), emitterTrans.rot[2]);
        
    // Test if the emitter is rotated away from the shading point
    if (cosPhi < 0.) { return 0.0; }
    
    float solidAngle = cosPhi * sqr(emitterTrans.sca) / max(1e-10, sqr(t));
    
    //if(!IsVolumetricBxDF(hit))
    {    
        float cosTheta = dot(hit.n, ray.od.d);
        if (cosTheta < 0.0f)  { return 0.0; }
        
        solidAngle *= cosTheta;   
    }
    
    //ray.weight *= kQuadEmitterRadiance * EvaluateSpotCone(cosPhi);
    ray.weight = kOne;
    ray.flags = kFlagsDirectSampleBxDF;
    return 1.0 / max(1e-10, solidAngle);
}

float3 QuadLightRadiance(in Ray ray, in HitCtx hit)
{
    //return ray.weight * kQuadEmitterRadiance;
    return ray.weight * kQuadEmitterRadiance * ((ray.depth == 0) ? 1. : EvaluateSpotCone(dot(-ray.od.d, hit.n)));
    //return ray.weight * kQuadEmitterRadiance * ((ray.depth == 0) ? 1. : EvaluateSpotCone(dot(-ray.od.d, hit.n)));
}

#endif
