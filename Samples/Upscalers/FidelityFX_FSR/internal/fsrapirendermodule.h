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
    enum UpscalerType
    {
        Upscaler_Native,
        Upscaler_FSRAPI,

    } UpscalerType;

    enum UICompositionMode
    {
        No_UI_Handling = 0,
        UiTexture,
        UiCallback,
		PreUiBackbuffer,
	};

public:
    FSRRenderModule()
        : RenderModule(L"FSRApiRenderModule"),
          m_SafetyMarginInMs(0.1f),
          m_VarianceFactor (0.1f),
          m_AllowHybridSpin (false),
          m_HybridSpinTime(2),
          m_AllowWaitForSingleObjectOnFence(false),
          framePacingTuning { m_SafetyMarginInMs, m_VarianceFactor, m_AllowHybridSpin, m_HybridSpinTime, m_AllowWaitForSingleObjectOnFence }
    {}
    virtual ~FSRRenderModule();

    void Init(const json& initData);
    void EnableModule(bool enabled) override;
    void OnPreFrame() override;

    /**
     * @brief   Setup parameters that the FSR API needs this frame and then call the FFX Dispatch.
     */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;
    void PreTransCallback(double deltaTime, cauldron::CommandList* pCmdList);
    void PostTransCallback(double deltaTime, cauldron::CommandList* pCmdList);

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

    void SetFrameInterpolationHotkey(bool enabled)
    {
        if (m_EnableFrameInterpolationSwapchain && m_FrameInterpolation != enabled)
        {
            m_FrameInterpolation = enabled;
            m_OfUiEnabled = m_FrameInterpolation && s_enableSoftwareMotionEstimation;
            m_NeedReInit = true;
        }
    }

    void EnableFrameInterpolationSwapchain(bool enabled);

#if defined(_VK)
    void EnableFrameInterpolationSwapchainVK(bool enabled);
#endif

#if defined(_DX12)
    void EnableFrameInterpolationSwapchainDX12(bool enabled);
#endif

