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

#pragma once

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

// rr::Orthonormal
#include "Basis.hlsli"
// rr::LightType
// rr::PackedLight
#include "PackedData.h"
// rr::g_rayEpsilon
// rr::Ray
// rr::TraceHitDistanceRay
// rr::TraceShadowRay
#include "PathTracing.hlsli"
// rr::UniformSampleCone
#include "Sampling.hlsli"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x3x3
// rr::U32
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

// Normal convention (world space):
//   nWorld  - shading normal (interpolated vertex normal perturbed by the normal map).
//             Used for the BRDF, the light-facing gate dot(n, l), and the G-buffer normal.
//   ngWorld - geometric face normal (perpendicular to the triangle).
//             Used ONLY for ray-origin offsets, to avoid self-intersection with
//             the originating surface.

namespace rr
{
    F32 EvaluateHitDistance(RaytracingAccelerationStructure rtas, PackedLight light,
                            inout U32 rngState, F32x3 pWorld, F32x3 nWorld, F32x3 ngWorld)
    {
        const F32x3 oWorld = pWorld + ngWorld * 10 * g_rayEpsilon;
        F32 hitDistance = 0.0f;
        static const F32 lightSourceRadius = 0.1f;
        switch (light.type)
        {

        case LightType_Directional:
        {
            const F32x3 lWorld = -light.dWorld;
            if (0.0f < dot(nWorld, lWorld))
            {
                const float cosHalfAngle = light.cosAngularRadius;

                const F32x3 dWorld = UniformSampleCone(rngState, lWorld, cosHalfAngle);

                Ray hitDistanceRay;
                hitDistanceRay.oWorld = oWorld;
                hitDistanceRay.dWorld = dWorld;
                hitDistanceRay.tMin   = 0.0f;
                hitDistanceRay.tMax   = 1024.0f;

                hitDistance = TraceHitDistanceRay(rtas, hitDistanceRay);
            }
            break;
        }
        case LightType_Spot:
        case LightType_Point:
        default:
        {
            F32x3 lWorld = light.pWorld - pWorld;
            const F32 distance = length(lWorld);
            lWorld = normalize(lWorld);
            if (0.0f < dot(nWorld, lWorld))
            {
                const F32x3 perpL = Orthonormal(lWorld);
                const F32x3 leWorld = normalize((light.pWorld + perpL * lightSourceRadius) - pWorld);
                const F32 cosHalfAngle = saturate(dot(lWorld, leWorld));

                const F32x3 dWorld = UniformSampleCone(rngState, lWorld, cosHalfAngle);

                Ray hitDistanceRay;
                hitDistanceRay.oWorld = oWorld;
                hitDistanceRay.dWorld = dWorld;
                hitDistanceRay.tMin   = 0.0f;
                hitDistanceRay.tMax   = distance;

                hitDistance = TraceHitDistanceRay(rtas, hitDistanceRay);
            }
            break;
        }
        };

        return hitDistance;
    }

    F32 EvaluateHardShadowFactor(RaytracingAccelerationStructure rtas, PackedLight light,
                                 inout U32 rngState, F32x3 pWorld, F32x3 ngWorld)
    {
        Ray shadowRay;
        shadowRay.oWorld = pWorld + ngWorld * g_rayEpsilon;
        shadowRay.tMin   = 0.0f;
        shadowRay.tMax   = 1024.0f;

        switch (light.type)
        {

        case LightType_Directional:
        {
            const F32x3 lWorld = -light.dWorld;

            shadowRay.dWorld = lWorld;
            break;
        }
        case LightType_Spot:
        case LightType_Point:
        default:
        {
            F32x3 lWorld = light.pWorld - pWorld;
            const F32 distance = length(lWorld);
            lWorld = normalize(lWorld);

            shadowRay.dWorld = lWorld;
            shadowRay.tMax   = distance;
            break;
        }
        };

        const U32 hitCount = TraceShadowRay(rtas, shadowRay);

        return 1.0f - hitCount;
    }

    F32 EvaluateSoftShadowFactor(RaytracingAccelerationStructure rtas, PackedLight light,
                                 inout U32 rngState, F32x3 pWorld, F32x3 nWorld, F32x3 ngWorld)
    {
        const F32x3 oWorld = pWorld + ngWorld * 10 * g_rayEpsilon;

        static const F32 lightSourceRadius = 0.1f;
        F32 shadowFactor = 0.0f;
        switch (light.type)
        {

        case LightType_Directional:
        {
            const F32x3 lWorld = -light.dWorld;
            if (0.0f < dot(nWorld, lWorld))
            {
                const float cosHalfAngle = light.cosAngularRadius;

                const F32x3 dWorld = UniformSampleCone(rngState, lWorld, cosHalfAngle);

                Ray shadowRay;
                shadowRay.oWorld = oWorld;
                shadowRay.dWorld = dWorld;
                shadowRay.tMin   = 0.0f;
                shadowRay.tMax   = 1024.0f;

                shadowFactor = 1.0f - TraceShadowRay(rtas, shadowRay);
            }
            break;
        }
        case LightType_Spot:
        case LightType_Point:
        default:
        {
            F32x3 lWorld = light.pWorld - pWorld;
            const F32 distance = length(lWorld);
            lWorld = normalize(lWorld);
            if (0.0f < dot(nWorld, lWorld))
            {
                const F32x3 perpL = Orthonormal(lWorld);
                const F32x3 leWorld = normalize((light.pWorld + perpL * lightSourceRadius) - pWorld);
                const F32 cosHalfAngle = saturate(dot(lWorld, leWorld));

                const F32x3 dWorld = UniformSampleCone(rngState, lWorld, cosHalfAngle);

                Ray shadowRay;
                shadowRay.oWorld = oWorld;
                shadowRay.dWorld = dWorld;
                shadowRay.tMin   = 0.0f;
                shadowRay.tMax   = distance;

                shadowFactor = 1.0f - TraceShadowRay(rtas, shadowRay);
            }
            break;
        }
        };

        return shadowFactor;
    }
}
