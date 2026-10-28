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

#ifndef SOURCE_INTEGRATOR_PT
#define SOURCE_INTEGRATOR_PT

#include "Ray.hlsl"
#include "random/Random.hlsl"
#include "bsdf/Simple.hlsl"
#include "bsdf/GGX.hlsl"
#include "light/Planar.hlsl"

#define IsValidHit(i) (i.x <= i.y)

#define PT_ENQUEUED_NOTHING 0
#define PT_ENQUEUED_DIRECT 1
#define PT_ENQUEUED_INDIRECT 2

#define PT_MODE_NAIVE 0
#define PT_MODE_NEE 1
#define PT_MODE_USE_MIS false

#define PT_INVALID_MATERIAL -1
#define PT_MAT_EMITTER 0
#define PT_MAT_ROUGH_SPECULAR 1
#define PT_MAT_ROUGH_DIELECTRIC 2
#define PT_MAT_PERFECT_SPECULAR 3
#define PT_MAT_PERFECT_DIELECTRIC 4
#define PT_MAT_LAMBERT 5

#define PT_MAX_PATH_DEPTH 5

struct RenderCtx
{
    RNGCtx rng;
    uint pixelIdx;
    float3 finalGatherWeight;
    uint renderMode;
    bool isTrainingPath;
    float time;
};

struct PTState
{
    uint genFlags;
    Ray directRay;
    Ray indirectRay;
    Transform emitterTrans;
    
    struct
    {
        // NOTE: Factorised radiance does not support incandescence whose colour is different from the surface albedo (e.g. black-body emitters)
        float3 LExtant[PT_MAX_PATH_DEPTH];      // The factored extant radiance at each bounce
        float3 weight[PT_MAX_PATH_DEPTH];       // The inter-bounce weight a.k.a. the product of the indirect weight and the albedo at the incident surface
    } 
    path;
};

PTState InitPTState(Ray ray)
{
    PTState pt;
    pt.indirectRay = ray;
    pt.genFlags = PT_ENQUEUED_INDIRECT; 

    // Reset the stack
    for(int i = 0; i < PT_MAX_PATH_DEPTH; ++i)
    {
        pt.path.LExtant[i] = 0.;
        pt.path.weight[i] = 1.;
    }

    return pt;
}

#define HasEnqueuedIndirect(pt) (pt.genFlags & PT_ENQUEUED_INDIRECT)
#define HasEnqueuedDirect(pt) (pt.genFlags & PT_ENQUEUED_DIRECT)
#define HasEnqueued(pt) (pt.genFlags != 0u)

bool GetNextRay(inout PTState pt, out Ray ray)
{
    if(pt.genFlags == PT_ENQUEUED_NOTHING) { return false; }

    if((pt.genFlags & PT_ENQUEUED_DIRECT) != 0) { ray = pt.directRay; return true; }
    ray = pt.indirectRay; return true;
}

void EscapeRay(in Ray ray, inout PTState pt, inout float3 L)
{
    if(ray.depth > 0 && !IsNEESample(ray))
    {
        float3 skydome = .001;
        L += ray.compoundWeight * skydome;
        pt.path.LExtant[ray.depth - 1] += skydome;
    }
    
    if(IsNEESample(ray)) { pt.genFlags &= ~PT_ENQUEUED_DIRECT; }
    else { pt.genFlags = 0u; }
}

float PowerHeuristic(float pdf1, float pdf2)
{
    return saturate(sqr(pdf1) / max(1e-10, sqr(pdf1) + sqr(pdf2)));
}

