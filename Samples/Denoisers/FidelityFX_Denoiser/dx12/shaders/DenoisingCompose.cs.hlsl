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

// DECLARE_CBV_BINDING
// DECLARE_SRV_BINDING
// DECLARE_UAV_BINDING
#include "Bindings.hlsli"
// rr::HsvToRgb
#include "Color.hlsli"
// GROUP_THREAD_COUNT_X
// GROUP_THREAD_COUNT_Y
// GROUP_THREAD_COUNT_Z
#include "DenoisingData.h"
// rr::DispatchInput
#include "Dispatch.hlsli"
// rr::EvaluateDirectionalLight
// rr::LightThroughput
#include "Lighting.hlsli"
// rr::CreateMaterial
// rr::Material
// rr::PathThroughputMode
#include "Material.hlsli"
// rr::g_fp16_max
// rr::g_pi
// rr::SNormToUNorm
// rr::Square
#include "Numeric.hlsli"
// rr::DenoisingViewMode
// rr::PackedCamera
// rr::PackedFrame
// rr::PackedLight
// rr::PackedLighting
#include "PackedData.h"
// rr::NdcSpaceToWorldSpace
// rr::ScreenSpaceToNdcSpace
#include "Transformation.hlsli"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x4
// rr::U32x2
#include "Types.h"
// rr::DecodeUnitVector_Octahedron
#include "UnitVector.hlsli"

//------------------------------------------------------------------------------
// Sample Bindings
//------------------------------------------------------------------------------

DECLARE_CBV_BINDING(0, rr::PackedFrame, g_frame);

DECLARE_SRV_BINDING( 0, Texture2D< rr::F32x3 >, g_denoisedDirectSpecular);
DECLARE_SRV_BINDING( 1, Texture2D< rr::F32x3 >, g_denoisedDirectDiffuse);
DECLARE_SRV_BINDING( 2, Texture2D< rr::F32x3 >, g_denoisedIndirectSpecular);
DECLARE_SRV_BINDING( 3, Texture2D< rr::F32x3 >, g_denoisedIndirectDiffuse);
DECLARE_SRV_BINDING( 4, Texture2D< rr::F32 >,   g_denoisedDominantLightVisibility);
DECLARE_SRV_BINDING( 5, Texture2D< rr::F32 >,   g_denoisedAmbientOcclusion);
DECLARE_SRV_BINDING( 6, Texture2D< rr::F32 >,   g_denoisedSpecularOcclusion);

DECLARE_SRV_BINDING( 7, Texture2D< rr::F32x3 >, g_noisyDirectSpecular);
DECLARE_SRV_BINDING( 8, Texture2D< rr::F32x3 >, g_noisyDirectDiffuse);
DECLARE_SRV_BINDING( 9, Texture2D< rr::F32x3 >, g_noisyIndirectSpecular);
DECLARE_SRV_BINDING(10, Texture2D< rr::F32x3 >, g_noisyIndirectDiffuse);
DECLARE_SRV_BINDING(11, Texture2D< rr::F32 >,   g_noisyDominantLightHitDistance);
DECLARE_SRV_BINDING(12, Texture2D< rr::F32 >,   g_noisyAmbientOcclusion);
DECLARE_SRV_BINDING(13, Texture2D< rr::F32 >,   g_noisySpecularOcclusion);

DECLARE_SRV_BINDING(14, Texture2D< rr::F32x3 >, g_skip);

DECLARE_SRV_BINDING(15, Texture2D< rr::F32x3 >, g_diffuseAlbedo);
DECLARE_SRV_BINDING(16, Texture2D< rr::F32x3 >, g_specularAlbedo);

DECLARE_SRV_BINDING(17, Texture2D< rr::F32 >,   g_linearDepth);
DECLARE_SRV_BINDING(18, Texture2D< rr::F32x3 >, g_motionVectors);
DECLARE_SRV_BINDING(19, Texture2D< rr::F32x4 >, g_normals);

