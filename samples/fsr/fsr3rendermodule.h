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

#include <FidelityFX/host/ffx_fsr3.h>

#include <functional>

#include <dxgi.h>
#include <dxgi1_4.h>

namespace cauldron
{
    class Texture;
    class ParameterSet;
    class ResourceView;
    class RootSignature;
}

class FSR3RenderModule : public cauldron::RenderModule
{
public:
    FSR3RenderModule() : RenderModule(L"FSR3RenderModule") {}
    virtual ~FSR3RenderModule();

    void Init(const json& initData);
    void EnableModule(bool enabled) override;
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;
    void PreTransCallback(double deltaTime, cauldron::CommandList* pCmdList);
    void PostTransCallback(double deltaTime, cauldron::CommandList* pCmdList);
    void OnResize(const cauldron::ResolutionInfo& resInfo) override;

    bool m_NeedReInit = false;

private:

    enum class FSR3ScalePreset
    {
        NativeAA = 0,       // 1.0f
        Quality,            // 1.5f
        Balanced,           // 1.7f
        Performance,        // 2.f
        UltraPerformance,   // 3.f
        Custom              // 1.f - 3.f range
    };

    enum class FSR3MaskMode
    {
        Disabled = 0,
        Manual,
        Auto
    };

    const float cMipBias[static_cast<uint32_t>(FSR3ScalePreset::Custom)] = {
        std::log2f(1.f / 1.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.5f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 1.7f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 2.0f) - 1.f + std::numeric_limits<float>::epsilon(),
        std::log2f(1.f / 3.0f) - 1.f + std::numeric_limits<float>::epsilon()
    };

    static void FfxMsgCallback(FfxMsgType type, const wchar_t* message);
#if defined(FFX_API_DX12)
    static FfxErrorCode UiCompositionCallback(const FfxPresentCallbackDescription* params);
#endif

    void                     UpdatePreset(const int32_t* pOldPreset);
    void                     UpdateUpscaleRatio(const float* pOldRatio);
    void                     UpdateMipBias(const float* pOldBias);

    cauldron::ResolutionInfo UpdateResolution(uint32_t displayWidth, uint32_t displayHeight);
    void                     UpdateFSR3Context(bool enabled);

    FSR3ScalePreset m_ScalePreset   = FSR3ScalePreset::Quality;
    float           m_UpscaleRatio  = 2.f;
    float           m_MipBias       = cMipBias[static_cast<uint32_t>(FSR3ScalePreset::Quality)];
    FSR3MaskMode    m_MaskMode      = FSR3MaskMode::Manual;
    float           m_Sharpness     = 0.8f;
    uint32_t        m_JitterIndex   = 0;
    float           m_JitterX       = 0.f;
    float           m_JitterY       = 0.f;

    bool m_UpscaleRatioEnabled  = false;
    bool m_UseMask              = true;
    bool m_RCASSharpen          = true;
    bool m_SharpnessEnabled     = false;

#if defined(FFX_API_DX12)
    bool    m_OfRebuildPipelines = false;

    bool    m_FrameInterpolation = true;
    bool    m_EnableAsyncCompute = true;
    bool    m_AllowAsyncCompute         = true;
    bool    m_PendingEnableAsyncCompute = true;
    bool    m_UseCallback           = true;
    bool    m_DrawDebugTearLines    = true;
    bool    m_DrawDebugView         = false;
    bool    m_OfUiEnabled           = true;
#endif

    bool    m_bUseHLSLShaders                   = true;

    // FSR3 Context members
    FfxFrameGenerationConfig  m_FrameGenerationConfig{};
    FfxFsr3ContextDescription m_InitializationParameters = {};
    FfxFsr3Context            m_FSR3Context;

#if defined(FFX_API_DX12)
    // DX swapchain descriptors for recreation of "real" swapchain
    DXGI_SWAP_CHAIN_DESC            gameSwapChainDesc;
    DXGI_SWAP_CHAIN_DESC1           gameSwapChainDesc1;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC gameFullscreenDesc;
#endif

    // For UI params
    cauldron::UISection       m_UISection;

    // FSR3 resources
    const cauldron::Texture*  m_pColorTarget        = nullptr;
    const cauldron::Texture*  m_pDepthTarget        = nullptr;
    const cauldron::Texture*  m_pMotionVectors      = nullptr;
    const cauldron::Texture*  m_pReactiveMask       = nullptr;
    const cauldron::Texture*  m_pCompositionMask    = nullptr;
    const cauldron::Texture*  m_pOpaqueTexture      = nullptr;

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
        FSR3_BACKEND_SHARED_RESOURCES,
        FSR3_BACKEND_UPSCALING,
        FSR3_BACKEND_FRAME_INTERPOLATION,
        FSR3_BACKEND_COUNT
    } Fsr3BackendTypes;
    bool         ffxBackendInitialized_               = false;
    FfxInterface ffxFsr3Backends_[FSR3_BACKEND_COUNT] = {};
};
