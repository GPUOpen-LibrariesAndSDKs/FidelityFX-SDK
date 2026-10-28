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

#ifndef SOURCE_INTERSECTOR_KIFS
#define SOURCE_INTERSECTOR_KIFS

#include "tracable/Intersector.hlsl"

float4 SDFUnion(float4 a, float4 b) { return (abs(a.x) < abs(b.x)) ? a : b; }
float4 SDFExclusion(float4 a, float4 b) { return (a.x > -b.x) ? a : float4(-b.x, -b.yzw); }

float4 SDFSphere(float3 p, float radius)
{
    float lenP = length(p);
    return float4(lenP - radius, p * ((lenP > radius) ? 1. : -1.));
}

float4 SDFTorus(in float3 p, in float r1, in float r2)
{
    float3 pPlane = float3(p.x, 0.0, p.z);
    float pPlaneLen = length(pPlane);        
    float3 pRing = (pPlaneLen < 1e-10) ? kZero : (p - (pPlane * r1 / pPlaneLen));
   
    return float4(length(pRing) - r2, pRing);
}

float4 SDFCube(float3 p, float3 scale)
{
    float3 d = abs(p) - scale;
    float4 f = float4(min(maxf3(d),0.0) + length(max(d,0.0)), 0., 0., 0);
    int md = maxDim(d);
    f[1+md] = sign(p[md]);
    return f;
}

float4 SDFWireframeCube(float3 p, float3 scale, float t)
{
    return SDFExclusion( 
                SDFExclusion(
                       SDFExclusion(SDFCube(p, scale), 
                                    SDFCube(p, scale + float3(-t, -t, t))),
                SDFCube(p, scale + float3(t, -t, -t))),
           SDFCube(p, scale + float3(-t, t, -t)));
}


#define FoldTetrahedron(p, bi) \
{ \
    if(p.x + p.y < 0.0) \
    { \
        p.xy = -p.yx; \
        bi[0].xy = -bi[0].yx; bi[1].xy = -bi[1].yx; bi[2].xy = -bi[2].yx; \
        code |= 1u << uint(foldIdx * 3); \
    } \
    if(p.x + p.z < 0.0) \
    { \
        p.xz = -p.zx; \
        bi[0].xz = -bi[0].zx; bi[1].xz = -bi[1].zx; bi[2].xz = -bi[2].zx; \
        code |= 1u << uint(foldIdx * 3 + 1); \
     } \
     if(p.y + p.z < 0.0) \
     { \
        p.zy = -p.yz; \
        bi[0].zy = -bi[0].yz; bi[1].zy = -bi[1].yz; bi[2].zy = -bi[2].yz; \
        code |= 1u << uint(foldIdx * 3 + 2); \
     } \
}

#define FoldCube(p, Bi) \
{ \
    for(int d = 0; d < 3; d++) \
    { \
        if(p[d] < 0.0) \
        { \
            p[d] = -p[d]; \
            Bi[0][d] = -Bi[0][d]; Bi[1][d] = -Bi[1][d]; Bi[2][d] = -Bi[2][d]; \
            code |= 1u << uint(foldIdx * 3 + d); \
        } \
    } \
}

void FoldPlane(in float3 p, in float3 n, inout float3 q, inout float3x3 bi)
{
    if(dot(q - p, n) < 0.)
    {
        q += n * 2. * (dot(p, n) - dot(q, n));
        for(int i = 0; i < 3; ++i)
            bi[i] -= (2. * n) * dot(n, bi[i]);
    }       
}

void FoldSphere(inout float3 q, inout float3x3 bi)
{
    float r = length(q);
    //if(r < 1.0)
    {
        float3 n = q / r;
        q = mix(n * (2. - r), q, exp(-sqr(r)));
        if(length2(q) - r*r > 0.)
            for(int i = 0; i < 3; ++i)
                bi[i] += (2. * n) * dot(n, bi[i]);
    }
}