DECLARE_UAV_BINDING(0, RWTexture2D< rr::F32x4 >, g_composed);

#define ROOT_SIGNATURE                                                         \
    "RootFlags(0),"                                                            \
    /* ---- CBVs ---- */                                                       \
    "CBV(b0, visibility = SHADER_VISIBILITY_ALL)," /* g_frame */               \
    /* ---- SRVs ---- */                                                       \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        "SRV(t0),"  /* g_denoisedDirectSpecular */                             \
        "SRV(t1),"  /* g_denoisedDirectDiffuse */                              \
        "SRV(t2),"  /* g_denoisedIndirectSpecular */                           \
        "SRV(t3),"  /* g_denoisedIndirectDiffuse */                            \
        "SRV(t4),"  /* g_denoisedDominantLightVisibility */                    \
        "SRV(t5),"  /* g_denoisedAmbientOcclusion */                           \
        "SRV(t6),"  /* g_denoisedSpecularOcclusion */                          \
        "SRV(t7),"  /* g_noisyDirectSpecular */                                \
        "SRV(t8),"  /* g_noisyDirectDiffuse */                                 \
        "SRV(t9),"  /* g_noisyIndirectSpecular */                              \
        "SRV(t10)," /* g_noisyIndirectDiffuse */                               \
        "SRV(t11)," /* g_noisyDominantLightHitDistance */                      \
        "SRV(t12)," /* g_noisyAmbientOcclusion */                              \
        "SRV(t13)," /* g_noisySpecularOcclusion */                             \
        "SRV(t14)," /* g_skip */                                               \
        "SRV(t15)," /* g_diffuseAlbedo */                                      \
        "SRV(t16)," /* g_specularAlbedo */                                     \
        "SRV(t17)," /* g_linearDepth */                                        \
        "SRV(t18)," /* g_motionVectors */                                      \
        "SRV(t19)"  /* g_normals */                                            \
    "),"                                                                       \
    /* ---- UAVs ---- */                                                       \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        "UAV(u0)"  /* g_composed */                                            \
    ")"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    F32x3 LinearDepthDebugColor(U32x2 pixel, F32 linearDepth)
    {
        const F32 normalized = saturate((abs(linearDepth) - g_frame.camera.nearView)
                             / (g_frame.camera.farView - g_frame.camera.nearView));

        return normalized.xxx;
    }

    F32x3 LinearDepthDeltaDebugColor(U32x2 pixel, F32 linearDepthDelta)
    {
        linearDepthDelta *= g_frame.motionVectorScale.z;

        if (0.0f <= linearDepthDelta)
        {
            return F32x3(linearDepthDelta, 0.0f, 0.0f);
        }
        else
        {
            return F32x3(0.0f, 0.0f, -linearDepthDelta);
        }
    }

    F32x3 MotionVectorDebugColor(U32x2 pixel, F32x2 motionVector)
    {
        const F32x2 velocity = motionVector
                             * g_frame.motionVectorScale.xy * g_frame.renderSize;

        static const F32 MotionVectorHsvSaturationDistance = 16.0f;

        const F32 r = tanh(length(velocity) / MotionVectorHsvSaturationDistance);
        const F32 theta = smoothstep(-g_pi, g_pi, atan2(velocity.y, velocity.x));
        const F32x3 hsv = F32x3(theta, 1.0f, r);
        const F32x3 rgb = HsvToRgb(hsv);

        F32x3 result = rgb;

        {
            const U32x2 blockIndex  = pixel / MotionVectorHsvSaturationDistance;
            const F32x2 blockCenter = (blockIndex + 0.5f) * MotionVectorHsvSaturationDistance;

            const F32x2 blockCenterVelocity = g_motionVectors[clamp(blockCenter, 0u, g_frame.renderSize - 1u)].xy
                                            * g_frame.motionVectorScale.xy * g_frame.renderSize;

            // Highlight lines of motion in white.
            if (abs(distance(blockCenter, blockCenter + blockCenterVelocity) - (distance(blockCenter, pixel) + distance(blockCenter + blockCenterVelocity, pixel))) < 0.025f)
            {
                result = F32x3(1.0f, 1.0f, 1.0f);
            }
            // Color the block center in red.
            if (all(U32x2(blockCenter) == pixel))
            {
                result = F32x3(1.0f, 0.0f, 0.0f);
            }
        }

        return result;
    }

    F32x3 NormalDebugColor(U32x2 pixel, F32x3 nWorld)
    {
        return SNormToUNorm(nWorld);
    }

    void Main(DispatchInput input)
    {
        const U32x2 pixel = input.dispatchThreadId.xy;
        if (any(g_frame.renderSize <= pixel))
        {
            return;
        }

        // A different right view mode enables the split; equal view modes show a single view mode.
        const bool split  = (g_frame.viewModeLeft != g_frame.viewModeRight);
        const F32  splitX = g_frame.viewModeSplit * F32(g_frame.renderSize.x);

        const DenoisingViewMode viewMode = (split && (F32(pixel.x) >= splitX))
                                         ? g_frame.viewModeRight
                                         : g_frame.viewModeLeft;

        // 1-1 to PathTracing.cs.hlsl
        const F32x2 uv        = (F32x2(pixel) + 0.5f) * g_frame.invRenderSize;
        const F32x2 pNdc      = ScreenSpaceToNdcSpace(uv);
        const F32x3 cWorld    = mul(g_frame.camera.viewToWorld, F32x4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
        const F32x3 nearWorld = NdcSpaceToWorldSpace(F32x3(pNdc, g_frame.camera.usesReversedZ ? 1.0f : 0.0f), g_frame.camera.clipToWorld);
        const F32x3 vWorld    = normalize(cWorld - nearWorld);

        //----------------------------------------------------------------------

        const F32x3 denoisedDirectSpecular          = g_denoisedDirectSpecular[pixel];
        const F32x3 denoisedDirectDiffuse           = g_denoisedDirectDiffuse[pixel];
        const F32x3 denoisedIndirectSpecular        = g_denoisedIndirectSpecular[pixel];
        const F32x3 denoisedIndirectDiffuse         = g_denoisedIndirectDiffuse[pixel];
        const F32   denoisedDominantLightVisibility = g_denoisedDominantLightVisibility[pixel];
        const F32   denoisedAmbientOcclusion        = g_denoisedAmbientOcclusion[pixel];
        const F32   denoisedSpecularOcclusion       = g_denoisedSpecularOcclusion[pixel];

        const F32x3 noisyDirectSpecular             = g_noisyDirectSpecular[pixel];
        const F32x3 noisyDirectDiffuse              = g_noisyDirectDiffuse[pixel];
        const F32x3 noisyIndirectSpecular           = g_noisyIndirectSpecular[pixel];
        const F32x3 noisyIndirectDiffuse            = g_noisyIndirectDiffuse[pixel];
        const F32   noisyDominantLightVisibility    = (g_noisyDominantLightHitDistance[pixel] < g_fp16_max) ? 0.0f : 1.0f;
        const F32   noisyAmbientOcclusion           = g_noisyAmbientOcclusion[pixel];
        const F32   noisySpecularOcclusion          = g_noisySpecularOcclusion[pixel];

        const F32x3 skip                            = g_skip[pixel];

        // sqrt decoding
        const F32x3 diffuseAlbedo                   = Square(g_diffuseAlbedo[pixel]);
        const F32x3 specularAlbedo                  = Square(g_specularAlbedo[pixel]);

        const F32 linearDepth                       = g_linearDepth[pixel];
        const F32x3 motionVectors                   = g_motionVectors[pixel];
        const F32x4 normalRoughnessMaterial         = g_normals[pixel];
        // octahedron decoding
        const F32x3 nWorld                          = DecodeUnitVector_Octahedron(normalRoughnessMaterial.xy);

        const Material material = CreateMaterial(0.0f /* unused */,
                                                 normalRoughnessMaterial.z,
                                                 0.0f /* unused */);

        //----------------------------------------------------------------------

        F32x3 denoisedDominantLightSpecular;
        F32x3 denoisedDominantLightDiffuse;
        F32x3 noisyDominantLightSpecular;
        F32x3 noisyDominantLightDiffuse;
        if (0.0f < g_frame.lighting.dominantLight.intensity)
        {
            // 1-1 to PathTracing.cs.hlsl
            const LightThroughput throughput = EvaluateDirectionalLight< PathThroughputMode_Direct >(g_frame.lighting.dominantLight, material, nWorld, vWorld);

            denoisedDominantLightSpecular = throughput.specularRadiance * denoisedDominantLightVisibility;
            denoisedDominantLightDiffuse  = throughput.diffuseRadiance  * denoisedDominantLightVisibility;
            noisyDominantLightSpecular    = throughput.specularRadiance * noisyDominantLightVisibility;
            noisyDominantLightDiffuse     = throughput.diffuseRadiance  * noisyDominantLightVisibility;
        }
        else
        {
            denoisedDominantLightSpecular = 0.0f;
            denoisedDominantLightDiffuse  = 0.0f;
            noisyDominantLightSpecular    = 0.0f;
            noisyDominantLightDiffuse     = 0.0f;
        }

        //----------------------------------------------------------------------
        // Background
        //----------------------------------------------------------------------

        #define COMPOSED                                                       \
             (((directDiffuse  + indirectDiffuse)  * diffuseAlbedo)            \
            + ((directSpecular + indirectSpecular) * specularAlbedo))

        F32x3 background;
        switch (viewMode)
        {

        case DenoisingViewMode_Denoised_Composited:
        {
            const F32x3 directDiffuse    = denoisedDirectDiffuse   + denoisedDominantLightDiffuse;
            const F32x3 directSpecular   = denoisedDirectSpecular  + denoisedDominantLightSpecular;
            const F32x3 indirectDiffuse  = denoisedIndirectDiffuse;
            const F32x3 indirectSpecular = denoisedIndirectSpecular;

            background = COMPOSED;
            background += skip;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_Direct:
        {
            const F32x3 directDiffuse    = denoisedDirectDiffuse   + denoisedDominantLightDiffuse;
            const F32x3 directSpecular   = denoisedDirectSpecular  + denoisedDominantLightSpecular;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_DirectDiffuse:
        {
            const F32x3 directDiffuse    = denoisedDirectDiffuse   + denoisedDominantLightDiffuse;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_DirectSpecular:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = denoisedDirectSpecular  + denoisedDominantLightSpecular;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_Indirect:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = denoisedIndirectDiffuse;
            const F32x3 indirectSpecular = denoisedIndirectSpecular;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_IndirectDiffuse:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = denoisedIndirectDiffuse;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_IndirectSpecular:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = denoisedIndirectSpecular;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Composited_DominantLightVisibility:
        {
            const F32x3 directDiffuse    = denoisedDominantLightDiffuse;
            const F32x3 directSpecular   = denoisedDominantLightSpecular;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_DirectDiffuse:
        {
            background = denoisedDirectDiffuse;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_DirectSpecular:
        {
            background = denoisedDirectSpecular;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_IndirectDiffuse:
        {
            background = denoisedIndirectDiffuse;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_IndirectSpecular:
        {
            background = denoisedIndirectSpecular;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_DominantLightVisibility:
        {
            background = denoisedDominantLightVisibility.xxx;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_AmbientOcclusion:
        {
            background = denoisedAmbientOcclusion.xxx;
            break;
        }
        case DenoisingViewMode_Denoised_Signal_SpecularOcclusion:
        {
            background = denoisedSpecularOcclusion.xxx;
            break;
        }
        case DenoisingViewMode_Noisy_Composited:
        {
            const F32x3 directDiffuse    = noisyDirectDiffuse   + noisyDominantLightDiffuse;
            const F32x3 directSpecular   = noisyDirectSpecular  + noisyDominantLightSpecular;
            const F32x3 indirectDiffuse  = noisyIndirectDiffuse;
            const F32x3 indirectSpecular = noisyIndirectSpecular;

            background = COMPOSED;
            background += skip;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_Direct:
        {
            const F32x3 directDiffuse    = noisyDirectDiffuse   + noisyDominantLightDiffuse;
            const F32x3 directSpecular   = noisyDirectSpecular  + noisyDominantLightSpecular;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_DirectDiffuse:
        {
            const F32x3 directDiffuse    = noisyDirectDiffuse   + noisyDominantLightDiffuse;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_DirectSpecular:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = noisyDirectSpecular  + noisyDominantLightSpecular;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_Indirect:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = noisyIndirectDiffuse;
            const F32x3 indirectSpecular = noisyIndirectSpecular;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_IndirectDiffuse:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = noisyIndirectDiffuse;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_IndirectSpecular:
        {
            const F32x3 directDiffuse    = 0.0f;
            const F32x3 directSpecular   = 0.0f;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = noisyIndirectSpecular;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Composited_DominantLightVisibility:
        {
            const F32x3 directDiffuse    = noisyDominantLightDiffuse;
            const F32x3 directSpecular   = noisyDominantLightSpecular;
            const F32x3 indirectDiffuse  = 0.0f;
            const F32x3 indirectSpecular = 0.0f;

            background = COMPOSED;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_DirectDiffuse:
        {
            background = noisyDirectDiffuse;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_DirectSpecular:
        {
            background = noisyDirectSpecular;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_IndirectDiffuse:
        {
            background = noisyIndirectDiffuse;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_IndirectSpecular:
        {
            background = noisyIndirectSpecular;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_DominantLightVisibility:
        {
            background = noisyDominantLightVisibility.xxx;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_AmbientOcclusion:
        {
            background = noisyAmbientOcclusion.xxx;
            break;
        }
        case DenoisingViewMode_Noisy_Signal_SpecularOcclusion:
        {
            background = noisySpecularOcclusion.xxx;
            break;
        }
        case DenoisingViewMode_Skip:
        {
            background = skip;
            break;
        }
        case DenoisingViewMode_DiffuseAlbedo:
        {
            background = diffuseAlbedo;
            break;
        }
        case DenoisingViewMode_SpecularAlbedo:
        {
            background = specularAlbedo;
            break;
        }
        case DenoisingViewMode_Roughness:
        {
            background = material.perceptualRoughness;
            break;
        }
        case DenoisingViewMode_LinearDepth:
        {
            background = LinearDepthDebugColor(pixel, linearDepth);
            break;
        }
        case DenoisingViewMode_LinearDepthDelta:
        {
            background = LinearDepthDeltaDebugColor(pixel, motionVectors.z);
            break;
        }
        case DenoisingViewMode_MotionVectors:
        {
            background = MotionVectorDebugColor(pixel, motionVectors.xy);
            break;
        }
        case DenoisingViewMode_Normals:
        {
            background = NormalDebugColor(pixel, nWorld.xyz);
            break;
        }
        default:
        {
            background = 0.0f;
            break;
        }

        }

        #undef COMPOSED

        //----------------------------------------------------------------------
        // Split-screen divider
        //----------------------------------------------------------------------

        // Draw the divider line in magenta.
        static const F32 DividerHalfWidth = 1.0f;
        if (split && (abs(F32(pixel.x) - splitX) < DividerHalfWidth))
        {
            background = F32x3(1.0f, 0.0f, 1.0f);
        }

        //----------------------------------------------------------------------

        g_composed[pixel] = g_frame.viewModeWeights * F32x4(background, 1.0f);
    }
}

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_THREAD_COUNT_X, GROUP_THREAD_COUNT_Y, GROUP_THREAD_COUNT_Z)]
void Main(rr::DispatchInput input)
{
    rr::Main(input);
}
