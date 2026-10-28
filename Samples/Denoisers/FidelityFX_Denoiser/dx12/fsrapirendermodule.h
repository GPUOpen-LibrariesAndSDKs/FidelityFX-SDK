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

#include <cauldron2/framework/render/rendermodule.h>
#include <cauldron2/framework/render/rendermodules/tonemapping/tonemappingrendermodule.h>
#include <cauldron2/framework/core/framework.h>
#include <cauldron2/framework/core/uimanager.h>

#include <cauldron2/rendermodules/taa/taarendermodule.h>
#include <cauldron2/rendermodules/translucency/translucencyrendermodule.h>

#include <FidelityFX/upscalers/include/ffx_upscale.hpp>
#include <FidelityFX/framegeneration/include/ffx_framegeneration.hpp>

#include <functional>

namespace cauldron
{
    class Texture;
    class ParameterSet;
    class ResourceView;
    class RootSignature;
    class UIRenderModule;
}

class FSRRenderModule : public cauldron::RenderModule
{
    enum UpscalerType : uint8_t
    {
        Upscaler_Native,
        Upscaler_FSRAPI,

    } UpscalerType;

public:
    [[nodiscard]]
    FSRRenderModule();
    virtual ~FSRRenderModule();

    void Init(const json& initData);
    void EnableModule(bool enabled) override;
    void OnPreFrame() override;

    /**
     * @brief   Setup parameters that the FSR API needs this frame and then call the FFX Dispatch.
     */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

    /**
     * @brief   Recreate the FSR API Context to resize internal resources. Called by the framework when the resolution changes.
     */
    void OnResize(const cauldron::ResolutionInfo& resInfo) override;

    /**
     * @brief   Init UI.
     */
    void InitUI(cauldron::UISection* uiSection);

    /**
     * @brief   Returns whether or not FSR requires sample-side re-initialization.
     */
    [[nodiscard]]
    bool NeedsReInit() const { return m_NeedReInit; }

    /**
     * @brief   Clears FSR re-initialization flag.
     */
    void ClearReInit() { m_NeedReInit = false; }

    void SetFilter(int32_t method)
    {
        m_UpscaleMethod = method;

        if (m_IsNonNative)
            m_CurScale = m_ScalePreset;
        m_IsNonNative = (m_UpscaleMethod != Upscaler_Native);

        m_ScalePreset = m_IsNonNative ? m_CurScale : FSRScalePreset::NativeAA;
        UpdatePreset((int32_t*)&m_ScalePreset);
    }

    void SetUpscaleMethodHotkey(int32_t method)
    {
        if (method != m_UiUpscaleMethod)
        {
            m_UiUpscaleMethod = method;
            m_UiEnabled = (method == Upscaler_FSRAPI); // Sync UI checkbox state

            // If enabling FSR API, set scale preset to Native AA
            if (method == Upscaler_FSRAPI)
            {
                m_ScalePreset = FSRScalePreset::NativeAA;
                m_CurScale = FSRScalePreset::NativeAA;  // Also update current scale
            }

            SwitchUpscaler(method);
        }
    }

    void SetScalePresetHotkey(int32_t preset)
    {
        if (preset >= 0 && preset <= static_cast<int32_t>(FSRScalePreset::UltraPerformance))
        {
            int32_t oldPreset = static_cast<int32_t>(m_ScalePreset);
            m_ScalePreset = static_cast<FSRScalePreset>(preset);
            UpdatePreset(&oldPreset);
        }
    }

    void ShowUI(bool show);

private:
    friend class FSRSettingsHandler;
    enum class FSRScalePreset : int32_t
    {
        NativeAA = 0,       // 1.0f
        Quality,            // 1.5f
        Balanced,           // 1.7f
        Performance,        // 2.f
        UltraPerformance,   // 3.f
    };

    const float cMipBias[static_cast<uint32_t>(FSRScalePreset::UltraPerformance) + 1] = {
        std::log2f(1.f / 1.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.5f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.7f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 2.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 3.0f) - 1.f + std::numeric_limits<float>::epsilon()
    };

    void                     SwitchUpscaler(int32_t newUpscaler);

    void                     UpdatePreset(const int32_t* pOldPreset);
    void                     UpdateUpscaleRatio(const float* pOldRatio);
    void                     UpdateMipBias(const float* pOldBias);

    cauldron::ResolutionInfo UpdateResolution(uint32_t displayWidth, uint32_t displayHeight);
    void                     UpdateFSRContext(bool enabled);

    int32_t         m_UpscaleMethod   = Upscaler_FSRAPI;
    int32_t         m_UiUpscaleMethod = Upscaler_FSRAPI;
    FSRScalePreset  m_CurScale        = FSRScalePreset::Performance;
    FSRScalePreset  m_ScalePreset     = FSRScalePreset::Performance;
    float           m_UpscaleRatio    = 2.f;
    float           m_MipBias         = cMipBias[static_cast<uint32_t>(FSRScalePreset::NativeAA)];
    uint32_t        m_JitterIndex     = 0;
    float           m_JitterX         = 0.f;
    float           m_JitterY         = 0.f;
    float           m_PreviousJitterX = 0.f;
    float           m_PreviousJitterY = 0.f;
    uint64_t        m_FrameID         = 0;

    bool m_Enabled                                  = true;
    bool m_UiEnabled                                = true;
    bool m_IsNonNative                              = true;
    bool m_NeedReInit                               = false;

    cauldron::UISection* m_pUISection = nullptr;

    // FFX API Context members
    std::vector<uint64_t> m_FsrVersionIds;
    int32_t m_FsrVersionIndex = 0;
    bool        m_overrideVersion = false;
    uint64_t    m_currentUpscaleContextVersionId = 0;
    const char* m_currentUpscaleContextVersionName = nullptr;
    std::vector<const char*> m_FsrVersionNames;

    bool m_ffxBackendInitialized = false;
    ffx::Context m_UpscalingContext = nullptr;

    // FSR resources
    const cauldron::Texture*  m_pColorTarget           = nullptr;
    const cauldron::Texture*  m_pTonemappedColorTarget = nullptr;
    const cauldron::Texture*  m_pTempTexture           = nullptr;
    const cauldron::Texture*  m_pDepthTarget           = nullptr;
    const cauldron::Texture*  m_pMotionVectors         = nullptr;
    const cauldron::Texture*  m_pReactiveMask          = nullptr;
    const cauldron::Texture*  m_pCompositionMask       = nullptr;
    const cauldron::Texture*  m_pOpaqueTexture         = nullptr;

    // For resolution updates
    std::function<cauldron::ResolutionInfo(uint32_t, uint32_t)> m_pUpdateFunc = nullptr;

    TAARenderModule*          m_pTAARenderModule         = nullptr;
    ToneMappingRenderModule*  m_pToneMappingRenderModule = nullptr;
    TranslucencyRenderModule* m_pTransRenderModule       = nullptr;
};

// alias to get sample.cpp to use this class.
using FSRApiRenderModule = FSRRenderModule;
