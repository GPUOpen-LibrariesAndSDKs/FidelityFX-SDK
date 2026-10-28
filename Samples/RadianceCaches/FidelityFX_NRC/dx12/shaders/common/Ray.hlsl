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

#ifndef SOURCE_RAY
#define SOURCE_RAY

#include "Transform.hlsl"
#include "Hash.hlsl"
#include "random/Random.hlsl"

// The minimum amount of data required to define an infinite ray in 3D space
#define RayBasicCtor { kZero, kZero }
struct RayBasic
{
    float3   o;
    float3   d;
};

// The full-fat ray objects that most methods refer to
#define RayCtor { RayBasicCtor, 0, kZero, 0u, 0, 0 }
struct Ray
{
    RayBasic od;       
    float    tNear;
    float3   weight;
    float3   compoundWeight;
    uint     flags;
    int      depth;
    float    pdf;
};

#define HitCtxCtor { -1, -1, kZero, asfloat2(0), 0, 0, kZero }
struct HitCtx
{
    int         matID;
    int         objID;
    float2      tRange;
    float3      n;
    float2      uv;
    float       alpha;
    float       eta;
    float       bias;
    float3      emission;
    float3      albedo;
};

#define kFlagsBackfacing      1u
#define kFlagsSubsurface      2u
#define kFlagsDirectSampleLight 4u
#define kFlagsDirectSampleBxDF 8u
#define kFlagsScattered       16u
#define kFlagsProbePath       32u
#define kFlagsCausticPath     64u
#define kFlagsVolumetricPath  128u
#define kFlagsInteracted      256u
#define kFlagsDirectSampleEnv 512u

//#define InheritFlags(ray) (ray.flags & kFlagsScattered)
#define InheritFlags(ray) (ray.flags & (kFlagsProbePath | kFlagsCausticPath | kFlagsInteracted))

#define IsBackfacing(ray) ((ray.flags & kFlagsBackfacing) != 0u)
#define IsSubsurface(ray) ((ray.flags & kFlagsSubsurface) != 0u)
#define IsScattered(ray) ((ray.flags & kFlagsScattered) != 0u)
#define IsNEESampleLight(ray) ((ray.flags & kFlagsDirectSampleLight) != 0u)
#define IsNEESampleBxDF(ray) ((ray.flags & kFlagsDirectSampleBxDF) != 0u)
#define IsNEESample(ray) ((ray.flags & (kFlagsDirectSampleLight | kFlagsDirectSampleBxDF)) != 0u)
#define IsProbePath(ray) ((ray.flags & kFlagsProbePath) != 0u)
#define IsCausticPath(ray) ((ray.flags & kFlagsCausticPath) != 0u)
#define IsVolumetricPath(ray) ((ray.flags & kFlagsVolumetricPath) != 0u)
#define HasInteracted(ray) ((ray.flags & kFlagsInteracted) != 0u)
#define IsNEESampleEnv(ray) ((ray.flags & kFlagsDirectSampleEnv) != 0u)


void SetRayFlag(inout Ray ray, in uint flag, in bool set)
{
    ray.flags &= ~flag;
    ray.flags |= flag * uint(set);
}

void CreateRay(inout Ray ray, float3 o, float3 d, float3 kickoff, float3 weight, int depth, uint flags)
{     
    ray.od.o = o + kickoff;
    ray.od.d = d;
    ray.tNear = kFltMax;
    ray.weight = weight;
    ray.compoundWeight = kOne;
    ray.flags = flags;
    ray.depth = depth;
}

RayBasic RayToObjectSpace(in RayBasic world, in Transform transform) 
{
    RayBasic object;
    object.o = world.o - transform.trans;
    object.d = world.d + object.o;
    object.o = mul(transform.rot, object.o / transform.sca);
    object.d = mul(transform.rot, object.d / transform.sca) - object.o;
    return object;
}

#define PointAt(ray) ( ray.od.o + ray.od.d * ray.tNear )
#define PointAtT(ray,  t) ( ray.o + ray.d * (t) )

#endif
