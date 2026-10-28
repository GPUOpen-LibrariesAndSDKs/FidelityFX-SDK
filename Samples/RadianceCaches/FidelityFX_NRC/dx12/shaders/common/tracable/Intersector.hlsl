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

#ifndef SOURCE_TRACABLE_INTERSECTOR
#define SOURCE_TRACABLE_INTERSECTOR
 
#include "Transform.hlsl"

#define kInvalidIntersectionRange float2(kFltMax, -kFltMax);
#define kMaxIntersectionRange float2(-kFltMax, kFltMax);
#define kInvalidIntersectionPoint kFltMax

bool Intersected(float2 i) { return i.x < i.y && (i.x > 0 || i.y > 0); }
bool Intersected(float i) { return abs(i) != kInvalidIntersectionPoint; }

float2 RayAABB(in RayBasic localRay, in float3 lowerBound, float3 upperBound)
{   
    float2 tNearFar = float2(-kFltMax, kFltMax);
    for(int dim = 0; dim < 3; dim++)
    {
        if(abs(localRay.d[dim]) > 1e-20)
        {
            float t0 = (upperBound[dim] - localRay.o[dim]) / localRay.d[dim];
            float t1 = (lowerBound[dim] - localRay.o[dim]) / localRay.d[dim];
            if(t0 < t1) { tNearFar.x = max(tNearFar.x, t0);  tNearFar.y = min(tNearFar.y, t1); }
            else { tNearFar.x = max(tNearFar.x, t1);  tNearFar.y = min(tNearFar.y, t0); }
        }
    }     
    return tNearFar;
}

float2 RaySphere(in RayBasic localRay, float radius)
{    
    float a = dot(localRay.d, localRay.d);
    float b = 2.0 * dot(localRay.d, localRay.o);
    float c = dot(localRay.o, localRay.o) - radius*radius;    
    float b2ac4 = b * b - 4.0 * a * c;
    if (b2ac4 < 0.0)
    {
        return kInvalidIntersectionRange;
    }
    else
    {
        b2ac4 = sqrt(b2ac4);
        return sorted(float2((-b + b2ac4) / (2.0 * a), (-b - b2ac4) / (2.0 * a)));
    }
}

float RayAAPlane(in RayBasic localRay, float3 origin, int axis)
{
    return (abs(localRay.d[axis]) < 1e-15) ? kInvalidIntersectionPoint : ((localRay.o[axis] - origin[axis]) / -localRay.d[axis]);
}

#endif
