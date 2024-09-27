// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
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

#include "render/rendermodule.h"
#include "shaders/upscalecommon.h"
#include "core/framework.h"
#include "core/uimanager.h"
#include "taa/taarendermodule.h"
#include "translucency/translucencyrendermodule.h"
#include "tonemapping/tonemappingrendermodule.h"
#include <FidelityFX/host/ffx_fsr1.h>
#include <FidelityFX/host/ffx_fsr2.h>
#include <FidelityFX/host/ffx_fsr3.h>

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
public:
    FSRRenderModule() : RenderModule(L"FSRRenderModule") {}
    virtual ~FSRRenderModule();

    void Init(const json& initData);
    void EnableModule(bool enabled) override;

    virtual void OnPreFrame() override;

    /**
     * @brief   Setup parameters that the FSR 3 API needs this frame and then call the FFX Dispatch.
     */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;
    void PreTransCallback(double deltaTime, cauldron::CommandList* pCmdList);
    void PostTransCallback(double deltaTime, cauldron::CommandList* pCmdList);

    /**
     * @brief   Recreate the FSR 3 API Context to resize internal resources. Called by the framework when the resolution changes.
     */
    void OnResize(const cauldron::ResolutionInfo& resInfo) override;

    /**
     * @brief   Init UI.
     */
    void InitUI(cauldron::UISection* uiSection);

    /**
     * @brief   Returns whether or not FSR3 requires sample-side re-initialization.
     */
    bool NeedsReInit() const { return m_NeedReInit; }

    /**
     * @brief   Clears FSR3 re-initialization flag.
     */
    void ClearReInit() { m_NeedReInit = false; }

    void SetFilter(UpscaleRM::UpscaleMethod method)
    {
        m_UpscaleMethod = method;

        if (m_IsNonNative)
            m_CurScale = m_ScalePreset;
        m_IsNonNative = (m_UpscaleMethod != UpscaleRM::UpscaleMethod::Native);

        m_ScalePreset = m_IsNonNative ? m_CurScale : FSRScalePreset::NativeAA;
        UpdatePreset((int32_t*)&m_ScalePreset);
    }

