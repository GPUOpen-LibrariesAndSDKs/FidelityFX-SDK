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

// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x4
// rr::F32x4x4
// rr::U32
// rr::U32x2
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Defines
//------------------------------------------------------------------------------

#if __cplusplus
#define ALIGN alignas(16u)
#define CONSTEXPR constexpr
#else  // __cplusplus
#define ALIGN
#define CONSTEXPR
#endif // __cplusplus

#define MAX_LIGHT_COUNT 128

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    #if __cplusplus
    inline constexpr U32 InvalidIndex = 0xFFFFFFFFu;
    #else  // __cplusplus
    static const     U32 InvalidIndex = 0xFFFFFFFFu;
    #endif // __cplusplus

    //--------------------------------------------------------------------------
    // DenoisingViewMode
    //--------------------------------------------------------------------------

    enum DenoisingViewMode : U32
    {
        // Composited output of the denoising.
        DenoisingViewMode_Denoised_Composited,
        DenoisingViewMode_Denoised_Composited_Direct,
        DenoisingViewMode_Denoised_Composited_DirectDiffuse,
        DenoisingViewMode_Denoised_Composited_DirectSpecular,
        DenoisingViewMode_Denoised_Composited_Indirect,
        DenoisingViewMode_Denoised_Composited_IndirectDiffuse,
        DenoisingViewMode_Denoised_Composited_IndirectSpecular,
        DenoisingViewMode_Denoised_Composited_DominantLightVisibility,

        // Raw, uncomposited output of the denoising.
        DenoisingViewMode_Denoised_Signal_DirectDiffuse,
        DenoisingViewMode_Denoised_Signal_DirectSpecular,
        DenoisingViewMode_Denoised_Signal_IndirectDiffuse,
        DenoisingViewMode_Denoised_Signal_IndirectSpecular,
        DenoisingViewMode_Denoised_Signal_DominantLightVisibility,
        DenoisingViewMode_Denoised_Signal_AmbientOcclusion,
        DenoisingViewMode_Denoised_Signal_SpecularOcclusion,

        // Composited output of the path tracing.
        DenoisingViewMode_Noisy_Composited,
        DenoisingViewMode_Noisy_Composited_Direct,
        DenoisingViewMode_Noisy_Composited_DirectDiffuse,
        DenoisingViewMode_Noisy_Composited_DirectSpecular,
        DenoisingViewMode_Noisy_Composited_Indirect,
        DenoisingViewMode_Noisy_Composited_IndirectDiffuse,
        DenoisingViewMode_Noisy_Composited_IndirectSpecular,
        DenoisingViewMode_Noisy_Composited_DominantLightVisibility,

        // Raw, uncomposited output of the path tracing.
        DenoisingViewMode_Noisy_Signal_DirectDiffuse,
        DenoisingViewMode_Noisy_Signal_DirectSpecular,
        DenoisingViewMode_Noisy_Signal_IndirectDiffuse,
        DenoisingViewMode_Noisy_Signal_IndirectSpecular,
        DenoisingViewMode_Noisy_Signal_DominantLightVisibility,
        DenoisingViewMode_Noisy_Signal_AmbientOcclusion,
        DenoisingViewMode_Noisy_Signal_SpecularOcclusion,

        DenoisingViewMode_Skip,

        DenoisingViewMode_DiffuseAlbedo,
        DenoisingViewMode_SpecularAlbedo,
        DenoisingViewMode_Roughness,

        DenoisingViewMode_LinearDepth,
        DenoisingViewMode_LinearDepthDelta,
        DenoisingViewMode_MotionVectors,
        DenoisingViewMode_Normals,

        DenoisingViewMode_Count
    };

    #if __cplusplus
    constexpr const char* DenoisingViewModeToString(DenoisingViewMode mode)
    {
        switch (mode)
        {

        case DenoisingViewMode_Denoised_Composited:
            return "Denoised Composited";
        case DenoisingViewMode_Denoised_Composited_Direct:
            return "Denoised Composited - Direct";
        case DenoisingViewMode_Denoised_Composited_DirectDiffuse:
            return "Denoised Composited - Direct Diffuse";
        case DenoisingViewMode_Denoised_Composited_DirectSpecular:
            return "Denoised Composited - Direct Specular";
        case DenoisingViewMode_Denoised_Composited_Indirect:
            return "Denoised Composited - Indirect";
        case DenoisingViewMode_Denoised_Composited_IndirectDiffuse:
            return "Denoised Composited - Indirect Diffuse";
        case DenoisingViewMode_Denoised_Composited_IndirectSpecular:
            return "Denoised Composited - Indirect Specular";
        case DenoisingViewMode_Denoised_Composited_DominantLightVisibility:
            return "Denoised Composited - Dominant Light Visibility";

         // Raw, uncomposited output of the denoising.
        case DenoisingViewMode_Denoised_Signal_DirectDiffuse:
            return "Denoised Signal - Direct Diffuse";
        case DenoisingViewMode_Denoised_Signal_DirectSpecular:
            return "Denoised Signal - Direct Specular";
        case DenoisingViewMode_Denoised_Signal_IndirectDiffuse:
            return "Denoised Signal - Indirect Diffuse";
        case DenoisingViewMode_Denoised_Signal_IndirectSpecular:
            return "Denoised Signal - Indirect Specular";
        case DenoisingViewMode_Denoised_Signal_DominantLightVisibility:
            return "Denoised Signal - Dominant Light Visibility";
        case DenoisingViewMode_Denoised_Signal_AmbientOcclusion:
            return "Denoised Signal - Ambient Occlusion";
        case DenoisingViewMode_Denoised_Signal_SpecularOcclusion:
            return "Denoised Signal - Specular Occlusion";

        // Composited output of the path tracing.
        case DenoisingViewMode_Noisy_Composited:
            return "Noisy Composited";
        case DenoisingViewMode_Noisy_Composited_Direct:
            return "Noisy Composited - Direct";
        case DenoisingViewMode_Noisy_Composited_DirectDiffuse:
            return "Noisy Composited - Direct Diffuse";
        case DenoisingViewMode_Noisy_Composited_DirectSpecular:
            return "Noisy Composited - Direct Specular";
        case DenoisingViewMode_Noisy_Composited_Indirect:
            return "Noisy Composited - Indirect";
        case DenoisingViewMode_Noisy_Composited_IndirectDiffuse:
            return "Noisy Composited - Indirect Diffuse";
        case DenoisingViewMode_Noisy_Composited_IndirectSpecular:
            return "Noisy Composited - Indirect Specular";
        case DenoisingViewMode_Noisy_Composited_DominantLightVisibility:
            return "Noisy Composited - Dominant Light Visibility";

        // Raw, uncomposited output of the path tracing.
        case DenoisingViewMode_Noisy_Signal_DirectDiffuse:
            return "Noisy Signal - Direct Diffuse";
        case DenoisingViewMode_Noisy_Signal_DirectSpecular:
            return "Noisy Signal - Direct Specular";
        case DenoisingViewMode_Noisy_Signal_IndirectDiffuse:
            return "Noisy Signal - Indirect Diffuse";
        case DenoisingViewMode_Noisy_Signal_IndirectSpecular:
            return "Noisy Signal - Indirect Specular";
        case DenoisingViewMode_Noisy_Signal_DominantLightVisibility:
            return "Noisy Signal - Dominant Light Visibility";
        case DenoisingViewMode_Noisy_Signal_AmbientOcclusion:
            return "Noisy Signal - Ambient Occlusion";
        case DenoisingViewMode_Noisy_Signal_SpecularOcclusion:
            return "Noisy Signal - Specular Occlusion";

        case DenoisingViewMode_Skip:
            return "Skip";

        case DenoisingViewMode_DiffuseAlbedo:
            return "Diffuse Albedo";
        case DenoisingViewMode_SpecularAlbedo:
            return "Specular Albedo";
        case DenoisingViewMode_Roughness:
            return "Roughness";

        case DenoisingViewMode_LinearDepth:
            return "Linear Depth";
        case DenoisingViewMode_LinearDepthDelta:
            return "Linear Depth Delta";
        case DenoisingViewMode_MotionVectors:
            return "Motion Vectors";
        case DenoisingViewMode_Normals:
            return "Normals";

        default:
            return {};
        }
    }
    #endif // __cplusplus

    //--------------------------------------------------------------------------
    // DenoisingDebugViewMode
    //--------------------------------------------------------------------------

    // Debug view overlays of Ray Regeneration itself. These are handled by the FFX
    // denoiser's own debug-view dispatch, not by the composition shader.
    enum DenoisingDebugViewMode : U32
    {
        DenoisingDebugViewMode_None,

        DenoisingDebugViewMode_MotionVectors,
        DenoisingDebugViewMode_MotionVectorsZ,
        DenoisingDebugViewMode_LinearDepth,
        DenoisingDebugViewMode_Normals,
        DenoisingDebugViewMode_WarpedConfidence,
        DenoisingDebugViewMode_HistoryUv,
        DenoisingDebugViewMode_WorldPosGrid,
        DenoisingDebugViewMode_VirtualWorldPosGrid,
        DenoisingDebugViewMode_UnetInputFeatures0,
        DenoisingDebugViewMode_UnetInputFeatures1,
        DenoisingDebugViewMode_UnetInputFeatures2,
        DenoisingDebugViewMode_CompositeRaw,
        DenoisingDebugViewMode_Overview,

        DenoisingDebugViewMode_Count
    };

    #if __cplusplus
    constexpr const char* DenoisingDebugViewModeToString(DenoisingDebugViewMode mode)
    {
        switch (mode)
        {

        case DenoisingDebugViewMode_None:
            return "Debug - None";
        case DenoisingDebugViewMode_MotionVectors:
            return "Debug - Motion Vectors";
        case DenoisingDebugViewMode_MotionVectorsZ:
            return "Debug - Motion Vectors Z";
        case DenoisingDebugViewMode_LinearDepth:
            return "Debug - Linear Depth";
        case DenoisingDebugViewMode_Normals:
            return "Debug - Normals";
        case DenoisingDebugViewMode_WarpedConfidence:
            return "Debug - Warped Confidence";
        case DenoisingDebugViewMode_HistoryUv:
            return "Debug - History UV";
        case DenoisingDebugViewMode_WorldPosGrid:
            return "Debug - World Position Grid";
        case DenoisingDebugViewMode_VirtualWorldPosGrid:
            return "Debug - Virtual World Position Grid";
        case DenoisingDebugViewMode_UnetInputFeatures0:
            return "Debug - UNet Input Features 0";
        case DenoisingDebugViewMode_UnetInputFeatures1:
            return "Debug - UNet Input Features 1";
        case DenoisingDebugViewMode_UnetInputFeatures2:
            return "Debug - UNet Input Features 2";
        case DenoisingDebugViewMode_CompositeRaw:
            return "Debug - Composite Raw";
        case DenoisingDebugViewMode_Overview:
            return "Debug - Overview";

        default:
            return {};
        }
    }
    #endif // __cplusplus

    //--------------------------------------------------------------------------
    // UpscalingMode
    //--------------------------------------------------------------------------

    enum UpscalingMode : U32
    {
        UpscalingMode_None,

        UpscalingMode_FSR_Native,
        UpscalingMode_FSR_Quality,
        UpscalingMode_FSR_Balanced,
        UpscalingMode_FSR_Performance,
        UpscalingMode_FSR_UltraPerformance,

        UpscalingMode_Count
    };

    #if __cplusplus
    constexpr const char* UpscalingModeToString(UpscalingMode mode)
    {
        switch (mode)
        {

        case UpscalingMode_None:
            return "None";
        case UpscalingMode_FSR_Native:
            return "FSR: Native";
        case UpscalingMode_FSR_Quality:
            return "FSR: Quality";
        case UpscalingMode_FSR_Balanced:
            return "FSR: Balanced";
        case UpscalingMode_FSR_Performance:
            return "FSR: Performance";
        case UpscalingMode_FSR_UltraPerformance:
            return "FSR: Ultra Performance";
        default:
            return {};
        }
    }
    #endif // __cplusplus

    CONSTEXPR F32 UpscalingModeToUpscaleRatio(UpscalingMode mode)
    {
        switch (mode)
        {

        case UpscalingMode_None:
        case UpscalingMode_FSR_Native:
            return 1.0f;
        case UpscalingMode_FSR_Quality:
            return 1.5f;
        case UpscalingMode_FSR_Balanced:
            return 1.7f;
        case UpscalingMode_FSR_Performance:
            return 2.0f;
        case UpscalingMode_FSR_UltraPerformance:
            return 3.0f;
        default:
            return 0.0f;
        }
    }

    CONSTEXPR U32x2 UpscaleSizeToRenderSize(UpscalingMode mode, U32x2 upscaleSize)
    {
        const F32 upscaleRatio = UpscalingModeToUpscaleRatio(mode);

        #if __cplusplus
        return U32x2
        {
            static_cast< U32 >(upscaleSize[0u] / upscaleRatio),
            static_cast< U32 >(upscaleSize[1u] / upscaleRatio)
        };
        #else  // __cplusplus
        return U32x2(upscaleSize / upscaleRatio);
        #endif // __cplusplus
    }

    //--------------------------------------------------------------------------
    // ToneMappingMode
    //--------------------------------------------------------------------------

    enum ToneMappingMode : U32
    {
        ToneMappingMode_None,
        ToneMappingMode_Timothy,
        ToneMappingMode_DX11DSK,
        ToneMappingMode_Reinhard,
        ToneMappingMode_Uncharted2,
        ToneMappingMode_AcesFilm
    };

    #if __cplusplus
    constexpr const char* ToneMappingModeToString(ToneMappingMode mode)
    {
        switch (mode)
        {

        case ToneMappingMode_None:
            return "None";
        case ToneMappingMode_Timothy:
            return "Timothy";
        case ToneMappingMode_DX11DSK:
            return "DX11 DSK";
        case ToneMappingMode_Reinhard:
            return "Reinhard";
        case ToneMappingMode_Uncharted2:
            return "Uncharted 2";
        case ToneMappingMode_AcesFilm:
            return "ACES Film";
        default:
            return {};
        }
    }
    #endif // __cplusplus

    //--------------------------------------------------------------------------
    // ColorSpace
    //--------------------------------------------------------------------------

    enum ColorSpace : U32
    {
        ColorSpace_None,
        ColorSpace_Rec2020
    };

    #if __cplusplus
    constexpr const char* ColorSpaceToString(ColorSpace colorSpace)
    {
        switch (colorSpace)
        {

        case ColorSpace_None:
            return "None";
        case ColorSpace_Rec2020:
            return "Rec2020";
        default:
            return {};
        }
    }
    #endif // __cplusplus

    //--------------------------------------------------------------------------
    // PackedCamera
    //--------------------------------------------------------------------------

    struct ALIGN PackedCamera
    {
        F32x4x4 worldToClip;
        F32x4x4 clipToWorld;
        F32x4x4 viewToClip;
        F32x4x4 clipToView;
        F32x4x4 worldToView;
        F32x4x4 viewToWorld;

        F32x4x4 worldToClipPrev;
        F32x4x4 clipToWorldPrev;
        F32x4x4 viewToClipPrev;
        F32x4x4 clipToViewPrev;
        F32x4x4 worldToViewPrev;
        F32x4x4 viewToWorldPrev;

        F32 aspectRatio;
        F32 nearView;
        F32 farView;
        F32 fovAngleVertical;

        F32x2 jitterOffsets;
        F32x2 padding;

        U32 usesInfiniteFarPlane;
        U32 usesReversedZ;
        U32x2 padding0;
    };

    //--------------------------------------------------------------------------
    // PackedLight
    //--------------------------------------------------------------------------

    enum LightType : U32
    {
        LightType_Directional,
        LightType_Spot,
        LightType_Point
    };

    struct ALIGN PackedLight
    {
        F32x3 dWorld;
        F32 maxDistance;

        F32x3 color;
        F32 intensity;

        F32x3 pWorld;
        F32 padding0;

        F32 cosAngularRadius;
        F32 cosInnerCone;
        F32 cosOuterCone;
        LightType type;
    };

    //--------------------------------------------------------------------------
    // PackedLighting
    //--------------------------------------------------------------------------

    struct ALIGN PackedLighting
    {
        U32 lightCount;
        F32 iblFactor;
        F32x2 padding0;

        PackedLight dominantLight;
        PackedLight lights[MAX_LIGHT_COUNT];
    };

    //--------------------------------------------------------------------------
    // PackedMaterial
    //--------------------------------------------------------------------------

    enum MaterialFlags : U32
    {
        MaterialFlag_None        = 0u,
        MaterialFlag_Opaque      = 1u << 0u,
        MaterialFlag_DoubleSided = 1u << 1u,
    };

    struct Map
    {
        U32 samplerIndex;
        U32 textureIndex;
    };

    struct ALIGN PackedMaterial
    {
        F32x4 baseColorFactor;

        F32 alphaCutoff;
        F32 ambientOcclusionFactor;
        F32 roughnessFactor;
        F32 metalnessFactor;

        F32x3 emissionFactor;
        U32 flags;

        Map ambientOcclusionRoughnessMetalness;
        Map baseColor;

        Map emission;
        Map normal;
    };

    //--------------------------------------------------------------------------
    // PackedSurface
    //--------------------------------------------------------------------------

    struct ALIGN PackedSurface
    {
        U32 materialIndex;
        U32 indexCount;
        U32 indexByteCount;
        U32 vertexCount;

        U32 ibvIndex;
        U32 positionVbvIndex;
        U32 normalVbvIndex;
        U32 tangentVbvIndex;

        U32 texcoord0VbvIndex;
        U32 texcoord1VbvIndex;
        U32 jointIndicesVbvIndex;
        U32 jointWeightsVbvIndex;
    };

    //--------------------------------------------------------------------------
    // PackedInstance
    //--------------------------------------------------------------------------

    struct ALIGN PackedInstance
    {
        U32 nodeIndex;
        U32 meshIndex;
        U32 surfaceOffset;
        U32 surfaceCount;
    };

    //--------------------------------------------------------------------------
    // PackedFrame
    //--------------------------------------------------------------------------

    struct ALIGN PackedFrame
    {
        U32x2 renderSize;
        U32x2 upscaleSize;

        F32x2 invRenderSize;
        F32x2 invUpscaleSize;

        F32x3 motionVectorScale;
        F32 deltaTime;

        U32 frameIndex;
        F32 exposure;
        U32 checkerboard;
        U32 debugging;

        F32x4 viewModeWeights;

        // Split-screen comparison: when viewModeRight differs from viewModeLeft, render
        // viewModeLeft on the left and viewModeRight on the right of a vertical divider at
        // viewModeSplit (in [0, 1] of render width). Equal modes disable the split.
        DenoisingViewMode viewModeLeft;
        DenoisingViewMode viewModeRight;
        F32 viewModeSplit;
        DenoisingDebugViewMode debugViewMode;

        UpscalingMode upscalingMode;
        ToneMappingMode toneMappingMode;
        ColorSpace colorSpace;
        U32 padding1;

        // Hit distance to AO/SO parameters
        F32 ambientOcclusionA;
        F32 ambientOcclusionB;
        F32 specularOcclusionA;
        F32 specularOcclusionB;

        PackedCamera camera;
        PackedLighting lighting;
    };
}

//------------------------------------------------------------------------------
// Sample Defines
//------------------------------------------------------------------------------

#undef ALIGN
#undef CONSTEXPR
