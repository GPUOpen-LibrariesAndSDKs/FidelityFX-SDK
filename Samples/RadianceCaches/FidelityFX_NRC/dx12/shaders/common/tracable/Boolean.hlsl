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

#ifndef SOURCE_TRACABLE_BOOLEAN
#define SOURCE_TRACABLE_BOOLEAN

#include "tracable/Intersector.hlsl"
 
#define CSG_SUBTRACT 0
#define CSG_ADD 1

#define PackCSGOpHit(op, hit) ((uint(op) & 1) | (uint(hit) << 1))
#define UnpackCSGOp(inter) int((inter.id) & 1u)
#define UnpackCSGHit(inter) ((inter.id) >> 1u)

#define kCsgMaxHits 6
#define kCsgMaxInts (kCsgMaxHits * 2)

struct CSGHit
{
    float3 n;
    float2 uv;
    uint id;
};

struct CSGInterval
{
    float t;
    uint id;
};

struct CSGCtx
{
    CSGHit hits[kCsgMaxHits];
    CSGInterval ints[kCsgMaxInts];
    int numHits;
    int numInts;
};

void InitCSGCtx(inout CSGCtx csg)
{
    csg.numHits = 0;
    csg.numInts = 0;
    csg.ints[0].id = csg.ints[1].id = 0;
}

#include "Ctor.hlsli"

bool PushCSGIntersection(inout CSGCtx csg, in float2 tRange, in float3 n, in float2 uv, in int primId, in int csgOp)
{
    if((tRange.x <= 0 && tRange.y <= 0) || csg.numHits >= kCsgMaxHits) { return false; }
    
    if(csg.numHits == 0)
    {   
        [unroll] for(int i = 0; i < 2; ++i)
        {
            CTOR2(CSGInterval, csg.ints[csg.numInts++], tRange[i], PackCSGOpHit((csgOp + i) & 1, 0));            
        }
        CTOR3(CSGHit, csg.hits[0], n, uv, primId);
        csg.numHits = 1;
    }
    else
    {            
        #define CSG_INSERT(op, element, aBound, bBound) \
            a = aBound; b = bBound; \
            while(b - a > 1) \
            { \
                int h = a + ((b - a) >> 1); \
                if(csg.ints[h].t > element) { b = h; } else { a = h; } \
            } \
            if(csg.ints[a].t < element) { ++a; } \
            for(int j = bBound; j > a; --j) { csg.ints[j] = csg.ints[j-1]; } \
            CTOR2(CSGInterval, csg.ints[a], element, PackCSGOpHit(op, csg.numHits)); \
            ++csg.numInts; 
        
        int a = 0, b = 0;
        CSG_INSERT(csgOp, tRange.x, 0, csg.numInts);
        CSG_INSERT((csgOp + 1) & 1, tRange.y, a+1, csg.numInts);

        #undef CSG_INSERT
      
        CTOR3(CSGHit, csg.hits[csg.numHits], n, uv, primId);
        ++csg.numHits;
    }
    return true;
}

int ResolveCSGIntersection(inout CSGCtx csg, inout Ray ray, inout HitCtx hit)
{
    int sum = 0;
    for(int i = 0; i < csg.numInts; ++i)
    {
        sum += UnpackCSGOp(csg.ints[i]) * 2 - 1;
        if(sum == 1 && csg.ints[i].t > 0)
        {
            if(csg.ints[i].t > ray.tNear) { return -1; }
            
            int hitIdx = clamp(UnpackCSGHit(csg.ints[i]), 0, kCsgMaxHits - 1);           
            const CSGHit csgHit = csg.hits[hitIdx];            
            ray.tNear = csg.ints[i].t;
            hit.n = csgHit.n;
            hit.uv = csgHit.uv; 
            hit.bias = 1e-3;
            hit.objID = csgHit.id;
            return csgHit.id;
        }
    }   
    return -1;
}

bool TestBoxCSG(inout CSGCtx csg, inout Ray ray, in Transform transform, in float3 size, in int primID, in int csgOp)
{
    const RayBasic localRay = RayToObjectSpace(ray.od, transform);
   
    float2 tNearFar = kMaxIntersectionRange;
    for(int dim = 0; dim < 3; dim++)
    {
        //if(abs(localRay.d[dim]) > 1e-10)
        {
            float t0 = (size[dim] * 0.5 - localRay.o[dim]) / localRay.d[dim];
            float t1 = (-size[dim] * 0.5 - localRay.o[dim]) / localRay.d[dim];
            if(t0 > t1) { tNearFar.x = max(tNearFar.x, t1); tNearFar.y = min(tNearFar.y, t0); }
            else  { tNearFar.x = max(tNearFar.x, t0); tNearFar.y = min(tNearFar.y, t1); }
        }
    }    
       
    if(!Intersected(tNearFar)) { return false; }  // Ray didn't hit the box       
    
    //float tHit = (csgOp == CSG_ADD) ? max(0., tNearFar.x) : max(0., tNearFar.y); 
    float tHit = (csgOp == CSG_ADD) ? tNearFar.x : tNearFar.y; 
    float3 hitLocal = localRay.o + localRay.d * tHit;

    int normPlane = maxDim(abs(hitLocal / size * 0.5));
    float3 n = kZero;
    n[normPlane] = sign(hitLocal[normPlane]);
    
    n *= (csgOp == CSG_SUBTRACT) ? -1. : 1.;    
    n = mul(n, transform.rot);//* transform.sca;
    float2 uv = float2(hitLocal[(normPlane + 1) % 2], hitLocal[(normPlane + 2) % 2]);
    int faceID = int(normPlane * 2) + int(step(0., hitLocal[normPlane]));
        
    return PushCSGIntersection(csg, tNearFar, n, uv, faceID, csgOp);    
}

// Ray-sphere intersection test
bool TestSphereCSG(inout CSGCtx csg, inout Ray ray, in Transform transform, in int csgOp)
{
    const RayBasic localRay = RayToObjectSpace(ray.od, transform);
    
    // A ray intersects a sphere in at most two places which means we can find t by solving a quadratic
    float a = dot(localRay.d, localRay.d);
    float b = 2.0 * dot(localRay.d, localRay.o);
    float c = dot(localRay.o, localRay.o) - 1.0;
    
    float t0, t1;
    if(!QuadraticSolve(a, b, c, t0, t1)) { return false; }
    
    sort(t0, t1);       
    
    float2 tNearFar = float2(t0, t1);
    float tHit = (csgOp == CSG_ADD) ? tNearFar.x : tNearFar.y;     
    float3 n = (localRay.o + localRay.d * tHit);    
    n = mul(n, transform.rot);//* transform.sca;    
    n *= (csgOp == CSG_SUBTRACT) ? -1. : 1.;       
    return PushCSGIntersection(csg, tNearFar, n, 0, 0, csgOp);    
}

#endif
