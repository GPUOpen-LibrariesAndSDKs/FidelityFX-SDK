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

#ifndef SOURCE_TRACABLE_PRIMITIVE
#define SOURCE_TRACABLE_PRIMITIVE

#include "tracable/Intersector.hlsl"
 
// Ray-plane intersection test
bool TestPlanePrimitive(inout Ray ray, inout HitCtx hit, in Transform transform, bool isBounded, bool isDisc)
{
    RayBasic localRay = RayToObjectSpace(ray.od, transform);
    if(/*localRay.o.z <= 0. || */abs(localRay.d.z) < 1e-10) { return false; } 

    float t = localRay.o.z / -localRay.d.z;
    if (t <= 0.0 || t >= ray.tNear) { return false; }
    
    float u = (localRay.o.x + localRay.d.x * t);
    float v = (localRay.o.y + localRay.d.y * t);

    if(isDisc && u*u + v*v > 0.25) { return false; }

    u += 0.5; v += 0.5;    
    if(isBounded && (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0)) { return false; }   

    
    ray.tNear = t;    
    hit.n = mul(float3(0.0, 0.0, 1.0), transform.rot);
    SetRayFlag(ray, kFlagsBackfacing, localRay.o.z < 0.0);   
    hit.bias = 1e-4;
    hit.uv = float2(u, v);
    
    return true;
}

// Ray-sphere intersection test
bool TestSpherePrimitive(inout Ray ray, inout HitCtx hit, in Transform transform, bool testBackfacing)
{    
    RayBasic localRay = RayToObjectSpace(ray.od, transform);
    
    const float2 tNearFar = RaySphere(localRay, 1.);
    if (!Intersected(tNearFar)) { return false; }   

    float3 n;
    float tNear = ray.tNear;
    if(tNearFar.x > 0.0 && tNearFar.x < tNear) { tNear = tNearFar.x; }
    else if(tNearFar.y > 0.0 && tNearFar.y < tNear) { tNear = tNearFar.y; }
    else { return false; }
    
    if(!testBackfacing && dot(localRay.o, localRay.o) < 1.0) return false; // Ignore backfacing
    
    ray.tNear = tNear;
    SetRayFlag(ray, kFlagsBackfacing, dot(localRay.o, localRay.o) < 1.0);
    hit.n = mul(localRay.o + localRay.d * tNear, transform.rot);
    hit.bias = 1e-4;
    hit.uv = float2(0, 0);    
    return true;
}

// Ray-plane intersection test
bool TestMaskedBoxPrimitive(inout Ray ray, inout HitCtx hit, in Transform transform, bool hitBackfacing, int faceMask)
{
    RayBasic localRay = RayToObjectSpace(ray.od, transform);
    
    float tNear = ray.tNear;
    float2 uv;
    float3 n;
    for(int face = 0; face < 6; face += 1)
    {    
        if((faceMask & (1 << face)) != 0)
        {
            int dim = face / 2;
            float side = 2.0 * float(face % 2) - 1.0;

            if(abs(localRay.d[dim]) < 1e-10) { continue; }                

            float tFace = (0.5 * side - localRay.o[dim]) / localRay.d[dim];
            if (tFace <= 0.0 || tFace >= tNear) { continue; }

            int a = (dim + 1) % 3, b = (dim + 2) % 3;
            float2 uvFace = float2((localRay.o[a] + localRay.d[a] * tFace) + 0.5,
                                   (localRay.o[b] + localRay.d[b] * tFace) + 0.5);

            if(uvFace.x < 0.0 || uvFace.x > 1.0 || uvFace.y < 0.0 || uvFace.y > 1.0) { continue; }

            float3 nf = kZero;
            nf[dim] = side;

            if(!hitBackfacing && dot(nf, localRay.d) < 0.) continue;

            tNear = tFace;
            uv = uvFace + float2(float(face), 0.0);     
            n = nf;
        }
    }
    
    if(tNear == ray.tNear) { return false; }      

    SetRayFlag(ray, kFlagsBackfacing, dot(n, localRay.d) > 0.);

    ray.tNear = tNear;
    hit.n = mul(n, transform.rot) * transform.sca;
    hit.bias = 1e-4;
    hit.uv = uv;
    
    float3 hitLocal = PointAtT(localRay, tNear);
    int normPlane = maxDim(abs(hitLocal));    
    hit.objID = int(normPlane * 2) + int(step(0., hitLocal[normPlane]));

    return true;
}


bool TestConePrimitive(inout Ray ray, inout HitCtx hit, in Transform transform, float g, float h)
{
    RayBasic localRay = RayToObjectSpace(ray.od, transform);
    localRay.o.z += h;// * 0.6;
    //localRay.d.z *= g;
    
    float tPlane = (localRay.o.z - h) / -localRay.d.z;
    if(localRay.o.z > h)
    {
        if(localRay.d.z > 0.) return false;
        
        if(length(localRay.o.xy + localRay.d.xy * tPlane) < g)
        {
            if(tPlane > ray.tNear) return false;
            
            ray.tNear = tPlane;
            hit.bias = 1e-4;
            hit.n = mul(float3(0., 0., 1.), transform.rot);
            return true;
        }        
    }
    
    // A ray intersects a sphere in at most two places which means we can find t by solving a quadratic
    #define ConeDot(a, b) (a.x * b.x + a.y * b.y - (a.z * b.z) * sqr(g/h))
    float a = ConeDot(localRay.d, localRay.d);
    float b = 2.0 * ConeDot(localRay.d, localRay.o);
    float c = ConeDot(localRay.o, localRay.o);
    #undef ConeDot
    
    float t0, t1;
    if(!QuadraticSolve(a, b, c, t0, t1)) { return false; }
    
    sort(t0, t1);   
    
    float tCone = ray.tNear;
    if(t0 > 0.0 && t0 < tCone && localRay.o.z + localRay.d.z * t0 > 0.) { tCone = t0; }
    else if(t1 > 0.0 && t1 < tCone && localRay.o.z + localRay.d.z * t1 > 0.) { tCone = t1; }
    else { return false; }   
    
    if(tPlane > 0. && localRay.o.z < h && tPlane < tCone) return false;
    if(localRay.o.z + localRay.d.z * tCone > h) return false;
    
    float3 p = PointAtT(localRay, tCone);
    ray.tNear = tCone;
    hit.bias = 1e-4;
    hit.n = mul(float3(normalize(p.xy), 0.), transform.rot);
    
    return true;
}


#endif
