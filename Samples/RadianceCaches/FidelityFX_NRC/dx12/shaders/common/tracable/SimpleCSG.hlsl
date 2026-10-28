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

#ifndef SOURCE_TRACABLE_SIMPLE_CSG
#define SOURCE_TRACABLE_SIMPLE_CSG

#include "tracable/Intersector.hlsl"
 
#define CSG_SUBTRACT 0
#define CSG_ADD 1

#define SetCSGData(op, data) ((op & 1) | (data << 1))
#define GetCSGOp(hit) ((hit.bits) & 1)
#define GetCSGData(hit) ((hit.bits) >> 1)

struct CSGHit
{
    float2 t;
    float3 n;
    float2 uv;
    uint bits;
};

/*int ResolveCSGIntersection(inout Ray ray, inout HitCtx hit, inout CSGHit pair[2])
{       
    int4 k = int4(0, 1, 2, 3);
    #define CSGInter(i) pair[k[i] >> 1].t[k[i] & 1]
    #define CSGSort(i, j) if(CSGInter(i) > CSGInter(j)) { swap(i, j); }
    
    CSGSort(0, 2); CSGSort(1, 3); CSGSort(0, 1); CSGSort(2, 3); CSGSort(1, 2);
    
    int sum = 0;
    float t;
    int i, j;
    for(i = 0; i < 4; ++i)
    {
        j = k[i];
        const int op = GetCSGOp(pair[j >> 1]);
        sum += ((op + (j&1)) & 1) * 2 - 1;
        if(sum == 1)
        {
            const float t = CSGInter(j);
            if(t > 0 && t < ray.tNear) { continue; }
        }
    }   
    if(i == 4) { return -1; }
    
    j >>= 1;
    ray.tNear = t;
    hit.n = pair[j];
    hit.uv = pair[j]; 
    hit.bias = 1e-3;
    hit.objID = GetCSGData(pair[j]);
    return j;
    
    #undef CSGInter
    #undef CSGSort
}*/

bool TestBoxCSG(inout Ray ray, in Transform transform, in float3 size, in int csgOp, in int csgIdx, inout CSGHit pair[2])
{
    const RayBasic localRay = RayToObjectSpace(ray.od, transform);

    CSGHit hit;   
    hit.t = kMaxIntersectionRange;
    for(int dim = 0; dim < 3; dim++)
    {
        //if(abs(localRay.d[dim]) > 1e-10)
        {
            float t0 = (size[dim] * 0.5 - localRay.o[dim]) / localRay.d[dim];
            float t1 = (-size[dim] * 0.5 - localRay.o[dim]) / localRay.d[dim];
            if(t0 > t1) { hit.t.x = max(hit.t.x, t1); hit.t.y = min(hit.t.y, t0); }
            else  { hit.t.x = max(hit.t.x, t0); hit.t.y = min(hit.t.y, t1); }
        }
    }    
       
    if(!Intersected(hit.t)) { pair[csgIdx].t = -kFltMax; return false; }  // Ray didn't hit the box       
    
    float3 hitLocal = localRay.o + localRay.d * hit.t[~csgOp & 1];
    int normPlane = maxDim(abs(hitLocal / size * 0.5));
    hit.n = kZero;
    hit.n[normPlane] = sign(hitLocal[normPlane]);
    
    hit.n *= csgOp * 2 - 1;;    
    hit.n = mul(hit.n, transform.rot);//* transform.sca;
    hit.uv = float2(hitLocal[(normPlane + 1) % 2], hitLocal[(normPlane + 2) % 2]);
    const int faceID = int(normPlane * 2) + int(step(0., hitLocal[normPlane]));    
    hit.bits = SetCSGData(csgOp, faceID);    
    pair[csgIdx] = hit;
    
    return true;
}

// Ray-sphere intersection test
bool TestSphereCSG(inout Ray ray, in Transform transform, in int csgOp, in int csgIdx, inout CSGHit pair[2])
{
    const RayBasic localRay = RayToObjectSpace(ray.od, transform);

    // A ray intersects a sphere in at most two places which means we can find t by solving a quadratic
    float a = dot(localRay.d, localRay.d);
    float b = 2.0 * dot(localRay.d, localRay.o);
    float c = dot(localRay.o, localRay.o) - 1.0;
    
    float t0, t1;
    if(!QuadraticSolve(a, b, c, t0, t1)) { pair[csgIdx].t = -kFltMax;  }
    
    sort(t0, t1);       
    
    CSGHit hit;   
    hit.t = float2(t0, t1);
    hit.n = localRay.o + localRay.d * hit.t[~csgOp & 1];    
    hit.n = mul(hit.n, transform.rot);//* transform.sca;    
    hit.n *= csgOp * 2 - 1;
    hit.uv = 0;
    hit.bits = SetCSGData(csgOp, 0);
    
    return true;  
}

#endif