private:
    friend class FSRSettingsHandler;

    struct Version
    {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;

        operator std::string() const
        {
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        }

        Version& operator=(const std::string& str)
        {
            return from_string(str.c_str());
        }

        Version& operator=(const char* str)
        {
            return from_string(str);
        }

    private:
        Version& from_string(const char* str)
        {
            sscanf_s(str, "%u.%u.%u", &major, &minor, &patch);
            return *this;
        }
    };

    enum class FSRScalePreset : int32_t
    {
        NativeAA = 0,       // 1.0f
        Quality,            // 1.5f
        Balanced,           // 1.7f
        Performance,        // 2.f
        UltraPerformance,   // 3.f
        Custom,             // 1.f - 3.f range
        CustomDRS,          // Custom but with DRS flag set
    };

    enum class FSRMaskMode
    {
        Disabled = 0,
        Manual,
        Auto
    };

    enum FSRDebugCheckerMode
    {
        Disabled = 0,
        EnabledNoMessageCallbackSilence,
        EnabledNoMessageCallbackErrors,
        EnabledNoMessageCallbackWarnings,
        EnabledWithMessageCallbackSilence,
        EnabledWithMessageCallbackErrors,
        EnabledWithMessageCallbackWarnings,
    };

    enum class FSRColorSpace
    {
        DefaultLinearColorSpace = 0,
        NonLinearColorSpace,
        sRGBColorSpace,
        PQColorSpace
    };

    const float cMipBias[static_cast<uint32_t>(FSRScalePreset::Custom)] = {
        std::log2f(1.f / 1.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.5f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.7f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 2.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 3.0f) - 1.f + std::numeric_limits<float>::epsilon()
    };

    static void FfxMsgCallback(uint32_t type, const wchar_t* message);
    ffxReturnCode_t UiCompositionCallback(ffxCallbackDescFrameGenerationPresent*);

    void                     SwitchUpscaler(int32_t newUpscaler);

    void                     UpdatePreset(const int32_t* pOldPreset);
    void                     UpdateUpscaleRatio(const float* pOldRatio);
    void                     UpdateMipBias(const float* pOldBias);

    cauldron::ResolutionInfo UpdateResolution(uint32_t displayWidth, uint32_t displayHeight);
    void                     UpdateFSRContext(bool enabled);

    cauldron::UIRenderModule*   m_pUIRenderModule = nullptr;
    cauldron::ResourceView*     m_pRTResourceView = nullptr;

    int32_t         m_UpscaleMethod   = Upscaler_FSRAPI;
    int32_t         m_UiUpscaleMethod = Upscaler_FSRAPI;
    FSRScalePreset  m_CurScale        = FSRScalePreset::Quality;
    FSRScalePreset  m_ScalePreset     = FSRScalePreset::Quality;
    float           m_UpscaleRatio    = 2.f;
    float           m_LetterboxRatioX = 1.0f;
    float           m_LetterboxRatioY  = 1.0f;
    float           m_GenerationRectRatioX = 1.0f;
    float           m_GenerationRectRatioY = 1.0f;
    float           m_MipBias         = cMipBias[static_cast<uint32_t>(FSRScalePreset::Quality)];
    FSRMaskMode     m_MaskMode        = FSRMaskMode::Manual;
    float           m_Sharpness       = 0.8f;
    uint32_t        m_JitterIndex     = 0;
    float           m_JitterX         = 0.f;
    float           m_JitterY         = 0.f;
    float           m_PreviousJitterX = 0.f;
    float           m_PreviousJitterY = 0.f;
    uint64_t        m_FrameID         = 0;

    bool m_IsNonNative                              = true;
    bool m_UpscaleRatioEnabled                      = false;
    FSRColorSpace m_colorSpace                      = FSRColorSpace::DefaultLinearColorSpace;
    bool m_UseMask                                  = true;
    bool m_UseDistortionField                       = false;
    bool m_RCASSharpen                              = true;
    bool m_SharpnessEnabled                         = false;
    bool m_NeedReInit                               = false;

    bool m_FrameInterpolationAvailable              = false;
    bool m_AsyncComputeAvailable                    = false;
    bool m_EnableMaskOptions                        = true;
    bool m_EnableWaitCallbackModeUI                 = true;
    bool m_FrameInterpolation                       = true;
    bool m_EnableAsyncCompute                       = true;
    bool m_AllowAsyncCompute                        = true;
    bool m_PendingEnableAsyncCompute                = true;
    bool m_UseCallback                              = true;
    bool m_DrawFrameGenerationDebugTearLines        = true;
    bool m_DrawFrameGenerationDebugResetIndicators  = true;
    bool m_DrawFrameGenerationDebugPacingLines      = false;
    bool m_DrawFrameGenerationDebugView             = false;
    bool m_DrawUpscalerDebugView                    = false;
    bool m_AppBlackOutsideGenerationRect             = true;
    bool m_PresentInterpolatedOnly                  = false;
    bool m_SimulatePresentSkip                      = false;
    bool m_ResetUpscale                             = false;
    bool m_ResetFrameInterpolation                  = false;
    bool m_DoublebufferInSwapchain                  = false;
    bool m_OfUiEnabled                              = true;
    bool m_FrameGenerationDebugViewEnabled          = false;
    bool m_EnableFrameInterpolationSwapchain        = true;

    // FFX API Context members
    std::vector<uint64_t> m_FsrVersionIds;
    int32_t m_FsrVersionIndex = 0;
    bool        m_overrideVersion = false;
    bool        m_InitialSettingsApplied = false;
    uint64_t    m_currentUpscaleContextVersionId = 0;
    const char* m_currentUpscaleContextVersionName = nullptr;
    std::vector<std::string> m_FsrVersionNames;

    std::vector<uint64_t> m_FgVersionIds;
    int32_t m_FgVersionIndex = 0;
    std::vector<std::string> m_FgVersionNames;
    Version m_currentFgContextVersion = {};

    bool m_scNeedReInit = false;

    bool m_ffxBackendInitialized = false;
    ffx::Context m_UpscalingContext = nullptr;
    ffx::Context m_FrameGenContext  = nullptr;
    ffx::Context m_SwapChainContext = nullptr;
    ffx::ConfigureDescFrameGeneration m_FrameGenerationConfig{};

    // Backup UI elements
    std::vector<cauldron::UIElement*> m_UIElements{}; // weak ptr
    cauldron::UIElement*              m_pAsyncComputeSupportUI = nullptr;
    cauldron::UIElement*              m_pAllowAsyncComputeUI   = nullptr;

    // FSR resources
    const cauldron::Texture*  m_pColorTarget           = nullptr;
    const cauldron::Texture*  m_pTonemappedColorTarget = nullptr;
    const cauldron::Texture*  m_pTempTexture           = nullptr;
    const cauldron::Texture*  m_pDepthTarget           = nullptr;
    const cauldron::Texture*  m_pMotionVectors         = nullptr;
    const cauldron::Texture*  m_pReactiveMask          = nullptr;
    const cauldron::Texture*  m_pCompositionMask       = nullptr;
    const cauldron::Texture*  m_pOpaqueTexture         = nullptr;

    // Raster views for reactive/composition masks
    std::vector<const cauldron::RasterView*> m_RasterViews           = {};
    cauldron::ResourceView*                  m_pUiTargetResourceView = nullptr;

    // For resolution updates
    std::function<cauldron::ResolutionInfo(uint32_t, uint32_t)> m_pUpdateFunc = nullptr;

    bool     s_enableSoftwareMotionEstimation = true;
    int32_t  s_uiRenderMode      = UICompositionMode::UiCallback;
    int32_t  s_uiRenderModeNextFrame = UICompositionMode::UiCallback; // needs to be in-sync with s_uiRenderMode after deviating at most 1 frame.
    int32_t  s_uiRenderModeAtDeactivation = UICompositionMode::UiCallback; // used to restore last Mode at reactivation of swapchain.

    // Surfaces for different UI render modes
    uint32_t                 m_curUiTextureIndex  = 0;
    const cauldron::Texture* m_pUiTexture[2]      = {};
    const cauldron::Texture* m_pHudLessTexture[2] = {};
    const cauldron::Texture* m_pDistortionField[2] = {};

    // surface interpolation output when not using the frame interpolation swapchain
    const cauldron::Texture* m_pInterpolationOutput = nullptr;

    TAARenderModule*          m_pTAARenderModule         = nullptr;
    ToneMappingRenderModule*  m_pToneMappingRenderModule = nullptr;
    TranslucencyRenderModule* m_pTransRenderModule       = nullptr;

    //Set Constant Buffer KeyValue via Configure Context KeyValue API. Valid Post Context creation.
    int32_t                  m_UpscalerCBKey = 0;
    float                    m_UpscalerCBValue = 1.f;
    float                    m_UpscalerCBValueStore[5] = {1.f,1.f,1.f,1.0f/3,-1.0f/3};
    void                     SetUpscaleConstantBuffer(uint64_t key, float value);

    FSRDebugCheckerMode      m_GlobalDebugCheckerMode = FSRDebugCheckerMode::Disabled;
    void                     SetGlobalDebugCheckerMode(FSRDebugCheckerMode mode, bool recreate);
    //Set Swapchain waitcallback via Configure Context KeyValue API
    int32_t                  m_waitCallbackMode = 0;

    //Set Swapchain Frame pacing Tuning
    float m_SafetyMarginInMs; // in Millisecond
    float m_VarianceFactor; // valid range [0.0,1.0]
    bool  m_AllowHybridSpin;
    uint32_t m_HybridSpinTime;
    bool m_AllowWaitForSingleObjectOnFence;
    FfxApiSwapchainFramePacingTuning framePacingTuning;
    uint32_t m_CameraAnimationMode = 0;
    bool m_EnableCameraAnimationNoise = false;
    bool m_ChangeCameraAnimationDirection = false; // there are only two directions, so use the bool directly

    bool  m_AnimateGenerationRect            = false;
    float m_GenRectAnimationT                = 0.0f;   // current animation parameter [0..1]
    bool  m_GenRectAnimationForward          = true;    // true = animating toward upscalerOutputRect

    bool   getLatencyWaitableObject = false;
    HANDLE latencyWaitableObj = 0;
};

// alias to get sample.cpp to use this class.
using FSRApiRenderModule = FSRRenderModule;
