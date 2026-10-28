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

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

// rr::OrthonormalBasis
#include "Basis.hlsli"
// DECLARE_CBV_BINDING
// DECLARE_SAMPLER_BINDING
// DECLARE_SAMPLER_ARRAY_BINDING
// DECLARE_SRV_BINDING
// DECLARE_SRV_ARRAY_BINDING
// DECLARE_UAV_BINDING
// DECLARE_UAV_ARRAY_BINDING
#include "Bindings.hlsli"
// rr::DispatchInput
#include "Dispatch.hlsli"
// rr::EvaluatePunctualLight
// rr::LightThroughput
#include "Lighting.hlsli"
// rr::Material
// rr::PathThroughputMode
#include "Material.hlsli"
// rr::g_epsilon
// rr::g_fp16_max
#include "Numeric.hlsli"
// rr::InvalidIndex
// rr::Map
// rr::PackedCamera
// rr::PackedFrame
// rr::PackedInstance
// rr::PackedLight
// rr::PackedLighting
// rr::PackedMaterial
// rr::PackedSurface
#include "PackedData.h"
// rr::ComputeMipLevelFromIntersection
// rr::FetchFaceIndicesU16
// rr::FetchFaceIndicesU32
// rr::FetchLocalBasis
// rr::FetchPixelFragment
// rr::g_rayEpsilon
// rr::Intersection
// rr::LocalBasis
// rr::PixelFragment
// rr::Ray
// rr::Sample
// rr::TraceRay
// rr::TraceRayOnHit
// rr::TraceRayOnMiss
// rr::TraceRayResult
// rr::TraceShadowRay
#include "PathTracing.hlsli"
// GROUP_THREAD_COUNT_X
// GROUP_THREAD_COUNT_Y
// GROUP_THREAD_COUNT_Z
// IBV_BEGIN_SLOT
// MAX_IBV_COUNT
// MAX_SAMPLER_COUNT
// MAX_TEXTURE_COUNT
// MAX_VBV_COUNT
// PACKED_INSTANCE_SLOT
// PACKED_MATERIAL_SLOT
// PACKED_SURFACE_SLOT
// SAMPLER_BEGIN_SLOT
// TEXTURE_BEGIN_SLOT
// VBV_BEGIN_SLOT
#include "PathTracingData.h"
// rr::SeedRngState
// rr::UniformUNorm
#include "Random.hlsli"
// rr::SampleDiffuseReflectionVector
// rr::SampleSpecularReflectionVector
#include "Sampling.hlsli"
// rr::EvaluateHardShadowFactor
// rr::EvaluateHitDistance
// rr::EvaluateSoftShadowFactor
#include "Shadow.hlsli"
// rr::NdcSpaceToWorldSpace
// rr::ScreenSpaceToNdcSpace
#include "Transformation.hlsli"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x4
// rr::U32
// rr::U32x2
// rr::U32x3
#include "Types.h"
// rr::EncodeUnitVector_Octahedron
#include "UnitVector.hlsli"

//------------------------------------------------------------------------------
// Sample Bindings
//------------------------------------------------------------------------------

DECLARE_CBV_BINDING(0, rr::PackedFrame, g_frame);

DECLARE_SRV_BINDING(0, RaytracingAccelerationStructure, g_tlas);

DECLARE_SRV_BINDING(1, Texture2D< rr::F32x4 >, g_brdf);
DECLARE_SRV_BINDING(2, TextureCube, g_prefilteredCube);

DECLARE_SRV_BINDING(PACKED_INSTANCE_SLOT, StructuredBuffer< rr::PackedInstance >, g_instances);
DECLARE_SRV_BINDING(PACKED_MATERIAL_SLOT, StructuredBuffer< rr::PackedMaterial >, g_materials);
DECLARE_SRV_BINDING(PACKED_SURFACE_SLOT,  StructuredBuffer< rr::PackedSurface >,  g_surfaces);
DECLARE_SRV_ARRAY_BINDING(TEXTURE_BEGIN_SLOT, Texture2D< rr::F32x4 >, MAX_TEXTURE_COUNT, g_textures);
DECLARE_SRV_ARRAY_BINDING(IBV_BEGIN_SLOT,     StructuredBuffer< rr::U32 >, MAX_IBV_COUNT, g_ibvs);
DECLARE_SRV_ARRAY_BINDING(VBV_BEGIN_SLOT,     StructuredBuffer< rr::F32 >, MAX_VBV_COUNT, g_vbvs);