private:
    enum class FSRScalePreset
    {
        NativeAA = 0,       // 1.0f
        Quality,            // 1.5f
        Balanced,           // 1.7f
        Performance,        // 2.f
        UltraPerformance,   // 3.f
        Custom,             // 1.f - 3.f range
        DRS
    };

    enum class FSRMaskMode
    {
        Disabled = 0,
        Manual,
        Auto
    };

    const float cMipBias[static_cast<uint32_t>(FSRScalePreset::Custom)] = {
        std::log2f(1.f / 1.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.5f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.7f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 2.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 3.0f) - 1.f + std::numeric_limits<float>::epsilon()
    };

    static void                 FfxMsgCallback(FfxMsgType type, const wchar_t* message);
    static FfxErrorCode         UiCompositionCallback(const FfxPresentCallbackDescription* params, void*);

    void                        SwitchUpscaler(UpscaleRM::UpscaleMethod newUpscaler);

    void                        UpdatePreset(const int32_t* pOldPreset);
    void                        UpdateUpscaleRatio(const float* pOldRatio);
    void                        UpdateMipBias(const float* pOldBias);

    cauldron::ResolutionInfo    UpdateResolution(uint32_t displayWidth, uint32_t displayHeight);
    void                        UpdateFSRContexts(bool enabled);

    static FSRRenderModule*     s_FSRRMInstance;

    bool m_PreShaderReloadEnabledState = false;

    cauldron::UIRenderModule*   m_pUIRenderModule = nullptr;
    cauldron::ResourceView*     m_pRTResourceView = nullptr;

    bool            m_DynamicRes            = false;
    uint32_t        m_MaxRenderWidth        = 0;
    uint32_t        m_MaxRenderHeight       = 0;
    uint32_t        m_MaxUpscaleWidth        = 0;
    uint32_t        m_MaxUpscaleHeight       = 0;
    size_t          m_UIInfoTextIndex       = 0;

    FSRScalePreset  m_CurScale              = FSRScalePreset::Quality;
    FSRScalePreset  m_ScalePreset           = FSRScalePreset::Quality;
    float           m_UpscaleRatio          = 2.f;
    float           m_LetterboxRatio        = 1.f;
    float           m_MipBias               = cMipBias[static_cast<uint32_t>(FSRScalePreset::Quality)];
    FSRMaskMode     m_MaskMode              = FSRMaskMode::Manual;
    float           m_Sharpness             = 0.8f;
    uint32_t        m_JitterIndex           = 0;
    float           m_JitterX               = 0.f;
    float           m_JitterY               = 0.f;
    uint64_t        m_FrameID               = 0;

    bool m_IsNonNative                          = true;
    bool m_UpscaleRatioEnabled                  = false;
    bool m_UseTnCMask                           = true;
    bool m_UseReactiveMask                      = true;
    bool m_RCASSharpen                          = false;
    bool m_SharpnessEnabled                     = false;
    bool m_NeedReInit                           = false;

    bool m_EnableMaskOptions                    = true;
    bool m_FrameInterpolation                   = true;
    bool m_EnableAsyncCompute                   = true;
    bool m_AllowAsyncCompute                    = true;
    bool m_PendingEnableAsyncCompute            = true;
    bool m_UseCallback                          = true;
    bool m_DrawFrameGenerationDebugTearLines    = true;
    bool m_DrawFrameGenerationDebugView         = false;
    bool m_DrawUpscalerDebugView                = false;
    bool m_PresentInterpolatedOnly              = false;
    bool m_DoublebufferInSwapchain              = false;
    bool m_OfUiEnabled                          = true;

    // Upscaling resources
    UpscaleCBData             m_ConstantData;
    cauldron::ParameterSet*   m_pParameters    = nullptr;
    cauldron::PipelineObject* m_pPipelineObj   = nullptr;
    cauldron::RootSignature*  m_pRootSignature = nullptr;

    cauldron::Sampler* m_pPointSampler  = nullptr;
    cauldron::Sampler* m_pLinearSampler = nullptr;

    cauldron::ParameterSet*   m_pParametersBicubic    = nullptr;
    cauldron::PipelineObject* m_pPipelineObjBicubic   = nullptr;
    cauldron::RootSignature*  m_pRootSignatureBicubic = nullptr;

    // FidelityFX Super Resolution information
    FfxFsr1ContextDescription m_FSR1InitializationParameters = {};
    FfxFsr1Context            m_FSR1Context;

    // FidelityFX Super Resolution 2 information
    FfxFsr2ContextDescription m_FSR2InitializationParameters = {};
    FfxFsr2Context            m_FSR2Context;

    // FSR3 Context members
    FfxFrameGenerationConfig  m_FrameGenerationConfig{};
    FfxFsr3ContextDescription m_FSR3InitializationParameters = {};
    FfxFsr3Context            m_FSR3Context;

    // Backup UI elements
    std::vector<cauldron::UIElement*> m_UIElements; // weak ptr

    // FSR3 resources
    const cauldron::Texture*  m_pColorTarget           = nullptr;
    const cauldron::Texture*  m_pTonemappedColorTarget = nullptr;
    const cauldron::Texture*  m_pTempTexture           = nullptr;
    const cauldron::Texture*  m_pDepthTarget           = nullptr;
    const cauldron::Texture*  m_pMotionVectors         = nullptr;
    const cauldron::Texture*  m_pReactiveMask          = nullptr;
    const cauldron::Texture*  m_pCompositionMask       = nullptr;
    const cauldron::Texture*  m_pOpaqueTexture         = nullptr;
    const cauldron::Texture*  m_pFsr1TmpTexture        = nullptr;


    // Raster views for reactive/composition masks
    std::vector<const cauldron::RasterView*> m_RasterViews           = {};
    cauldron::ResourceView*                  m_pUiTargetResourceView = nullptr;

    // For resolution updates
    std::function<cauldron::ResolutionInfo(uint32_t, uint32_t)> m_pUpdateFunc = nullptr;

    void     BuildOpticalFlowUI();
    bool     s_enableSoftwareMotionEstimation = true;
    int32_t  s_uiRenderMode      = 2;

    // Surfaces for different UI render modes
    uint32_t                 m_curUiTextureIndex  = 0;
    const cauldron::Texture* m_pUiTexture[2]      = {};
    const cauldron::Texture* m_pHudLessTexture[2] = {};

    typedef enum Fsr3BackendTypes : uint32_t
    {
        FSR3_BACKEND_UPSCALING,
        FSR3_BACKEND_FRAME_INTERPOLATION,
        FSR3_BACKEND_COUNT
    } Fsr3BackendTypes;
    bool         m_ffxBackendInitialized               = false;
    FfxInterface m_ffxFsr3Backends[FSR3_BACKEND_COUNT] = {};

    UpscaleRM::UpscaleMethod    m_UpscaleMethod = UpscaleRM::UpscaleMethod::FSR3;
    UpscaleRM::UpscaleMethod    m_UIMethod      = UpscaleRM::UpscaleMethod::Native;

    TAARenderModule*          m_pTAARenderModule         = nullptr;
    ToneMappingRenderModule*  m_pToneMappingRenderModule = nullptr;
    TranslucencyRenderModule* m_pTransRenderModule       = nullptr;
};
