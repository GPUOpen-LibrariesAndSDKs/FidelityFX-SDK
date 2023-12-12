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
#include "shaders/upscalecommon.h"
#include "core/uimanager.h"

namespace cauldron
{
    class ParameterSet;
    class PipelineObject;
    class RootSignature;
    class Texture;
    class Sampler;
} // namespace cauldron

constexpr uint32_t g_NumThreadX = 32;
constexpr uint32_t g_NumThreadY = 32;

/// @defgroup FfxFsrSample FidelityFX Super Resolution Sample
/// Sample documentation for FidelityFX Super Resolution
///
/// @ingroup SDKEffects

/// @defgroup USRM UpscaleRenderModule
/// UpscaleRenderModule Reference Documentation
///
/// @ingroup FfxFsrSample
/// @{

/**
 * @class UpscaleRenderModule
 *
 * UpscaleRenderModule takes care of:
 *      - creating UI section that enable users to select upscaling options
 *      - creating GPU resources
 *      - dispatch workloads for simple bilinear and bicubic upsampling
 */
class UpscaleRenderModule : public cauldron::RenderModule
{
public:
    /**
     * @brief   Constructor with default behavior.
     */
    UpscaleRenderModule() : RenderModule(L"UpscaleRenderModule") {}

    /**
     * @brief   Release GPU resources.
     */
    virtual ~UpscaleRenderModule();

    /**
     * @brief   Create upscaling resources and setup UI.
     */
    void Init(const json& initData) override;

    /**
     * @brief   Enable upscaling and register UI section if render module is enabled.
     */
    void EnableModule(bool enabled) override;

    /**
     * @brief   Dispatch workloads to upscale image using bilinear or bicubic sampling.
     */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

    /**
     * @brief   Window resize callback with default behavior.
     */
    void OnResize(const cauldron::ResolutionInfo& resInfo) override;

    /**
     * @brief   Set the filtering method used during upscaling.
     */
    void SetFilter(UpscaleRM::UpscaleMethod method) { m_UpscaleMethod = method; }

private:
    cauldron::ResolutionInfo UpdateResolution(uint32_t displayWidth, uint32_t displayHeight);

    // Enum representing the upscaling quality modes. 
    enum class ScalePreset
    {
        Quality = 0,        // 1.5f
        Balanced,           // 1.7f
        Performance,        // 2.f
        UltraPerformance,   // 3.f
        Custom              // 1.f - 3.f range
    };

    void InitUI();
    void UpdatePreset(const int32_t* pOldPreset);
    void UpdateUpscaleRatio(const float* pOldRatio);

     // For UI params
    cauldron::UISection m_UISection;

    float m_UpscaleRatio = 2.0f;
    ScalePreset m_ScalePreset = ScalePreset::Performance;
    bool m_UpscaleRatioEnabled = false;

    UpscaleCBData m_ConstantData;

    // Upscaling resources
    cauldron::ParameterSet* m_pParameters = nullptr;
    cauldron::PipelineObject* m_pPipelineObj = nullptr;
    const cauldron::Texture* m_pRenderInOut = nullptr;
    const cauldron::Texture* m_pTempTexture = nullptr;
    cauldron::RootSignature* m_pRootSignature = nullptr;

    cauldron::Sampler* m_pPointSampler = nullptr;
    cauldron::Sampler* m_pLinearSampler = nullptr;

    cauldron::ParameterSet* m_pParametersBicubic = nullptr;
    cauldron::PipelineObject* m_pPipelineObjBicubic = nullptr;
    cauldron::RootSignature* m_pRootSignatureBicubic = nullptr;

    uint32_t m_UpscaleOrder = 0;
    UpscaleRM::UpscaleMethod m_UpscaleMethod = UpscaleRM::UpscaleMethod::Point;
    // For resolution updates
    std::function<cauldron::ResolutionInfo(uint32_t, uint32_t)> m_pUpdateFunc = nullptr;

/// @}
};