// Generic ray-SDF intersector
bool TestKIFSPrimitive(inout Ray ray, inout HitCtx hit, in Transform transform, RenderCtx renderCtx, out uint code)
{
    #define kSDFMaxIters 100
    #define kSDFCutoffThreshold 1e-3
    #define kSDFFailThreshold   1e2
    #define kSDFEscapeThreshold 3.
    #define kSDFNewtonStep 1.  
    #define kSDFIsosurface 0.0
    #define kKIFSNumFolds 3
        
    //if(length2(ray.od.o + ray.od.d * (dot(transform.trans, ray.od.d) - dot(ray.od.o, ray.od.d))) > sqr(kSDFEscapeThreshold * transform.sca)) return false;
    
    RayBasic localRay = RayToObjectSpace(ray.od, transform);
    float localMag = 1. / transform.sca;//length(localRay.d);
    localRay.d /= localMag;    
    
    float2 tNearFar = RayAABB(localRay, -kSDFEscapeThreshold / 1.44224957031, kSDFEscapeThreshold / 1.44224957031);
    if (!Intersected(tNearFar)) return false;
    //if(tNearFar.x > 0 && ray.tNear / localMag > tNearFar.x) return false;
    
    const float3x3 kifsTrans = (kRenderFlags & RENDER_ANIMATE_GEOMETRY) ? mul(RotXMat3(renderCtx.time * 0.3), RotYMat3(renderCtx.time * 0.3)) : float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
    
    int iterIdx;
    bool isSubsurface;
    float t = max(0., tNearFar.x);
    float3 p = localRay.o + t * localRay.d;       
    float3x3 Bi, B = CreateBasis(localRay.d);
    float sdfMin = kFltMax;
     
    float4 F;
    for(iterIdx = 0; iterIdx < kSDFMaxIters; ++iterIdx)
    {                         
        Bi = B;
        F = kFltMax;
        float foldScale = 1.;//pow(2., float(kKIFSNumFolds));
        
        float3 pi = p * foldScale;
        code = 0u;
        
        float3 n = float3(0., 0., 1.);
        for(int foldIdx = 0; foldIdx < kKIFSNumFolds; ++foldIdx)
        {
            //if(foldIdx < kKIFSNumFolds - 1)
            {                
                FoldTetrahedron(pi, Bi);
                //FoldCube(p, Bi);
                
                //n = kifsTrans * n.yzx;
                //FoldPlane(kZero, n, p, Bi);
                //p -= n * 0.5;
                pi *= 1.3;
                foldScale *= 1.3;
                
                //FoldSphere(p, Bi);  
                
                pi = mul(pi - 0.5, kifsTrans);
                Bi = mul(Bi, kifsTrans);
            }
            
            //if(foldIdx == kKIFSNumFolds - 1)
            {            
                //float4 Fi = SDFTorus(p, 1., 0.1);
                //float4 Fi = SDFSphere(p, 1.);  
                //float4 Fi = SDFCube(p, float3(1.));
                float4 Fi = SDFExclusion(SDFCube(pi, kOne), SDFCube(pi, float3(1.1, 0.9, 0.9)));
                Fi = SDFExclusion(Fi, SDFCube(pi, float3(0.9, 1.1, 0.9)));
                //float4 Fi = SDFExclusion(SDFCube(pi, kOne), SDFSphere(p - 0., 1.35));
                //float4 Fi = SDFWireframeCube(pi, float3(1., 1., 1.) * 1., 0.3); 

                //float4 Fi = SDFBox(p, float3(1., 1., 0.02), 1.);
                //Fi = SDFTorus(p, .5, 0.2);
                //Fi.x /= foldScale;
                Fi.x = (Fi.x - kSDFIsosurface) / foldScale;
                if(abs(Fi.x) < abs(F.x)) { F = Fi; }

                // Otherwise, check to see if we're at the surface
                if(F.x > 0.0 && F.x < kSDFCutoffThreshold) { break; }           
            }
        }  
        
        sdfMin = min(sdfMin, abs(F.x));      
        
        // On the first iteration, simply determine whether we're inside the isosurface or not
        if(iterIdx == 0) { isSubsurface = F.x < 0.0; }
        // Otherwise, check to see if we're at the surface
        else if(F.x > 0.0 && F.x < kSDFCutoffThreshold) { break; }        

        if(F.x > kSDFEscapeThreshold) { return false; }        
        t += isSubsurface ? -F.x : F.x;
        if(t / localMag > ray.tNear) { /*debug += kOne * sdfMin;*/ return false; }
        
        p = localRay.o + t * localRay.d;
    }  
      
    //debug += kOne * sdfMin;
        
    F.yzw = mul(Bi, F.yzw);
    F.yzw = normalize(B[0] * F.y + B[1] * F.z + B[2] * F.w);     
        
    // If the ray didn't find the isosurface before running out of iterations, discard it
    if(F.x > kSDFFailThreshold) {  return false; }    
    ray.tNear = t / localMag;
    hit.n = normalize(mul(F.yzw, transform.rot));
    hit.bias = 1e-3;
    SetRayFlag(ray, kFlagsBackfacing, isSubsurface);    
    
    return true;
}

#endif 
