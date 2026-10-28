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

// rr::DenoisingViewMode
// rr::PackedFrame
#include "shaders/PackedData.h"
// rr::F32
// rr::S32
// rr::U32
#include "shaders/Types.h"

//------------------------------------------------------------------------------
// Cauldron Includes
//------------------------------------------------------------------------------

// json
#include "Cauldron2/framework/core/framework.h"
// cauldron::Vec3
#include "Cauldron2/framework/misc/math.h"
// cauldron::ResolutionInfo
#include "Cauldron2/framework/render/renderdefines.h"
// cauldron::CommandList
// cauldron::RenderModule
#include "Cauldron2/framework/render/rendermodule.h"
// cauldron::SamplerDesc
#include "Cauldron2/framework/render/sampler.h"
// SceneLightingInformation
#include "Cauldron2/framework/shaders/shadercommon.h"

//------------------------------------------------------------------------------
// FidelityFX Includes
//------------------------------------------------------------------------------

// ffx::Context
#include <FidelityFX/api/include/ffx_api.hpp>
// FfxApiFloatBounds
#include <FidelityFX/api/include/ffx_api_types.h>
// FfxApiConfigureDenoiserKey
// FfxApiDenoiserDebugViewMode
#include <FidelityFX/denoisers/include/ffx_denoiser.hpp>

//------------------------------------------------------------------------------
// External Includes
//------------------------------------------------------------------------------

// std::uint64_t
#include <cstdint>
// std::vector
#include <vector>

//------------------------------------------------------------------------------
// Cauldron Forward Declarations
//------------------------------------------------------------------------------

