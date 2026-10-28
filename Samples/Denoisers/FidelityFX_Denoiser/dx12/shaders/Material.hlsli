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
#include "Geometry.hlsli"
// rr::g_inv_pi
// rr::g_pi
#include "Numeric.hlsli"
// rr::F32
// rr::F32x3
// rr::U32
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    static const F32 MinPerceptualRoughness = 0.04f;

    static const F32 MinReflectance = 0.04f;

    struct Material
    {
        // The albedo         of the material for dielectrics.
        // The specular color of the material for metals.
        F32x3 baseColor;
        // The roughness of the material, as authored by the content author.
        F32 perceptualRoughness;

        // The specular reflectance of the material at normal incidence.
        F32x3 reflectance0;
        // The roughness of the material mapped to a more linear change in the
        // roughness (proposed by [2]).
        F32 alphaRoughness;

        // The specular reflectance of the material at grazing incidence.
        F32x3 reflectance90;
        // whether a material is a metal or a dielectric
        F32 metalness;
    };

    Material CreateMaterial(F32x3 baseColor, F32 roughness, F32 metalness)
    {
        roughness = saturate(roughness);

        Material material = (Material)0;

        material.perceptualRoughness = max(roughness, MinPerceptualRoughness);
        material.alphaRoughness      = material.perceptualRoughness * material.perceptualRoughness;
        material.metalness           = metalness;
        material.baseColor           = baseColor;
        material.reflectance0        = lerp(MinReflectance, baseColor, metalness);
        material.reflectance90       = 1.0f;

        return material;
    }

    struct AngularInfo
    {
        // The cosine of the angle between the surface normal and light direction.
        F32 NoL;
        // The cosine of the angle between the surface normal and view direction.
        F32 NoV;
        // The cosine of the angle between the surface normal and half vector.
        F32 NoH;
        // The cosine of the angle between the light direction and half vector.
        F32 LoH;

        F32 VoH; // The cosine of the angle between the view direction and half vector.
        F32x3 padding0;
    };

    AngularInfo ComputeAngularInfo(F32x3 l, F32x3 n, F32x3 v)
    {
        n = normalize(n); // Outward direction of surface point
        v = normalize(v); // Direction from surface point to view
        l = normalize(l); // Direction from surface point to light
        const F32x3 h = ComputeHalfVector(v, l);

        AngularInfo result;
        result.NoL = saturate(dot(n, l));
        result.NoV = saturate(dot(n, v));
        result.NoH = saturate(dot(n, h));
        result.LoH = saturate(dot(l, h));
        result.VoH = saturate(dot(v, h));
        result.padding0 = 0.0f;
        return result;
    }

    namespace brdf
    {
        // Lambert lighting
        // see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
        F32x3 Diffuse_Lambert(Material material)
        {
            return material.baseColor * (1.0f - material.metalness) * g_inv_pi;
        }

        // The following equation models the Fresnel reflectance term of the
        // specular equation (a.k.a. F())
        // Implementation of Fresnel from [3], Equation 15.
        F32x3 F_Schlick(Material material, AngularInfo angularInfo)
        {
            // F(v,h) = F0 + (F90 - F0) (1 - v.h)^5

            const F32 grazingFactor  = saturate(1.0f - angularInfo.VoH);
            const F32 grazingFactor2 = grazingFactor * grazingFactor;
            const F32 grazingFactor5 = grazingFactor2 * grazingFactor2 * grazingFactor;

            return material.reflectance0 + (material.reflectance90 - material.reflectance0) * grazingFactor5;
        }

        // Smith Joint GGX
        // Note: V = G / (4 * NdotL * NdotV)
        // see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
        // see Real-Time Rendering. Pages 331 to 336.
        // see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
        F32 V_SmithJointGGX(Material material, AngularInfo angularInfo)
        {
            const F32 NoL = angularInfo.NoL;
            const F32 NoV = angularInfo.NoV;
            const F32 a2  = material.alphaRoughness * material.alphaRoughness;

            const F32 GGXV = NoL * sqrt(NoV * NoV * (1.0f - a2) + a2);
            const F32 GGXL = NoV * sqrt(NoL * NoL * (1.0f - a2) + a2);
            const F32 GGX  = GGXV + GGXL;

            return (0.0f < GGX) ? 0.5f / GGX : 0.0f;
        }

        // The following equation(s) model the distribution of microfacet
        // normals across the area being drawn (a.k.a. D()).
        // Implementation from "Average Irregularity Representation of a
        // Roughened Surface for Ray Reflection" by T. S. Trowbridge, and
        // K. P. Reitz.
        // Follows the distribution function recommended in the SIGGRAPH 2013
        // course notes from EPIC Games [1], Equation 3.
        F32 D_TrowbridgeReitz(F32 alphaRoughness, F32 NoH)
        {
            const F32 a2 = alphaRoughness * alphaRoughness;
            const F32 f  = NoH * NoH * (a2 - 1.0f) + 1.0f;

            return a2 / (g_pi * f * f + 0.000001f);
        }

        F32 D_TrowbridgeReitz(Material material, AngularInfo angularInfo)
        {
            return D_TrowbridgeReitz(material.alphaRoughness, angularInfo.NoH);
        }
    }

    // BRDF contributions (diffuse & specular) scaled by NoL, before light.
    struct PathThroughput
    {
        F32x3 diffuse;
        F32x3 specular;
    };

    enum PathThroughputMode : U32
    {
        PathThroughputMode_Direct,
        PathThroughputMode_Indirect
    };

    template< PathThroughputMode Mode >
    PathThroughput ComputePathThroughput(Material material, F32x3 l, F32x3 n, F32x3 v)
    {
        const AngularInfo angularInfo = ComputeAngularInfo(l, n, v);

        PathThroughput result = (PathThroughput)0;
        if (0.0f < angularInfo.NoL)
        {
            //
            // Lo := (Kd + Ks) n.l
            //
            // n.l = geometric foreshortening
            //
            // Kd := (1 - F(v,h)) baseColor
            //                    ---------
            //                       pi
            //
            //       D(h) F(v,h) G(l,v,h)
            // Ks := -------------------- =  D(h) F(v,h) V(n,l,v,h)
            //          4 (n.l) (n.v)
            //

            F32 V = 0.0f;
            F32 D = 0.0f;
            if (0.0f < angularInfo.NoV)
            {
                V = brdf::V_SmithJointGGX(material, angularInfo);
                D = brdf::D_TrowbridgeReitz(material, angularInfo);
            }

            if (Mode == PathThroughputMode_Direct)
            {
                // Kd := (1 - F(v,h)) baseColor  1
                //                              --
                //                              pi
                //     ~-----1------~----2---~
                //
                // 1: (1 - F(v,h)) is         0 for conductors
                //                 is assumed 1 for dielectrics
                // 2: handled during composition
                //
                // Ks := D(h) F(v,h) V(n,l,v,h)
                //            ~-3-~
                //
                // 3: handled using BRDF LUT

                const F32 Kd = g_inv_pi;
                const F32 Ks = V * D; // specular lobe

                result.diffuse  = Kd * angularInfo.NoL;
                result.specular = Ks * angularInfo.NoL;
            }
            if (Mode == PathThroughputMode_Indirect)
            {
                const F32x3 F = brdf::F_Schlick(material, angularInfo);

                const F32x3 Kd = (1.0f - F) * brdf::Diffuse_Lambert(material);
                const F32x3 Ks = F * V * D;

                result.diffuse  = Kd * angularInfo.NoL;
                result.specular = Ks * angularInfo.NoL;
            }
        }

        return result;
    }
}
