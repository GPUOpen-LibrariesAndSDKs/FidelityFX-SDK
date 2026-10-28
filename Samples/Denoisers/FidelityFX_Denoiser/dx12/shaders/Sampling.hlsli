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

// rr::ComputeHalfVector
// rr::ComputeReflectionVector
#include "Geometry.hlsli"
// rr::brdf::D_TrowbridgeReitz
#include "Material.hlsli"
// rr::g_epsilon
// rr::g_inv_pi
// rr::g_pi
// rr::g_pi_div_2
// rr::g_pi_div_4
// rr::UNormToSNorm
#include "Numeric.hlsli"
// UniformUNorm
#include "Random.hlsli"
// CreateRotation3x3
#include "Transformation.hlsli"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x3x3
// rr::U32
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    F32x3 UniformSampleCone(inout U32 rngState, F32x3 direction, F32 cosHalfAngle)
    {
        const F32 z   = UniformUNorm(rngState) * (1.0f - cosHalfAngle) + cosHalfAngle;
        const F32 phi = UniformUNorm(rngState) * 2.0f * g_pi;

        const F32 x = sqrt(1.0f - z * z) * cos(phi);
        const F32 y = sqrt(1.0f - z * z) * sin(phi);
        const F32x3 north = F32x3(0.0f, 0.0f, 1.0f);

        const F32x3 axis = normalize(cross(direction, north)); // left-handed
        const F32 angle = acos(dot(north, direction));

        const F32x3x3 rotation = CreateRotation3x3(angle, axis);

        return mul(rotation, F32x3(x, y, z));
    }

    F32x2 UniformSampleConcentricDisk(inout U32 rngState)
    {
        const F32x2 u = F32x2(UniformUNorm(rngState), UniformUNorm(rngState));
        const F32x2 uOffset = UNormToSNorm(u);

        if (all(uOffset == 0))
        {
            return 0.0f;
        }

        F32 theta, r;
        if (abs(uOffset.x) > abs(uOffset.y))
        {
            r = uOffset.x;
            theta = g_pi_div_4 * (uOffset.y / uOffset.x);
        }
        else
        {
            r = uOffset.y;
            theta = g_pi_div_2 - g_pi_div_4 * (uOffset.x / uOffset.y);
        }
        return r * F32x2(cos(theta), sin(theta));
    }

    //--------------------------------------------------------------------------
    // Diffuse Reflections
    //--------------------------------------------------------------------------

    F32x3 SampleCosineWeightedHemisphere(inout U32 rngState, out F32 pdf)
    {
        F32x3 rLocal;
        rLocal.xy = UniformSampleConcentricDisk(rngState);
        rLocal.z  = sqrt(1.0f - saturate(dot(rLocal.xy, rLocal.xy)));
        pdf = rLocal.z * g_inv_pi;
        return rLocal;
    }

    F32x3 SampleDiffuseReflectionVector(inout U32 rngState, F32x3x3 localToWorld, out F32 pdf)
    {
        const F32x3 rLocal = SampleCosineWeightedHemisphere(rngState, pdf);

        return normalize(mul(rLocal, localToWorld));
    }

    //--------------------------------------------------------------------------
    // Specular Reflections
    //--------------------------------------------------------------------------

    F32x3 SampleVndfHemisphereBounded(inout U32 rngState, F32x3 vLocal, F32x2 alpha, out F32 pdf)
    {
        // [Source]
        // K. Eto, Y. Tokuyoshi: Bounded VNDF Sampling for Smith-GGX Reflections.

        const F32x3 vLocalStd = normalize(F32x3(vLocal.xy * alpha, vLocal.z));
        // Sample a spherical cap.
        const F32 phi = 2.0f * g_pi * UniformUNorm(rngState);
        const F32 a = saturate(min(alpha.x, alpha.y)); // Eq. 6
        const F32 s = 1.0f + length(vLocal.xy); // Omit sgn for a <=1
        const F32 a2 = a * a;
        const F32 s2 = s * s;
        const F32 k = (1.0f - a2) * s2 / (s2 + a2 * vLocal.z * vLocal.z); // Eq. 5
        const F32 b = (0.0f < vLocal.z) ? k * vLocalStd.z : vLocalStd.z;
        const F32 z = mad(1.0f - UniformUNorm(rngState), 1.0f + b, -b);
        const F32 sinTheta = sqrt(saturate(1.0f - z * z));
        const F32x3 rLocalStd = { sinTheta * cos(phi), sinTheta * sin(phi), z };
        // Compute the microfacet normal m.
        const F32x3 mLocalStd = vLocalStd + rLocalStd;
        const F32x3 mLocal = normalize(F32x3(mLocalStd.xy * alpha, mLocalStd.z));
        // Return the reflection vector r.
        const F32x3 rLocal = ComputeReflectionVector(vLocal, mLocal);

        const F32 ndf = brdf::D_TrowbridgeReitz(sqrt(a), mLocal.z);
        const F32x2 av = alpha * vLocal.xy;
        const F32 len2 = dot(av, av);
        const F32 t = sqrt(len2 + vLocal.z * vLocal.z);
        if (0.0f <= vLocal.z)
        {
            pdf = ndf / (2.0f * (k * vLocal.z + t)); // Eq . 8 * || dm / do ||
        }
        else
        {
            pdf = ndf * (t - vLocal.z) / (2.0f * len2); // Eq . 7 * || dm / do ||
        }

        return rLocal;
    }

    F32x3 SampleSpecularReflectionVector(inout U32 rngState, F32x3x3 localToWorld, F32x3 vWorld, F32 roughness, out F32 pdf)
    {
        const F32x3 nWorld = localToWorld[2];

        if (roughness < 0.1f)
        {
            // If roughness is extremely low, the surface is basically a
            // perfect mirror.
            pdf = 1.0f;
            return ComputeReflectionVector(vWorld, nWorld);
        }

        //const F32x3 vLocal = mul(vWorld, transpose(localToWorld));
        const F32x3 vLocal = mul(localToWorld, vWorld);
        const F32   alpha  = roughness * roughness;
        const F32x3 rLocal = SampleVndfHemisphereBounded(rngState, vLocal, alpha.xx, pdf);

        return normalize(mul(rLocal, localToWorld));
    }
}