DECLARE_SAMPLER_BINDING(0, SamplerState, g_brdfSampler);
DECLARE_SAMPLER_BINDING(1, SamplerState, g_prefilteredCubeSampler);
DECLARE_SAMPLER_ARRAY_BINDING(SAMPLER_BEGIN_SLOT, SamplerState, MAX_SAMPLER_COUNT, g_samplers);

DECLARE_UAV_BINDING( 0, RWTexture2D< rr::F32x3 >, g_directSpecular);
DECLARE_UAV_BINDING( 1, RWTexture2D< rr::F32x3 >, g_directDiffuse);
DECLARE_UAV_BINDING( 2, RWTexture2D< rr::F32x4 >, g_indirectSpecular);
DECLARE_UAV_BINDING( 3, RWTexture2D< rr::F32x4 >, g_indirectDiffuse);
DECLARE_UAV_BINDING( 4, RWTexture2D< rr::F32 >,   g_dominantLightHitDistance);
DECLARE_UAV_BINDING( 5, RWTexture2D< rr::F32 >,   g_ambientOcclusion);
DECLARE_UAV_BINDING( 6, RWTexture2D< rr::F32 >,   g_specularOcclusion);
DECLARE_UAV_BINDING( 7, RWTexture2D< rr::F32x3 >, g_skip);
DECLARE_UAV_BINDING( 8, RWTexture2D< rr::F32x3 >, g_diffuseAlbedo);
DECLARE_UAV_BINDING( 9, RWTexture2D< rr::F32x3 >, g_specularAlbedo);
DECLARE_UAV_BINDING(10, RWTexture2D< rr::F32x4 >, g_normals);