float SampleEmitter(in Ray incident, inout PTState pt, in HitCtx hit, in float2 xi/*, inout float3 L*/)
{
    float3 i = -incident.od.d;
    float emitterPdf = SampleQuadLight(incident, pt.directRay, hit, pt.emitterTrans, xi.xy, false);

    if(emitterPdf <= 0.) { return 0.; }                
        
    float bxdfPdf;
    switch(hit.matID) 
    {
        case PT_MAT_LAMBERT:
        {
            bxdfPdf = EvaluateLambertian(); 
            break;
        }
        case PT_MAT_ROUGH_SPECULAR:
        {
            bxdfPdf = EvaluateMicrofacetReflectorGGX(i, pt.directRay.od.d, hit.n, hit.alpha);
            break;
        }
        /*case PT_MAT_ROUGH_DIELECTRIC:
        {
            bxdfPdf = EvaluateMicrofacetDielectricGGX(i, extant.od.d, hit.n, hit.alpha);
            break;
        }*/
        default:
            /*L += kRed * 1e3; */return 0.; // Should never be here!
    }
    
    // Lambert cosine factor
    bxdfPdf *= dot(pt.directRay.od.d, hit.n);                    
     
    // Surface albedo x PDF
    pt.directRay.weight *= bxdfPdf;
    pt.directRay.pdf = bxdfPdf;
    
    if(PT_MODE_USE_MIS)
    {
        pt.directRay.weight *= 2. * PowerHeuristic(emitterPdf, bxdfPdf);
    }

    // Compound the extant weight with the incident weight
    pt.directRay.compoundWeight = incident.compoundWeight * incident.weight;

    return emitterPdf;
}

float SampleBxDF(in Ray incident, inout PTState pt, in HitCtx hit, in float3 xi, in bool isDirectSample/*, inout float3 L*/)
{
    float3 o;
    float bxdfPdf, brdfWeight = 1.;
    switch(hit.matID) 
    {    
        case PT_MAT_LAMBERT:
        {
            bxdfPdf = SampleLambertian(xi.xy, hit.n, o); 
            break;
        }
        case PT_MAT_PERFECT_SPECULAR:
        {
            bxdfPdf = SamplePerfectSpecular(-incident.od.d, hit.n, o);
            break;
        }
        case PT_MAT_PERFECT_DIELECTRIC:
        {
            float2 eta = IsBackfacing(incident) ? float2(hit.eta, 1.) : float2(1., hit.eta);
            bxdfPdf = SamplePerfectDielectric(xi.y, -incident.od.d, hit.n, eta, o, hit.bias);
            break;
        }
        case PT_MAT_ROUGH_SPECULAR:
        {
            bxdfPdf = SampleMicrofacetReflectorGGX(xi.xy, -incident.od.d, hit.n, hit.alpha, o, brdfWeight);
            break;
        }
        /*case PT_MAT_ROUGH_DIELECTRIC:
        {
            float2 eta = IsBackfacing(incident) ? float2(hit.eta, 1.) : float2(1., hit.eta);
            bxdfPdf = SampleMicrofacetDielectricGGX(xi, -incident.od.d, hit.n, hit.alpha, eta, o, brdfWeight, hit.bias);
            break;
        }*/
    }    
            
    if(bxdfPdf <= 0.) { return 0.; }

    Ray extant;
    CreateRay(extant, PointAt(incident), o, hit.n * hit.bias, kOne, incident.depth + 1, InheritFlags(incident)); 
    
    if(!(hit.matID == PT_MAT_PERFECT_SPECULAR || hit.matID == PT_MAT_PERFECT_DIELECTRIC)) { extant.flags |= kFlagsScattered; }
        
    if(isDirectSample) 
    { 
        // Re-weight the ray based on the BxDF and MIS heuristic
        // NOTE: Taking this branch implies MIS is enabled
        extant.pdf = EvaluateQuadLight(extant, hit, pt.emitterTrans, false);
        extant.weight *= 2. * PowerHeuristic(bxdfPdf, extant.pdf);
        extant.flags |= kFlagsDirectSampleBxDF;
        extant.compoundWeight = incident.compoundWeight * extant.weight;
        
        pt.directRay = extant; 
    } 
    else 
    { 
        // Store the weights of indirect rays in the path stack
        pt.path.weight[incident.depth] *= brdfWeight;
        extant.compoundWeight = incident.compoundWeight * brdfWeight;
        extant.pdf = bxdfPdf;
        pt.indirectRay = extant;     
    }
    return bxdfPdf;
}

