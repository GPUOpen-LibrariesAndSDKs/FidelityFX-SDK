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

// rr::CreateMaterial
// rr::Material
#include "Material.hlsli"
// rr::g_fp16_max
#include "Numeric.hlsli"
// rr::InvalidIndex
// rr::Map
// rr::PackedMaterial
// rr::PackedSurface
#include "PackedData.h"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x3x4
// rr::F32x4
// rr::U32
// rr::U32x3
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    //--------------------------------------------------------------------------
    // Geometry
    //--------------------------------------------------------------------------

    U32x3 FetchFaceIndicesU32(StructuredBuffer< U32 > ibvs[], U32 ibvIndex, U32 triangleIndex)
    {
        U32x3 faceIndices;
        [unroll]
        for (U32 i = 0u; i < 3u; ++i)
        {
            faceIndices[i] = ibvs[NonUniformResourceIndex(ibvIndex)][3u * triangleIndex + i];
        }
        return faceIndices;
    }

    U32x3 FetchFaceIndicesU16(StructuredBuffer< U32 > ibvs[], U32 ibvIndex, U32 triangleIndex)
    {
        const U32x3 u32Indices = U32x3(3u * triangleIndex + 0u,
                                       3u * triangleIndex + 1u,
                                       3u * triangleIndex + 2u);
        const U32x3 u16BaseIndices = u32Indices / 2u;
        const U32x3 u16Shift       = 16u * (u32Indices & 0x1u);
        const U32   u16Mask        = 0xFFFFu;

        U32x3 faceIndices;
        [unroll]
        for (U32 i = 0u; i < 3u; ++i)
        {
            faceIndices[i] = (ibvs[NonUniformResourceIndex(ibvIndex)][u16BaseIndices[i]] >> u16Shift[i]) & u16Mask;
        }
        return faceIndices;
    }

    F32 FetchVertexF32(StructuredBuffer< F32 > vbvs[], U32 vbvIndex, U32 vertexIndex)
    {
        return vbvs[NonUniformResourceIndex(vbvIndex)][vertexIndex];
    }

    F32x2 FetchVertexF32x2(StructuredBuffer< F32 > vbvs[], U32 vbvIndex, U32 vertexIndex)
    {
        F32x2 vertex;
        [unroll]
        for (U32 i = 0u; i < 2u; ++i)
        {
            vertex[i] = vbvs[NonUniformResourceIndex(vbvIndex)][2u * vertexIndex + i];
        }
        return vertex;
    }

    F32x3 FetchVertexF32x3(StructuredBuffer< F32 > vbvs[], U32 vbvIndex, U32 vertexIndex)
    {
        F32x3 vertex;
        [unroll]
        for (U32 i = 0u; i < 3u; ++i)
        {
            vertex[i] = vbvs[NonUniformResourceIndex(vbvIndex)][3u * vertexIndex + i];
        }
        return vertex;
    }

    F32x4 FetchVertexF32x4(StructuredBuffer< F32 > vbvs[], U32 vbvIndex, U32 vertexIndex)
    {
        F32x4 vertex;
        [unroll]
        for (U32 i = 0u; i < 4u; ++i)
        {
            vertex[i] = vbvs[NonUniformResourceIndex(vbvIndex)][4u * vertexIndex + i];
        }
        return vertex;
    }

    struct Vertex
    {
        // The position of the vertex, expressed in object space.
        F32x3 pObject;
        // The normal of the vertex, expressed in object space.
        F32x3 nObject;
        // The tangent of the vertex, expressed in object space.
        F32x4 tObject;
        // The texture coordinates of the vertex, expressed in UV space.
        F32x2 uv;
    };

    Vertex FetchVertexAttributes(StructuredBuffer< F32 > vbvs[], PackedSurface surface, U32 vertexIndex)
    {
        Vertex vertex;

        #define FETCH_ATTRIBUTE(member, fetchFunction, vbvIndex)               \
            vertex.member = (surface.vbvIndex != InvalidIndex)                 \
                          ? fetchFunction(vbvs, surface.vbvIndex, vertexIndex) \
                          : 0.0f

        FETCH_ATTRIBUTE(pObject, FetchVertexF32x3, positionVbvIndex);
        FETCH_ATTRIBUTE(nObject, FetchVertexF32x3, normalVbvIndex);
        FETCH_ATTRIBUTE(tObject, FetchVertexF32x4, tangentVbvIndex);
        FETCH_ATTRIBUTE(uv,      FetchVertexF32x2, texcoord0VbvIndex);

        #undef FETCH_ATTRIBUTE

        return vertex;
    }

    struct LocalBasis
    {
        // Interpolated vertex normal, expressed in world space.
        // The smooth surface normal that normal mapping perturbs to produce
        // the shading normal.
        F32x3 nWorld;
        F32 padding0;

        // Geometric face normal, expressed in world space (perpendicular to
        // the triangle, oriented into nWorld's hemisphere).
        // Used for ray-origin offsets, to avoid self-intersection.
        F32x3 ngWorld;
        F32 padding1;

        // .xyz = The tangent of the local basis, expressed in world space.
        // .w   = The handedness/sign of bitangent of the local basis.
        F32x4 tWorld;

        F32x2 uv;
        // The area of the triangle in UV space.
        F32 surfaceAreaUv;
        // The area of the triangle in world space.
        F32 surfaceAreaWorld;
    };

    LocalBasis FetchLocalBasis(StructuredBuffer< F32 > vbvs[], PackedSurface surface,
                               F32x3x4 objectToWorld,
                               U32x3 faceIndices,
                               F32x2 barycentrics,
                               bool isFrontFace)
    {
        const F32x3 bs = F32x3(1.0f - barycentrics.x - barycentrics.y, barycentrics.xy);

        Vertex vertices[3u];
        F32x3 pWorld[3u];
        Vertex interpolated = (Vertex)0;
        [unroll]
        for (uint i = 0u; i < 3u; ++i)
        {
            vertices[i] = FetchVertexAttributes(vbvs, surface, faceIndices[i]);
            pWorld[i]   = mul(objectToWorld, F32x4(vertices[i].pObject, 1.0f));

            interpolated.pObject += bs[i] * vertices[i].pObject;
            interpolated.nObject += bs[i] * vertices[i].nObject;
            interpolated.tObject += bs[i] * vertices[i].tObject;
            interpolated.uv      += bs[i] * vertices[i].uv;
        }

        LocalBasis localBasis = (LocalBasis)0;
        localBasis.nWorld     = normalize(mul(objectToWorld, F32x4(interpolated.nObject,     0.0f)));
        localBasis.tWorld.xyz = normalize(mul(objectToWorld, F32x4(interpolated.tObject.xyz, 0.0f)));
        localBasis.tWorld.w   = interpolated.tObject.w;
        localBasis.uv         = interpolated.uv;
        localBasis.surfaceAreaUv = abs(cross(F32x3(vertices[1u].uv - vertices[0u].uv, 0.0f),
                                             F32x3(vertices[2u].uv - vertices[0u].uv, 0.0f)).z) * 0.5f;

        const F32x3 edgeCross = cross(pWorld[1u] - pWorld[0u], pWorld[2u] - pWorld[0u]);
        localBasis.surfaceAreaWorld = length(edgeCross) * 0.5f;

        if (isFrontFace == false)
        {
            localBasis.nWorld = -localBasis.nWorld;
        }

        // Face normal, oriented into the (front-face-corrected) interpolated
        // normal's hemisphere.
        F32x3 ngWorld = normalize(edgeCross);
        if (dot(ngWorld, localBasis.nWorld) < 0.0f)
        {
            ngWorld = -ngWorld;
        }
        localBasis.ngWorld = ngWorld;

        return localBasis;
    }

    //--------------------------------------------------------------------------
    // Material
    //--------------------------------------------------------------------------

    F32x4 Sample(Texture2D< F32x4 > textures[], SamplerState samplers[], Map map, F32x2 uv, F32 mipLevel)
    {
        const SamplerState s       = samplers[NonUniformResourceIndex(map.samplerIndex)];
        const Texture2D< F32x4 > t = textures[NonUniformResourceIndex(map.textureIndex)];

        return t.SampleLevel(s, uv, mipLevel);
    }

    struct PixelFragment
    {
        F32x3 pWorld;
        F32x3 nWorld;

        F32x4 baseColor;
        F32 ambientOcclusion;
        F32 roughness;
        F32 metalness;
    };

    PixelFragment FetchPixelFragment(Texture2D< F32x4 > textures[],
                                     SamplerState samplers[],
                                     LocalBasis localBasis,
                                     PackedMaterial material,
                                     F32 mipLevel)
    {
        PixelFragment fragment = (PixelFragment)0;

        #define SAMPLE_MAP(map)\
            Sample(textures, samplers, map, localBasis.uv, mipLevel)

        // Normal
        F32x3 nWorld = localBasis.nWorld;
        F32x3 tWorld = localBasis.tWorld.xyz;
        if ((material.normal.textureIndex != InvalidIndex) && any(0.0f < abs(tWorld)))
        {
            const F32x3 bWorld = normalize(cross(nWorld, tWorld)) * localBasis.tWorld.w;

            F32x3 nLocal;
            nLocal.xy = UNormToSNorm(SAMPLE_MAP(material.normal).xy);
            nLocal.z  = sqrt(1.0f - saturate(dot(nLocal.xy, nLocal.xy)));

            nWorld = normalize(nLocal.x * tWorld + nLocal.y * bWorld + nLocal.z * nWorld);
        }
        fragment.nWorld = nWorld;

        // Base Color (incl. Alpha)
        if (material.baseColor.textureIndex != InvalidIndex)
        {
            fragment.baseColor = SAMPLE_MAP(material.baseColor);
        }
        else
        {
            fragment.baseColor = 1.0f; // glTF default
        }
        fragment.baseColor *= material.baseColorFactor;

        // Ambient Occlusion - Roughness - Metalness
        if (material.ambientOcclusionRoughnessMetalness.textureIndex != InvalidIndex)
        {
            const F32x3 s = SAMPLE_MAP(material.ambientOcclusionRoughnessMetalness).xyz;

            fragment.ambientOcclusion = s.x;
            fragment.roughness        = s.y;
            fragment.metalness        = s.z;
        }
        else
        {
            fragment.ambientOcclusion = 1.0f;
            fragment.roughness        = 1.0f; // glTF default
            fragment.metalness        = 1.0f; // glTF default
        }
        fragment.ambientOcclusion *= material.ambientOcclusionFactor;
        fragment.roughness        *= material.roughnessFactor;
        fragment.metalness        *= material.metalnessFactor;

        #undef SAMPLE_MAP

        return fragment;
    }

    Material GetMaterial(PixelFragment fragment)
    {
        return CreateMaterial(fragment.baseColor.rgb,
                              fragment.roughness,
                              fragment.metalness);
    }

    //--------------------------------------------------------------------------
    // Ray Tracing
    //--------------------------------------------------------------------------

    static const F32 g_rayEpsilon = 1e-3f;

    struct Ray
    {
        // The origin of this ray, expressed in world space.
        // This is the point from which the ray starts (i.e. t == 0.0f).
        F32x3 oWorld;

        // The minimum ray parameter of this ray.
        // Intersections with t < tMin are ignored. This is commonly set to a
        // small positive value (e.g. 1e-3f) to avoid self-intersections due to
        // floating-point precision issues.
        F32 tMin;

        // The direction of this ray, expressed in world space.
        // This ray direction should be normalized. The ray direction determines
        // the positive direction of increasing t.
        F32x3 dWorld;

        // The maximum ray parameter of this ray.
        // Intersections with t > tMax are ignored. This can be used to limit ray
        // length (e.g. for shadow rays or visibility queries).
        F32 tMax;

        // The recursion depth of this ray.
        // A recursion depth of 0 corresponds to the 1st path segment (i.e. primary ray).
        //   ...
        // A recursion depth of n corresponds to the (n+1)th path segment.
        U32 recursionDepth;
    };

    struct Intersection
    {
        F32x3x4 objectToWorld;

        // The intersection position of this intersection, expressed in world
        // space.
        F32x3 pWorld;

        // The ray parameter at the intersection point of this intersection.
        F32 tHit;

        // The barycentrics coordinates at the intersection point of this
        // intersection.
        F32x2 barycentrics;

        // The instance index of this intersection.
        // This index maps to the D3D12_RAYTRACING_INSTANCE_DESC::InstanceID of
        // the intersected instance. This is part of the TLAS construction.
        U32 instanceIndex;

        // The geometry index of this intersection.
        // This index maps to the D3D12_RAYTRACING_GEOMETRY_DESC index of the
        // intersected surface within the intersected mesh referenced by the
        // intersected instance. This is part of the BLASs construction.
        U32 geometryIndex;

        // The triangle index of this intersection.
        // This index maps to the intersected triangle of the intersected mesh
        // referenced by the intersected instance. This is part of the IBV and
        // VBV used during the BLASs construction.
        U32 triangleIndex;

        // A flag specifying whether the intersected triangle is front-facing or
        // back-facing.
        bool isFrontFace;
    };

    struct TraceRayResult
    {
        bool hit;
        F32x3 radiance;
        F32x3 diffuseRadiance;
        F32x3 specularRadiance;
        F32x3 emission;

        Material material;
        LocalBasis localBasis;
        F32x3 pWorld;
        F32x3 nWorld;
        F32 dominantLightHitDistance;
    };

    // Declaration
    void TraceRayOnHit(inout TraceRayResult result,
                       in Ray ray,
                       in Intersection intersection,
                       inout U32 rngState);
    // Declaration
    void TraceRayOnMiss(inout TraceRayResult result,
                        in Ray ray,
                        inout U32 rngState);

    TraceRayResult TraceRay(RaytracingAccelerationStructure rtas, in Ray ray, inout U32 rngState)
    {
        // Convert (custom) Ray to (intrinsic) RayDesc
        RayDesc rayDesc;
        rayDesc.Origin    = ray.oWorld;
        rayDesc.TMin      = ray.tMin;
        rayDesc.Direction = ray.dWorld;
        rayDesc.TMax      = ray.tMax;

        RayQuery< RAY_FLAG_NONE > rayQuery;
        const U32 rayFlags = RAY_FLAG_CULL_NON_OPAQUE
                           | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
        // Set up a ray trace.
        rayQuery.TraceRayInline(rtas, rayFlags, 0xFFu /* InstanceInclusionMask */, rayDesc);
        // Perform the ray traversal.
        rayQuery.Proceed();

        TraceRayResult result = (TraceRayResult)0;
        if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            // Convert (intrinsic) RayQuery to (custom) Intersection
            Intersection intersection;
            intersection.objectToWorld = rayQuery.CommittedObjectToWorld3x4();
            intersection.tHit          = rayQuery.CommittedRayT();
            intersection.pWorld        = ray.oWorld + ray.dWorld * intersection.tHit;
            intersection.barycentrics  = rayQuery.CommittedTriangleBarycentrics();
            intersection.instanceIndex = rayQuery.CommittedInstanceID();
            intersection.geometryIndex = rayQuery.CommittedGeometryIndex();
            intersection.triangleIndex = rayQuery.CommittedPrimitiveIndex();
            intersection.isFrontFace   = rayQuery.CommittedTriangleFrontFace();

            TraceRayOnHit(result, ray, intersection, rngState);
            result.hit = true;
        }
        else
        {
            TraceRayOnMiss(result, ray, rngState);
            result.hit = false;
        }

        return result;
    }

    F32 ComputeMipLevelFromIntersection(F32 tHit, LocalBasis localBasis)
    {
        // Assuming a pin-hole camera projection.
        // The further the intersection, the smaller the projected screen-space area.
        const F32 surfaceAreaScreen = localBasis.surfaceAreaWorld / (tHit * tHit);
        // Compare texture-space area to projected screen-space area.
        // * Large footprint -> one pixel covers a lot of texture -> higher mip level.
        // * Small footprint -> one pixel covers little texture   -> lower  mip level.
        const F32 footprint = localBasis.surfaceAreaUv / surfaceAreaScreen;
        const F32 lambda = 0.5f * log2(footprint);

        return max(0.0f, lambda);
    }

    F32 TraceHitDistanceRay(RaytracingAccelerationStructure rtas, in Ray ray)
    {
        // Convert (custom) Ray to (intrinsic) RayDesc
        RayDesc rayDesc;
        rayDesc.Origin    = ray.oWorld;
        rayDesc.TMin      = ray.tMin;
        rayDesc.Direction = ray.dWorld;
        rayDesc.TMax      = ray.tMax;

        RayQuery< RAY_FLAG_NONE > rayQuery;
        const U32 rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
                           | RAY_FLAG_CULL_NON_OPAQUE
                           | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
        // Set up a ray trace.
        rayQuery.TraceRayInline(rtas, rayFlags, 0xFFu /* InstanceInclusionMask */, rayDesc);
        // Perform the ray traversal.
        rayQuery.Proceed();

        return (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? rayQuery.CommittedRayT() : g_fp16_max;
    }

    bool TraceShadowRay(RaytracingAccelerationStructure rtas, in Ray ray)
    {
        // Convert (custom) Ray to (intrinsic) RayDesc
        RayDesc rayDesc;
        rayDesc.Origin    = ray.oWorld;
        rayDesc.TMin      = ray.tMin;
        rayDesc.Direction = ray.dWorld;
        rayDesc.TMax      = ray.tMax;

        RayQuery< RAY_FLAG_NONE > rayQuery;
        const U32 rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
                           | RAY_FLAG_CULL_NON_OPAQUE
                           | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
                           | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
        // Set up a ray trace.
        rayQuery.TraceRayInline(rtas, rayFlags, 0xFFu /* InstanceInclusionMask */, rayDesc);
        // Perform the ray traversal.
        rayQuery.Proceed();

        return rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
    }
}
