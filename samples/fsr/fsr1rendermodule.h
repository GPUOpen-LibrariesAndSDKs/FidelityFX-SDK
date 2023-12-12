// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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

#include "render/rendermodule.h"
#include "core/framework.h"
#include "core/uimanager.h"

#include <FidelityFX/host/ffx_fsr1.h>

#include <functional>

namespace cauldron
{
    class Texture;
}  // namespace cauldron

/// @defgroup FfxFsrSample FidelityFX Super Resolution Sample
/// Sample documentation for FidelityFX Super Resolution
///
/// @ingroup SDKEffects

/// @defgroup FSR1RM FSR1RenderModule
/// FSR1RenderModule Reference Documentation
///
/// @ingroup FfxFsrSample
/// @{

/**
 * @class FSR1RenderModule
 *
 * FSR1RenderModule takes care of:
 *      - creating UI section that enable users to select upscaling options
 *      - dispatch workloads for upscaling using FSR 
 */
class FSR1RenderModule : public cauldron::RenderModule
{
public:
    /**
     * @brief   Constructor with default behavior.
     */
    FSR1RenderModule() : RenderModule(L"FSR1RenderModule") {}
    
    /**
     * @brief   Tear down the FSR 1 API Context and release resources.
     */
    virtual ~FSR1RenderModule();

    /**
     * @brief   Initialize FSR 1 API Context, create resources, and setup UI section for FSR 1.
     */
    void Init(const json& initData) override;
    
    /**
     * @brief   If render module is enabled, initialize the FSR 1 API Context. If disabled, destroy the FSR 1 API Context.
     */
    void EnableModule(bool enabled) override;

    /**
     * @brief   Setup parameters that the FSR 1 API needs this frame and then call the FFX Dispatch.
     */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;
    
    /**
     * @brief   Recreate the FSR 1 API Context to resize internal resources. Called by the framework when the resolution changes.
     */
    void OnResize(const cauldron::ResolutionInfo& resInfo) override;

private:

    // Enum representing the FSR 1 quality modes. 
    enum class FSR1ScalePreset
    {
        UltraQuality = 0,   // 1.3f
        Quality,            // 1.5f
        Balanced,           // 1.7f
        Performance,        // 2.f
        Custom              // 1.f - 3.f range
    };

    // LUT of mip bias values corresponding to each FSR1ScalePreset value.
    const float cMipBias[static_cast<uint32_t>(FSR1ScalePreset::Custom)] =
    {
        CalculateMipBias(1.3f),
        CalculateMipBias(1.5f),
        CalculateMipBias(1.7f),
        CalculateMipBias(2.0f)
    };

    void                     InitUI();
    void                     UpdatePreset(const int32_t* pOldPreset);
    void                     UpdateUpscaleRatio(const float* pOldRatio);
    void                     UpdateMipBias(const float* pOldBias);
    void                     UpdateRCasSetting(const bool* pOldSetting);

    cauldron::ResolutionInfo UpdateResolution(uint32_t displayWidth, uint32_t displayHeight);
    void                     UpdateFSR1Context(bool enabled);

    FSR1ScalePreset m_ScalePreset  = FSR1ScalePreset::UltraQuality;
    float           m_UpscaleRatio = 2.f;
    float           m_MipBias      = cMipBias[static_cast<uint32_t>(FSR1ScalePreset::UltraQuality)];
    float           m_Sharpness    = 1.f;

    bool m_UpscaleRatioEnabled = false;
    bool m_UseMask             = true;
    bool m_RCASSharpen         = true;

    // FidelityFX Super Resolution information
    FfxFsr1ContextDescription m_InitializationParameters = {};
    FfxFsr1Context            m_FSR1Context;

    // For UI params
    cauldron::UISection       m_UISection;

    // FidelityFX Super Resolution resources
    const cauldron::Texture*  m_pColorTarget     = nullptr;
    const cauldron::Texture*  m_pTempColorTarget = nullptr;

    // For resolution updates
    std::function<cauldron::ResolutionInfo(uint32_t, uint32_t)> m_pUpdateFunc = nullptr;

/// @}
};