int Shade(in Ray incident, inout PTState pt, in HitCtx hit, inout RenderCtx renderCtx/*, inout float3 L*/)
{             
    // Generate some random numbers
    float4 xi[2];
    Rand8(renderCtx.rng, uint((1 + incident.depth) * 5), xi);

    pt.path.weight[incident.depth] *= hit.albedo;
    incident.compoundWeight *= hit.albedo; 
    
    int genFlags = 0;
    if(renderCtx.renderMode == PT_MODE_NEE && !(hit.matID == PT_MAT_PERFECT_SPECULAR || hit.matID == PT_MAT_PERFECT_DIELECTRIC))
    {
        if(!PT_MODE_USE_MIS || xi[1].x < .5)
        {   
            if(SampleEmitter(incident, pt, hit, xi[0].xy) > 0.)
            {
                genFlags |= PT_ENQUEUED_DIRECT; 
            }
        }
        else
        {        
            if(SampleBxDF(incident, pt, hit, float3(xi[0].xy, xi[1].x), true) > 0.)
            {
                genFlags |= PT_ENQUEUED_DIRECT;
            }            
        }
    }

    #define kRRThreshold 0.
    //if(luminance(incident.weight) <= kRRThreshold && xi[1].x < 0.5) { return genFlags; }
    //incident.weight /= 1. - 0.5 * kRRThreshold;

    if(/*renderCtx.renderMode == PT_MODE_NAIVE && */SampleBxDF(incident, pt, hit, float3(xi[0].yzw), false /*, L*/) > 0.)
    {
        genFlags |= PT_ENQUEUED_INDIRECT;
    } 
    
    return genFlags;
} 

bool Integrate(inout Ray incidentRay, inout PTState pt, inout HitCtx hit, inout RenderCtx renderCtx, inout float3 L)
{   
    if ((pt.genFlags & PT_ENQUEUED_DIRECT) != 0)
    {
        pt.genFlags &= ~PT_ENQUEUED_DIRECT;
        if (hit.matID == PT_MAT_EMITTER && !IsBackfacing(incidentRay))
        {
            // If this sample is a light ray, all we need to know is whether or not it hit the light. 
            // If it did, just accumulate the weight which contains the radiant energy from the light sample.       
            float3 LEmitter = QuadLightRadiance(incidentRay, hit);
            L += incidentRay.compoundWeight * LEmitter;
            pt.path.LExtant[incidentRay.depth - 1] += LEmitter;
        }
    }
    else if ((pt.genFlags & PT_ENQUEUED_INDIRECT) != 0)
    {
        pt.genFlags &= ~PT_ENQUEUED_INDIRECT;
        if (hit.matID == PT_MAT_EMITTER)
        {
            // Emitters don't reflect light, so simply accumulate its radiance and we're done
            if(!IsBackfacing(incidentRay) && (renderCtx.renderMode == PT_MODE_NAIVE || IsNEESampleBxDF(incidentRay) || !IsScattered(incidentRay)))
            {
                float3 LEmitter = QuadLightRadiance(incidentRay, hit);                
                L += incidentRay.compoundWeight * LEmitter;
                if(incidentRay.depth > 1) { pt.path.LExtant[incidentRay.depth - 1] += LEmitter; }
            }
            pt.genFlags = 0;
        }
        else if(!IsNEESampleBxDF(incidentRay))
        {
            if(incidentRay.depth < PT_MAX_PATH_DEPTH - 1)
            {
                // Otherwise, shade indirect rays
                pt.genFlags = Shade(incidentRay, pt, hit, renderCtx/*, L*/);
            }   
        }
    }

    return ((pt.genFlags & PT_ENQUEUED_INDIRECT) != 0);
}

#endif