#define ROOT_SIGNATURE                                                         \
    "RootFlags(0),"                                                            \
    /* ---- CBVs ---- */                                                       \
    "CBV(b0, visibility = SHADER_VISIBILITY_ALL)," /* g_frame */               \
    /* ---- SRVs ---- */                                                       \
    "SRV(t0, visibility = SHADER_VISIBILITY_ALL)," /* g_tlas */                \
    /* ---- Samplers ---- */                                                   \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        DECLARE_SAMPLER_ARRAY_DESCRIPTOR_RANGE(SAMPLER_BEGIN_SLOT, MAX_SAMPLER_COUNT)\
    "),"                                                                       \
    /* ---- SRVs ---- */                                                       \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        "SRV(t1)," /* g_brdf */                                                \
        "SRV(t2)," /* g_prefilteredCube */                                     \
        "SRV(t3)," /* g_instances */                                           \
        "SRV(t4)," /* g_materials */                                           \
        "SRV(t5)," /* g_surfaces */                                            \
        DECLARE_SRV_ARRAY_DESCRIPTOR_RANGE(TEXTURE_BEGIN_SLOT, MAX_TEXTURE_COUNT)","\
        DECLARE_SRV_ARRAY_DESCRIPTOR_RANGE(IBV_BEGIN_SLOT, MAX_IBV_COUNT)","   \
        DECLARE_SRV_ARRAY_DESCRIPTOR_RANGE(VBV_BEGIN_SLOT, MAX_VBV_COUNT)      \
    "),"                                                                       \
    /* ---- UAVs ---- */                                                       \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        "UAV(u0)," /* g_directSpecular */                                      \
        "UAV(u1)," /* g_directDiffuse */                                       \
        "UAV(u2)," /* g_indirectSpecular */                                    \
        "UAV(u3)," /* g_indirectDiffuse */                                     \
        "UAV(u4)," /* g_dominantLightHitDistance */                            \
        "UAV(u5)," /* g_ambientOcclusion */                                    \
        "UAV(u6)," /* g_specularOcclusion */                                   \
        "UAV(u7)," /* g_skip */                                                \
        "UAV(u8)," /* g_diffuseAlbedo */                                       \
        "UAV(u9)," /* g_specularAlbedo */                                      \
        "UAV(u10)" /* g_normals */                                             \
    "),"                                                                       \
    "StaticSampler(                                                            \
        s0,"       /* g_brdfSampler */                                         \
        "filter         = FILTER_MIN_MAG_MIP_LINEAR,"                          \
        "addressU       = TEXTURE_ADDRESS_WRAP,"                               \
        "addressV       = TEXTURE_ADDRESS_WRAP,"                               \
        "addressW       = TEXTURE_ADDRESS_WRAP,"                               \
        "borderColor    = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"              \
        "maxAnisotropy  = 1,"                                                  \
        "comparisonFunc = COMPARISON_ALWAYS,"                                  \
        "visibility     = SHADER_VISIBILITY_ALL                                \
    ),"                                                                        \
    "StaticSampler(                                                            \
        s1,"       /* g_prefilteredCubeSampler */                              \
        "filter         = FILTER_MIN_MAG_MIP_LINEAR,"                          \
        "addressU       = TEXTURE_ADDRESS_CLAMP,"                              \
        "addressV       = TEXTURE_ADDRESS_CLAMP,"                              \
        "addressW       = TEXTURE_ADDRESS_WRAP,"                               \
        "borderColor    = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"              \
        "maxAnisotropy  = 16,"                                                 \
        "comparisonFunc = COMPARISON_ALWAYS,"                                  \
        "visibility     = SHADER_VISIBILITY_ALL                                \
    )"

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
    F32x4 SampleBrdf(F32x2 uv)
    {
        return g_brdf.SampleLevel(g_brdfSampler, uv, 0);
    }

    F32x4 SamplePrefilteredCube(F32x3 reflection, F32 lod)
    {
        return g_prefilteredCube.SampleLevel(g_prefilteredCubeSampler, reflection, lod);
    }

    // Primary hit: dominant light → DLV hit distance for the denoiser (re-added in compose
    // via denoised visibility to avoid double-counting). All other lights use soft shadows.
    LightThroughput ComputeDirectLighting(inout U32 rngState,
                                          out F32 dominantLightHitDistance,
                                          F32x3 pWorld,
                                          F32x3 nWorld,
                                          F32x3 ngWorld,
                                          F32x3 vWorld,
                                          Material material)
    {
        LightThroughput result = (LightThroughput)0;

        dominantLightHitDistance = 0.0f;
        if (0.0f < g_frame.lighting.dominantLight.intensity)
        {
            dominantLightHitDistance = EvaluateHitDistance(g_tlas, g_frame.lighting.dominantLight, rngState, pWorld, nWorld, ngWorld);
        }

        for (U32 i = 0u; i < g_frame.lighting.lightCount; ++i)
        {
            const PackedLight light        = g_frame.lighting.lights[i];
            const F32         shadowFactor = EvaluateSoftShadowFactor(g_tlas, light, rngState, pWorld, nWorld, ngWorld);
            if (0.0f < shadowFactor)
            {
                const LightThroughput throughput = EvaluatePunctualLight< PathThroughputMode_Direct >(light, material, pWorld, nWorld, vWorld);
                result.diffuseRadiance  += throughput.diffuseRadiance  * shadowFactor;
                result.specularRadiance += throughput.specularRadiance * shadowFactor;
            }
        }

        return result;
    }

    // Secondary hit: dominant light contributes directly (no DLV for bounces).
    // All lights including dominant use hard shadows and albedo-modulated throughput.
    LightThroughput ComputeIndirectLighting(inout U32 rngState,
                                            F32x3 pWorld,
                                            F32x3 nWorld,
                                            F32x3 ngWorld,
                                            F32x3 vWorld,
                                            Material material)
    {
        LightThroughput result = (LightThroughput)0;

        if (0.0f < g_frame.lighting.dominantLight.intensity)
        {
            const F32 shadowFactor = EvaluateHardShadowFactor(g_tlas, g_frame.lighting.dominantLight, rngState, pWorld, ngWorld);
            if (0.0f < shadowFactor)
            {
                const LightThroughput throughput = EvaluatePunctualLight< PathThroughputMode_Indirect >(g_frame.lighting.dominantLight, material, pWorld, nWorld, vWorld);
                result.diffuseRadiance  += throughput.diffuseRadiance  * shadowFactor;
                result.specularRadiance += throughput.specularRadiance * shadowFactor;
            }
        }

        for (U32 i = 0u; i < g_frame.lighting.lightCount; ++i)
        {
            const PackedLight light        = g_frame.lighting.lights[i];
            const F32         shadowFactor = EvaluateHardShadowFactor(g_tlas, light, rngState, pWorld, ngWorld);
            if (0.0f < shadowFactor)
            {
                const LightThroughput throughput = EvaluatePunctualLight< PathThroughputMode_Indirect >(light, material, pWorld, nWorld, vWorld);
                result.diffuseRadiance  += throughput.diffuseRadiance  * shadowFactor;
                result.specularRadiance += throughput.specularRadiance * shadowFactor;
            }
        }

        return result;
    }

    void TraceRayOnHit(inout TraceRayResult result,
                       in Ray ray,
                       in Intersection intersection,
                       inout U32 rngState)
    {
        const PackedInstance packedInstance = g_instances[intersection.instanceIndex];
        const PackedSurface  packedSurface  = g_surfaces[packedInstance.surfaceOffset + intersection.geometryIndex];
        const PackedMaterial packedMaterial = g_materials[packedSurface.materialIndex];

        U32x3 faceIndices;
        if (packedSurface.indexByteCount == 2u)
        {
            faceIndices = FetchFaceIndicesU16(g_ibvs, packedSurface.ibvIndex, intersection.triangleIndex);
        }
        if (packedSurface.indexByteCount == 4u)
        {
            faceIndices = FetchFaceIndicesU32(g_ibvs, packedSurface.ibvIndex, intersection.triangleIndex);
        }

        const LocalBasis localBasis = FetchLocalBasis(g_vbvs,
                                                      packedSurface,
                                                      intersection.objectToWorld,
                                                      faceIndices,
                                                      intersection.barycentrics,
                                                      intersection.isFrontFace);

        const F32 mipLevel = ComputeMipLevelFromIntersection(intersection.tHit, localBasis);

        PixelFragment fragment = FetchPixelFragment(g_textures, g_samplers, localBasis, packedMaterial, mipLevel);
        fragment.pWorld = intersection.pWorld;

        Material material = GetMaterial(fragment);
        F32 dominantLightHitDistance = 0.0f;
        LightThroughput throughput;
        if (ray.recursionDepth == 0u)
        {
            throughput = ComputeDirectLighting(rngState, dominantLightHitDistance, fragment.pWorld, fragment.nWorld, localBasis.ngWorld, -ray.dWorld, material);
        }
        else
        {
            throughput = ComputeIndirectLighting(rngState, fragment.pWorld, fragment.nWorld, localBasis.ngWorld, -ray.dWorld, material);
        }

        F32x3 emission;
        if ((packedMaterial.emission.textureIndex != InvalidIndex) && any(0.0f < packedMaterial.emissionFactor))
        {
            emission = Sample(g_textures, g_samplers, packedMaterial.emission, localBasis.uv, 0.0f).xyz;
        }
        else
        {
            emission = 0.0f;
        }
        emission *= packedMaterial.emissionFactor;

        result.radiance                 = throughput.diffuseRadiance + throughput.specularRadiance + emission;
        result.diffuseRadiance          = throughput.diffuseRadiance;
        result.specularRadiance         = throughput.specularRadiance;
        result.emission                 = emission;
        result.material                 = material;
        result.localBasis               = localBasis;
        result.pWorld                   = fragment.pWorld;
        result.nWorld                   = fragment.nWorld;
        result.dominantLightHitDistance = dominantLightHitDistance;
    }

    void TraceRayOnMiss(inout TraceRayResult result,
                        in Ray ray,
                        inout U32 rngState)
    {
        result.radiance = SamplePrefilteredCube(ray.dWorld, 0.0f).xyz * g_frame.lighting.iblFactor;
    }

    void Main(DispatchInput input)
    {
        const U32x2 pixel = input.dispatchThreadId.xy;
        if (any(g_frame.renderSize <= pixel))
        {
            return;
        }

        // Checkerboard: radiance/occlusion outputs are packed to the left half of the target.
        const U32   checkerboardOrigin  = g_frame.frameIndex & 1u;
        const bool  isCheckerboard      = (g_frame.checkerboard != 0u);
        const bool  hasCheckerboardData = (!isCheckerboard) || ((((pixel.x ^ pixel.y) & 1u) == checkerboardOrigin));
        const U32x2 checkerboardPixel   = isCheckerboard ? U32x2(pixel.x >> 1u, pixel.y) : pixel;

        // Radiance / occlusion signals (checkerboard-packed).
        F32x3 directSpecular           = 0.0f;
        F32x3 directDiffuse            = 0.0f;
        F32x4 indirectSpecular         = F32x4(0.0f, 0.0f, 0.0f, g_fp16_max);
        F32x4 indirectDiffuse          = F32x4(0.0f, 0.0f, 0.0f, g_fp16_max);
        F32   dominantLightHitDistance = 0.0f;
        F32   ambientOcclusion         = 1.0f;
        F32   specularOcclusion        = 1.0f;
        // Full-resolution G-buffer signals.
        F32x3 skip                     = 0.0f;
        F32x3 diffuseAlbedo            = 0.0f;
        F32x3 specularAlbedo           = 0.0f;
        F32x4 normalRoughnessMaterial  = 0.0f;

        const F32x2 uv        = (F32x2(pixel) + 0.5f) * g_frame.invRenderSize;
        const F32x2 pNdc      = ScreenSpaceToNdcSpace(uv);
        const F32x3 cWorld    = mul(g_frame.camera.viewToWorld, F32x4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
        const F32x3 nearWorld = NdcSpaceToWorldSpace(F32x3(pNdc, g_frame.camera.usesReversedZ ? 1.0f : 0.0f), g_frame.camera.clipToWorld);
        const F32x3 vWorld    = normalize(cWorld - nearWorld);

        U32 rngState = SeedRngState(pixel.y, pixel.x, g_frame.frameIndex + 1u);

        // Trace primary ray
        Ray primaryRay;
        primaryRay.oWorld         = nearWorld;
        primaryRay.dWorld         = -vWorld;
        primaryRay.tMin           = 0.0f;
        primaryRay.tMax           = 1024.0f;
        primaryRay.recursionDepth = 0u;

        const TraceRayResult primaryResult = TraceRay(g_tlas, primaryRay, rngState);

        if (primaryResult.hit)
        {
            const Material material     = primaryResult.material;
            const F32      roughness    = saturate(material.perceptualRoughness);
            const F32x3x3  localToWorld = OrthonormalBasis(primaryResult.nWorld);

            // Full-resolution G-buffer outputs.
            {
                const F32   roll   = UniformUNorm(rngState);
                // DXGI_FORMAT_R10G10B10A2 => quantization step = 1/1024 (reduce 10-bit banding).
                const F32   dither = lerp(-1.0f, 1.0f, roll) / 1024.0f;
                const F32   NoV    = dot(primaryResult.nWorld, vWorld);
                // In DirectX and Vulkan, the top left corner is the origin and the
                // y-axis points down.
                // In the BRDF LUT, the bottom left corner is the origin and the
                // y-axis points up (increasing roughness).
                // => y-axis points down (increasing smoothness)
                const F32x2 brdfSamplePoint = saturate(F32x2(NoV, 1.0f - material.perceptualRoughness));
                // Retrieve a scale and bias to F0. See [1], Figure 3.
                const F32x2 brdf            = SampleBrdf(brdfSamplePoint).rg;
                const F32x3 specularFactor  = material.reflectance0 * brdf.x + brdf.y;

                skip            = primaryResult.emission;
                // sqrt encoding
                diffuseAlbedo   = sqrt(material.baseColor * (1.0f - material.metalness));
                specularAlbedo  = sqrt(saturate(abs(specularFactor + dither)));
                // octahedron encoding
                normalRoughnessMaterial = F32x4(EncodeUnitVector_Octahedron(primaryResult.nWorld), roughness, 0.0f);
            }

            // Radiance + occlusion (checkerboard-packed, only for pixels with data this frame).
            if (hasCheckerboardData)
            {
                F32x3 indirectSpecularRadiance    = 0.0f;
                F32   indirectSpecularHitDistance = g_fp16_max;
                {
                    F32 pdf;
                    const F32x3 dWorld = SampleSpecularReflectionVector(rngState, localToWorld, vWorld, roughness, pdf);
                    const F32 sampleWeight = 0.5f / max(pdf, g_epsilon);

                    // Trace indirect specular ray
                    Ray secondaryRay;
                    secondaryRay.oWorld         = primaryResult.pWorld + primaryResult.localBasis.ngWorld * g_rayEpsilon;
                    secondaryRay.dWorld         = dWorld;
                    secondaryRay.tMin           = primaryRay.tMin;
                    secondaryRay.tMax           = primaryRay.tMax;
                    secondaryRay.recursionDepth = primaryRay.recursionDepth + 1u;

                    const TraceRayResult secondaryResult = TraceRay(g_tlas, secondaryRay, rngState);
                    indirectSpecularRadiance = sampleWeight * secondaryResult.radiance;
                    if (secondaryResult.hit)
                    {
                        indirectSpecularHitDistance = length(secondaryRay.oWorld - secondaryResult.pWorld);
                    }
                }

                F32x3 indirectDiffuseRadiance    = 0.0f;
                F32   indirectDiffuseHitDistance = g_fp16_max;
                {
                    F32 pdf;
                    const F32x3 dWorld = SampleDiffuseReflectionVector(rngState, localToWorld, pdf);
                    const F32 sampleWeight = 0.5f / max(pdf, g_epsilon);

                    // Trace indirect diffuse ray
                    Ray secondaryRay;
                    secondaryRay.oWorld         = primaryResult.pWorld + primaryResult.localBasis.ngWorld * g_rayEpsilon;
                    secondaryRay.dWorld         = dWorld;
                    secondaryRay.tMin           = primaryRay.tMin;
                    secondaryRay.tMax           = primaryRay.tMax;
                    secondaryRay.recursionDepth = primaryRay.recursionDepth + 1u;

                    const TraceRayResult secondaryResult = TraceRay(g_tlas, secondaryRay, rngState);
                    indirectDiffuseRadiance = sampleWeight * secondaryResult.radiance;
                    if (secondaryResult.hit)
                    {
                        indirectDiffuseHitDistance = length(secondaryRay.oWorld - secondaryResult.pWorld);
                    }
                }

                const F32 primaryHitDistance = length(primaryRay.oWorld - primaryResult.pWorld);

                directSpecular           = primaryResult.specularRadiance;
                directDiffuse            = primaryResult.diffuseRadiance;
                indirectSpecular         = F32x4(indirectSpecularRadiance, indirectSpecularHitDistance);
                indirectDiffuse          = F32x4(indirectDiffuseRadiance,  indirectDiffuseHitDistance);
                // Dominant light hit distance is in [0,g_fp16_max]
                dominantLightHitDistance = primaryResult.dominantLightHitDistance;
                // Ambient  occlusion is in [0,1]
                ambientOcclusion         = saturate(indirectDiffuseHitDistance  / (g_frame.ambientOcclusionA  + primaryHitDistance * g_frame.ambientOcclusionB));
                // Specular occlusion is in [0,1]
                specularOcclusion        = saturate(indirectSpecularHitDistance / (g_frame.specularOcclusionA + primaryHitDistance * g_frame.specularOcclusionB));
            }
        }
        else
        {
            skip = primaryResult.radiance;
        }

        // Full-resolution G-buffer writes.
        g_diffuseAlbedo[pixel]  = diffuseAlbedo;
        g_specularAlbedo[pixel] = specularAlbedo;
        g_normals[pixel]        = normalRoughnessMaterial;
        g_skip[pixel]           = skip;

        // Checkerboard-packed radiance / occlusion writes.
        if (hasCheckerboardData)
        {
            g_directSpecular[checkerboardPixel]           = directSpecular;
            g_directDiffuse[checkerboardPixel]            = directDiffuse;
            g_indirectSpecular[checkerboardPixel]         = indirectSpecular;
            g_indirectDiffuse[checkerboardPixel]          = indirectDiffuse;
            g_dominantLightHitDistance[checkerboardPixel] = dominantLightHitDistance;
            g_ambientOcclusion[checkerboardPixel]         = ambientOcclusion;
            g_specularOcclusion[checkerboardPixel]        = specularOcclusion;
        }
    }
}

//------------------------------------------------------------------------------
// Sample Entry Point
//------------------------------------------------------------------------------

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_THREAD_COUNT_X, GROUP_THREAD_COUNT_Y, GROUP_THREAD_COUNT_Z)]
void Main(rr::DispatchInput input)
{
    rr::Main(input);
}
