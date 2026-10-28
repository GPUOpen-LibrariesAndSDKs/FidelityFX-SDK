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

// rr::ComputePathThroughput
// rr::Material
// rr::PathThroughput
// rr::PathThroughputMode
#include "Material.hlsli"
// rr::LightType
// rr::PackedLight
#include "PackedData.h"
// rr::F32
// rr::F32x3
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    struct LightThroughput
    {
        F32x3 diffuseRadiance;
        F32x3 specularRadiance;
    };

    // Maximum light distance in world-space meters. The smaller this is, the less pixels we shade.
    static const F32 g_max_light_distance = 10.0f;

    // https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
    F32 GetDistanceAttenuation(PackedLight light, F32 distance)
    {
        // Attenuate according to the inverse square law and smooth transition near the max light range to prevent hard cutoff.
        const F32 f = saturate(distance / light.maxDistance);
        const F32 f2 = f * f;
        const F32 f4 = f2 * f2;

        return (1.0f - f4) / (distance * distance);
    }

    // https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
    F32 GetAngularAttenuation(PackedLight light, F32x3 lWorld)
    {
        lWorld = normalize(lWorld);

        const F32 cosCone = dot(light.dWorld, -lWorld);
        if (light.cosOuterCone < cosCone)
        {
            if (cosCone < light.cosInnerCone)
            {
                return smoothstep(light.cosOuterCone, light.cosInnerCone, cosCone);
            }

            return 1.0f;
        }

        return 0.0f;
    }

    template< PathThroughputMode Mode >
    LightThroughput EvaluateDirectionalLight(PackedLight light, Material material, F32x3 nWorld, F32x3 vWorld)
    {
        const F32x3 lWorld = -light.dWorld;

        // TODO: double sideness
        //if (material.isDoubleSided && (dot(nWorld, lWorld) < 0))
        //{
        //    nWorld = -nWorld;
        //}

        const F32x3 lightIntensity = light.color * light.intensity;
        const PathThroughput throughput = ComputePathThroughput< Mode >(material, lWorld, nWorld, vWorld);

        LightThroughput result = (LightThroughput)0;
        result.diffuseRadiance  = lightIntensity * throughput.diffuse;
        result.specularRadiance = lightIntensity * throughput.specular;
        return result;
    }

    template< PathThroughputMode Mode >
    LightThroughput EvaluatePointLight(PackedLight light, Material material, F32x3 pWorld, F32x3 nWorld, F32x3 vWorld)
    {
        const F32x3 lWorld = light.pWorld - pWorld; // Will be normalized in Shade(...)
        const F32 distance = length(lWorld);

        // The GLTF attenuation function expects a range, make this happen CPU side?
        light.maxDistance = (0.0f < light.maxDistance) ? light.maxDistance : g_max_light_distance;
        if (g_max_light_distance < distance)
        {
            return (LightThroughput)0;
        }

        // TODO: double sideness
        //if (material.isDoubleSided && (dot(nWorld, lWorld) < 0))
        //{
        //   nWorld = -nWorld;
        //}

        const F32 attenuation = GetDistanceAttenuation(light, distance);
        const F32x3 lightIntensity = light.color * light.intensity * attenuation;
        const PathThroughput throughput = ComputePathThroughput< Mode >(material, lWorld, nWorld, vWorld);

        LightThroughput result = (LightThroughput)0;
        result.diffuseRadiance  = lightIntensity * throughput.diffuse;
        result.specularRadiance = lightIntensity * throughput.specular;
        return result;
    }

    template< PathThroughputMode Mode >
    LightThroughput EvaluateSpotLight(PackedLight light, Material material, F32x3 pWorld, F32x3 nWorld, F32x3 vWorld)
    {
        const F32x3 lWorld = light.pWorld - pWorld; // Will be normalized in Shade(...)
        const F32 distance = length(lWorld);

        // The GLTF attenuation function expects a range, make this happen CPU side?
        light.maxDistance = (0.0f < light.maxDistance) ? light.maxDistance : g_max_light_distance;

        // TODO: double sideness
        //if (material.isDoubleSided && (dot(nWorld, lWorld) < 0))
        //{
        //    nWorld = -nWorld;
        //}

        const F32 distanceAttenuation = GetDistanceAttenuation(light, distance);
        const F32 angularAttenuation = GetAngularAttenuation(light, lWorld);
        const F32 attenuation = distanceAttenuation * angularAttenuation;
        const F32x3 lightIntensity = light.color * light.intensity * attenuation;
        const PathThroughput throughput = ComputePathThroughput< Mode >(material, lWorld, nWorld, vWorld);

        LightThroughput result = (LightThroughput)0;
        result.diffuseRadiance  = lightIntensity * throughput.diffuse;
        result.specularRadiance = lightIntensity * throughput.specular;
        return result;
    }

    template< PathThroughputMode Mode >
    LightThroughput EvaluatePunctualLight(PackedLight light, Material material, F32x3 pWorld, F32x3 nWorld, F32x3 vWorld)
    {
        switch (light.type)
        {

        case LightType_Directional:
            return EvaluateDirectionalLight< Mode >(light, material, nWorld, vWorld);
        case LightType_Spot:
            return EvaluateSpotLight< Mode >(light, material, pWorld, nWorld, vWorld);
        case LightType_Point:
            return EvaluatePointLight< Mode >(light, material, pWorld, nWorld, vWorld);
        default:
            return (LightThroughput)0;
        };
    }
}
