// Portions Copyright 2023 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This shader code was ported from https://github.com/KhronosGroup/glTF-WebGL-PBR
// All credits should go to his original author.

//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
//

#include "surfacerendercommon.h"

//////////////////////////////////////////////////////////////////////////
// Resources
//////////////////////////////////////////////////////////////////////////

Texture2D AllTextures[]    : register(t0);
SamplerState AllSamplers[] : register(s0);

cbuffer CBSceneInformation : register(b0)
{
    SceneInformation SceneInfo;
};
cbuffer CBInstanceInformation : register(b1)
{
    InstanceInformation InstanceInfo;
};
cbuffer CBTextureIndices : register(b2)
{
    TextureIndices Textures;
}

// Defines
// ID_albedoTexture                 - Has albedo texture
// ID_metallicRoughnessTexture      - Has metalrough texture
// ID_specularGlossinessTexture     - Has specgloss texture
// ID_normalTexture                 - Has normal texture

// Not yet supported
// ID_emissiveTexture               - Has emissive texture
// ID_occlusionTexture              - Had occlusion texture

struct GBufferOutput
{
    float4 BaseColorAlpha       : SV_Target0;
    float4 Normals              : SV_Target1;
    float4 AoRoughnessMetallic  : SV_Target2;
#ifdef HAS_MOTION_VECTORS_RT
    float2 MotionVectors : TARGET(HAS_MOTION_VECTORS_RT);
#endif // HAS_MOTION_VECTORS_RT
};

GBufferOutput MainPS(VS_SURFACE_OUTPUT SurfaceInput
#ifdef ID_doublesided
    , bool isFrontFace : SV_IsFrontFace
#endif
)
{
#ifndef ID_doublesided
    bool isFrontFace = false;
#endif
#ifdef HAS_COLOR_0
    if (SurfaceInput.Color0.a <= 0)
        discard;
#endif
    float4 BaseColorAlpha;
    float3 AoRoughnessMetallic;
    GetPBRParams(SurfaceInput, InstanceInfo.MaterialInfo, BaseColorAlpha, AoRoughnessMetallic, Textures, AllTextures, AllSamplers, SceneInfo.MipLODBias);

    DiscardPixelIfAlphaCutOff(BaseColorAlpha.a, InstanceInfo);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    AoRoughnessMetallic.g *= AoRoughnessMetallic.g;

    GBufferOutput GBuffer;
#ifdef HAS_MOTION_VECTORS_RT
    float2 cancelJitter = SceneInfo.CameraInfo.PrevJitter - SceneInfo.CameraInfo.CurrJitter;
    GBuffer.MotionVectors = (SurfaceInput.PrevPosition.xy / SurfaceInput.PrevPosition.w) -
                            (SurfaceInput.CurPosition.xy / SurfaceInput.CurPosition.w) + cancelJitter;

    // Transform motion vectors from NDC space to UV space (+Y is top-down).
    GBuffer.MotionVectors *= float2(0.5f, -0.5f);
#endif // HAS_MOTION_VECTORS_RT

    GBuffer.BaseColorAlpha      = BaseColorAlpha;
    GBuffer.AoRoughnessMetallic = float4(AoRoughnessMetallic, 0.f);
    GBuffer.AoRoughnessMetallic.r = 1.0f; // Temp for SSAO

    float3 normals = GetPixelNormal(SurfaceInput, Textures, SceneInfo, AllTextures, AllSamplers, SceneInfo.MipLODBias, isFrontFace);
    // Compress normal range from [-1, 1] to [0, 1] to fit in unsigned R11G11B10 format    
    GBuffer.Normals             = float4(normals * 0.5 + 0.5, 0.f);

    return GBuffer;
}