namespace cauldron
{
    class CameraComponent;
    class Entity;
    class ParameterSet;
    class ResourceView;
    class RootSignature;
    class Texture;
    class UIRenderModule;
    class UISection;
}

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    // Builds the per-frame PackedFrame shared by the path tracer and denoiser: camera matrices,
    // render sizes, frame index, reversed-Z flag, and the full light set (dir/spot/point) plus the
    // dominant directional light. Callers set module-specific fields (viewModeLeft, viewModeRight,
    // viewModeWeights, AO/SO params, iblFactor) after.
    [[nodiscard]]
    PackedFrame CreatePackedFrame(const cauldron::ResolutionInfo& resInfo,
                                  cauldron::CameraComponent* pCamera,
                                  bool usesReversedZ,
                                  U32 frameIndex,
                                  const SceneLightingInformation& sceneLightInfo,
                                  bool useDominantLight,
                                  U32 dominantLightIndex);

    //--------------------------------------------------------------------------
    // DenoiserRenderModule
    //--------------------------------------------------------------------------
    
    class DenoiserRenderModule final : public cauldron::RenderModule
    {

    public:

        //----------------------------------------------------------------------
        // Member Methods: Constructors and Destructors
        //----------------------------------------------------------------------

        [[nodiscard]]
        DenoiserRenderModule();

        ~DenoiserRenderModule() override;

        //----------------------------------------------------------------------
        // Member Methods
        //----------------------------------------------------------------------
        
        void Init(const json& initData);
        
        void EnableModule(bool enabled) override;
        
        void OnPreFrame() override;

        /**
         * @brief   Setup parameters that the Denoiser context needs this frame and then call the FFX Dispatch.
         */
        void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

        /**
         * @brief   Recreate the Denoiser context to resize internal resources. Called by the framework when the resolution changes.
         */
        void OnResize(const cauldron::ResolutionInfo& resInfo) override;

        /**
         * Update the Debug Option UI element.
         */
        void UpdateUI(double deltaTime) override;

        /**
         * @brief   Build UI.
         */
        void BuildUI();

        /**
         * @brief   Returns whether or not Denoiser requires sample-side re-initialization.
         */
        [[nodiscard]]
        bool NeedsReInit() const
        {
            return m_NeedReInit;
        }

        /**
         * @brief   Clears Denoiser re-initialization flag.
         */
        void ClearReInit()
        {
            m_NeedReInit = false;
        }

        [[nodiscard]]
        bool UseDominantLightVisibility() const { return m_EnableDominantLightVisibility; }
       
        [[nodiscard]]
        bool IsDebugViewEnabled() const { return m_EnableDebugging and m_ShowDebugView; }
        
        [[nodiscard]]
        bool IsCheckerboardingEnabled() const { return m_EnableCheckerboarding; }

        void ShowUI(bool show);

        /**
         * @brief   Toggles dominant light visibility denoising via hotkey.
         */
        void ToggleDominantLightVisibilityHotkey()
        {
            if (m_DenoiserAvailable)
            {
                m_EnableDominantLightVisibility = not m_EnableDominantLightVisibility;
                m_NeedReInit = true;
            }
        }

        /**
         * @brief   Cycles through view modes via hotkey.
         */
        void CycleViewModeHotkey()
        {
            if (m_DenoiserAvailable and (not m_ViewModeValues.empty()))
            {
                m_ViewModeLeft = (m_ViewModeLeft + 1) % static_cast< S32 >(m_ViewModeValues.size());
            }
        }

        /**
         * @brief   Cycles through RGBA channel combinations via hotkey.
         */
        void CycleRGBAChannelsHotkey()
        {
            if (m_DenoiserAvailable)
            {
                // Treat the 4 booleans as a 4-bit number and increment
                // R=bit0, G=bit1, B=bit2, A=bit3
                static U32 channelMask = 0b1111; // Start with all channels enabled (RGBA)

                // Cycle through all 16 combinations (0b0000 to 0b1111)
                channelMask = (channelMask + 1) & 0xF;

                // Update individual channel flags
                m_DebugShowChannelR = (channelMask & 0b0001) != 0;
                m_DebugShowChannelG = (channelMask & 0b0010) != 0;
                m_DebugShowChannelB = (channelMask & 0b0100) != 0;
                m_DebugShowChannelA = (channelMask & 0b1000) != 0;
            }
        }

    private:

        //----------------------------------------------------------------------
        // Friends
        //----------------------------------------------------------------------

        friend class DenoiserSettingsHandler;

        //----------------------------------------------------------------------
        // Member Methods
        //----------------------------------------------------------------------

        [[nodiscard]]
        S32 RemapViewMode(S32 mode);

        bool CreateDenoiserContext();
        bool CreateMainDenoiserContext();
        bool CreateDominantLightVisibilityDenoiserContext();
        void DestroyDenoiserContext();
        void DestroyMainDenoiserContext();
        void DestroyDominantLightVisibilityDenoiserContext();
        bool CreateResources();
        bool CreatePipelineObjects();
        bool CreateContent();

        void ApplyConfiguration();
        void ApplyConfiguration(FfxApiConfigureDenoiserKey key);
        void SetDefaultConfiguration();
        void SetDefaultConfiguration(FfxApiConfigureDenoiserKey key);

        [[nodiscard]]
        PackedFrame CreatePackedFrame(const cauldron::ResolutionInfo& resInfo, cauldron::CameraComponent* pCamera, const SceneLightingInformation& sceneLightInfo, U32 dominantLightIndex) const;

        void DispatchPrepare(double deltaTime, cauldron::CommandList* pCmdList, const PackedFrame& frame);
        void DispatchDenoise(double deltaTime, cauldron::CommandList* pCmdList, const Vec3& dominantLightDir, const Vec3& dominantLightEmission);
        void DispatchCompose(double deltaTime, cauldron::CommandList* pCmdList, const PackedFrame& frame);

        //----------------------------------------------------------------------
        // Member Variables
        //----------------------------------------------------------------------

        cauldron::UISection* m_pUISection = nullptr;

        // Settings
        bool m_DenoiserAvailable = {};
        bool m_EnableDebugging   = {};
        bool m_EnableValidation  = {};
        bool m_ShowDebugView     = {};

        struct DenoiserConfiguration
        {
            F32 m_CrossBilateralNormalStrength = {};
            FfxApiFloatBounds m_DebugViewLinearDepthBounds = {};
            F32 m_DisocclusionThreshold        = {};
            F32 m_GaussianKernelRelaxation     = {};
            F32 m_MaxRadiance                  = {};
            F32 m_RadianceClipStdK             = {};
            F32 m_StabilityBias                = {};
        };
        DenoiserConfiguration m_DenoiserConfiguration = {};

        FfxApiDenoiserDebugViewMode m_DebugViewMode = FFX_API_DENOISER_DEBUG_VIEW_MODE_OVERVIEW;
        S32 m_DebugViewport = {};

        bool m_DebugShowChannelR = true;
        bool m_DebugShowChannelG = true;
        bool m_DebugShowChannelB = true;
        bool m_DebugShowChannelA = true;

        // Input signals
        bool m_EnableDirectDiffuse           = true;
        bool m_EnableDirectSpecular          = true;
        bool m_EnableIndirectDiffuse         = true;
        bool m_EnableIndirectSpecular        = true;
        bool m_EnableDominantLightVisibility = true;
        bool m_EnableAmbientOcclusion        = false;
        bool m_EnableSpecularOcclusion       = false;
        bool m_EnableCheckerboarding         = false;
        bool m_EnableSeparateDominantLightVisibilityDenoiserContext = false;

        // View mode dropdowns: index into m_ViewModeValues, populated from the DenoisingViewMode
        // enum excluding the FSR Debug_* overlays (handled by the separate debug view UI). Split-screen
        // comparison renders m_ViewModeLeft on the left and m_ViewModeRight on the right of a vertical
        // divider at m_ViewModeSplit (in [0, 1]); equal left/right modes disable the split.
        S32 m_ViewModeLeft  = {};
        S32 m_ViewModeRight = {};
        F32 m_ViewModeSplit = 0.5f;
        std::vector< DenoisingViewMode > m_ViewModeValues = {};
        std::vector< const char* > m_ViewModeLabels = {};

        cauldron::Entity* m_pDenoiserCamera = {};
        cauldron::CameraComponent* m_pDenoiserCameraComponent = {};

        const cauldron::Texture* m_pColorTarget                     = {};
        const cauldron::Texture* m_pDepthTarget                     = {};
        const cauldron::Texture* m_pGBufferMotionVectors            = {};

        const cauldron::Texture* m_pLinearDepth                     = {};
        const cauldron::Texture* m_pNormals                         = {};
        const cauldron::Texture* m_pDiffuseAlbedo                   = {};
        const cauldron::Texture* m_pSpecularAlbedo                  = {};
        const cauldron::Texture* m_pSkipSignal                      = {};

        const cauldron::Texture* m_pDebugView                       = {};

        // Noisy input signals
        const cauldron::Texture* m_pDirectDiffuse                   = {};
        const cauldron::Texture* m_pDirectSpecular                  = {};
        const cauldron::Texture* m_pIndirectSpecular                = {};
        const cauldron::Texture* m_pIndirectDiffuse                 = {};
        const cauldron::Texture* m_pDominantLightVisibility         = {};
        const cauldron::Texture* m_pAmbientOcclusion                = {};
        const cauldron::Texture* m_pSpecularOcclusion               = {};
        // Denoised output signals
        const cauldron::Texture* m_pDenoisedDirectDiffuse           = {};
        const cauldron::Texture* m_pDenoisedDirectSpecular          = {};
        const cauldron::Texture* m_pDenoisedIndirectSpecular        = {};
        const cauldron::Texture* m_pDenoisedIndirectDiffuse         = {};
        const cauldron::Texture* m_pDenoisedDominantLightVisibility = {};
        const cauldron::Texture* m_pDenoisedAmbientOcclusion        = {};
        const cauldron::Texture* m_pDenoisedSpecularOcclusion       = {};

        cauldron::SamplerDesc m_BilinearSampler;

        cauldron::RootSignature* m_pPrePassRootSignature = {};
        cauldron::PipelineObject* m_pPrePassPipeline = {};
        cauldron::ParameterSet* m_pPrePassParameterSet = {};

        cauldron::RootSignature* m_pComposeRootSignature = {};
        cauldron::PipelineObject* m_pComposePipeline = {};

        static constexpr U32 m_composeParameterSetCount = 3;
        cauldron::ParameterSet* m_pComposeParameterSets[m_composeParameterSetCount];
        cauldron::ParameterSet* m_pCurrentComposeParameterSet = {};
        U32 m_pCurrentComposeParameterSetIndex = 0;

        ffx::Context m_pMainDenoiserContext = {};
        ffx::Context m_pDominantLightVisibilityDenoiserContext = {};

        std::vector< uint64_t > m_DenoiserVersionIds;
        std::vector< const char* > m_DenoiserVersionStrings;
        U32 m_SelectedDenoiserVersion = 0;

        Vec3 m_PrevCameraPosition = Vec3(0);

        bool m_NeedReInit = false;
        bool m_ForceReset = false;
    };
} // namespace rr
