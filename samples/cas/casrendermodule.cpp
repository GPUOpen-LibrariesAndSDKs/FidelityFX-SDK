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


#include "casrendermodule.h"
#include "validation_remap.h"

#include "render/dynamicresourcepool.h"
#include "render/profiler.h"
#include "render/uploadheap.h"

#include "core/components/cameracomponent.h"
#include "core/scene.h"
#include "core/uimanager.h"

#include <functional>

using namespace cauldron;

void CASRenderModule::Init(const json& initData)
{
    //////////////////////////////////////////////////////////////////////////
    // Resource setup

    // Fetch needed resources
    m_pColorTarget = GetFramework()->GetColorTargetForCallback(GetName());

    // Create an temporary texture to which the color target will be copied every frame, as input
    TextureDesc           desc    = m_pColorTarget->GetDesc();
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    desc.Width                    = resInfo.RenderWidth;
    desc.Height                   = resInfo.RenderHeight;
    desc.Name                     = L"CAS_Copy_Color";
    m_pTempColorTarget            = GetDynamicResourcePool()->CreateRenderTexture(
        &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
            desc.Width  = renderingWidth;
            desc.Height = renderingHeight;
        });

    // Setup Cauldron FidelityFX interface.
    const size_t scratchBufferSize = ffxGetScratchMemorySize(FFX_CAS_CONTEXT_COUNT);
    void*        scratchBuffer     = calloc(scratchBufferSize, 1);
    FfxErrorCode errorCode         = ffxGetInterface(&m_InitializationParameters.backendInterface, GetDevice(), scratchBuffer, scratchBufferSize, FFX_CAS_CONTEXT_COUNT);
    FFX_ASSERT(errorCode == FFX_OK);

    // Set our render resolution function as that to use during resize to get render width/height from display width/height
    m_pUpdateFunc = [this](uint32_t displayWidth, uint32_t displayHeight) { return this->UpdateResolution(displayWidth, displayHeight); };

    //////////////////////////////////////////////////////////////////////////
    // Build UI and register them to the framework

    UISection uiSection;
    uiSection.SectionName = "Sharpening";
    uiSection.SectionType = UISectionType::Sample;


    // Setup CAS state
    std::vector<std::string>   casStateComboOptions{"No Cas", "Cas Upsample", "Cas Sharpen Only"};
    std::function<void(void*)> CasStateCallback = [this](void* pParams) {
        this->m_CasEnabled          = (this->m_CasState != CAS_State_NoCas);
        this->m_CasUpscalingEnabled = (this->m_CasState == CAS_State_Upsample);
        if (!this->m_CasUpscalingEnabled) {
            this->m_UpscaleRatioEnabled = false;  // Upscale ratio bar must be also disabled here since UpdatePreset will not be hit if only state is changed.
            GetFramework()->EnableUpscaling(false); // Tell the framework we are not performing upscaling, so that it can provide full display sized render target.
        }
        else
        {
            UpdatePreset(nullptr);
        }
    };
    uiSection.AddCombo("Cas Options", reinterpret_cast<int32_t*>(&m_CasState), &casStateComboOptions, CasStateCallback);

    // CAS settings
    uiSection.AddFloatSlider("Cas Sharpness", &m_Sharpness, 0.f, 1.f, nullptr, &m_CasEnabled);

    // Setup scale preset options
    const char*              preset[] = {"Ultra Quality (1.3x)", "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Ultra Performance (3x)", "Custom"};
    std::vector<std::string> presetComboOptions;
    presetComboOptions.assign(preset, preset + _countof(preset));
    std::function<void(void*)> presetCallback = [this](void* pParams) { this->UpdatePreset(static_cast<int32_t*>(pParams)); };
    uiSection.AddCombo("Scale Preset", reinterpret_cast<int32_t*>(&m_ScalePreset), &presetComboOptions, presetCallback, &m_CasUpscalingEnabled);

    // Setup scale factor (disabled for all but custom)
    std::function<void(void*)> ratioCallback = [this](void* pParams) { this->UpdateUpscaleRatio(static_cast<float*>(pParams)); };
    uiSection.AddFloatSlider("Custom Scale", &m_UpscaleRatio, 1.f, 3.f, ratioCallback, &m_UpscaleRatioEnabled);

    // ... and register UI elements for active upscaler
    GetUIManager()->RegisterUIElements(uiSection);

    //////////////////////////////////////////////////////////////////////////
    // Finish up init

    // Create the cas context
    ResetCas();

    // That's all we need for now
    SetModuleReady(true);
}

CASRenderModule::~CASRenderModule()
{
    ffxCasContextDestroy(&m_CasContext);

    // Destroy the FidelityFX interface memory
    free(m_InitializationParameters.backendInterface.scratchBuffer);
}

void CASRenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    ResetCas();
}

cauldron::ResolutionInfo CASRenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    return {
        static_cast<uint32_t>((float)displayWidth / m_UpscaleRatio), static_cast<uint32_t>((float)displayHeight / m_UpscaleRatio), displayWidth, displayHeight};
}

void CASRenderModule::UpdatePreset(const int32_t* pOldPreset)
{
    switch (m_ScalePreset)
    {
    case CASScalePreset::UltraQuality:
        m_UpscaleRatio = 1.3f;
        break;
    case CASScalePreset::Quality:
        m_UpscaleRatio = 1.5f;
        break;
    case CASScalePreset::Balanced:
        m_UpscaleRatio = 1.7f;
        break;
    case CASScalePreset::Performance:
        m_UpscaleRatio = 2.0f;
        break;
    case CASScalePreset::UltraPerformance:
        m_UpscaleRatio = 3.0f;
        break;
    case CASScalePreset::Custom:
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update whether we can update the custom scale slider
    m_UpscaleRatioEnabled = (m_CasUpscalingEnabled && m_ScalePreset == CASScalePreset::Custom);

    // Update resolution since rendering ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void CASRenderModule::ResetCas()
{
    static bool s_firstReset = true;
    if (!s_firstReset)
    {
        ffxCasContextDestroy(&m_CasContext);
    }
    else
    {
        s_firstReset = false;
    }

    if (m_CasState == CAS_State_SharpenOnly)
        m_InitializationParameters.flags |= FFX_CAS_SHARPEN_ONLY;
    else
        m_InitializationParameters.flags &= ~FFX_CAS_SHARPEN_ONLY;

    m_InitializationParameters.colorSpaceConversion = FFX_CAS_COLOR_SPACE_LINEAR;

    const ResolutionInfo& resInfo                   = GetFramework()->GetResolutionInfo();
    m_InitializationParameters.maxRenderSize.width  = resInfo.RenderWidth;
    m_InitializationParameters.maxRenderSize.height = resInfo.RenderHeight;
    m_InitializationParameters.displaySize.width    = resInfo.DisplayWidth;
    m_InitializationParameters.displaySize.height   = resInfo.DisplayHeight;

    // Create the cas context
    ffxCasContextCreate(&m_CasContext, &m_InitializationParameters);
}

void CASRenderModule::UpdateUpscaleRatio(const float* pOldRatio)
{
    // Disable/Enable CAS since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void CASRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    if (m_CasState == CAS_State_NoCas)
    {
        return;
    }

    GPUScopedProfileCapture sampleMarker(pCmdList, L"FFX CAS");
    const ResolutionInfo&   resInfo = GetFramework()->GetResolutionInfo();
    CameraComponent*        pCamera = GetScene()->GetCurrentCamera();

    FfxCasDispatchDescription dispatchParameters = {};
    dispatchParameters.commandList               = ffxGetCommandList(pCmdList);
    dispatchParameters.renderSize                = {resInfo.RenderWidth, resInfo.RenderHeight};
    dispatchParameters.sharpness                 = m_Sharpness;

    // We need to copy color buffer to an internal temp texture and use this texture as the input of CAS
    // TODO: explore other (more efficient) ways.
    std::vector<Barrier> barriers;
    barriers.push_back(Barrier::Transition(m_pTempColorTarget->GetResource(), 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
        ResourceState::CopyDest));
    barriers.push_back(Barrier::Transition(m_pColorTarget->GetResource(), 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
        ResourceState::CopySource));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    TextureCopyDesc desc(m_pColorTarget->GetResource(), m_pTempColorTarget->GetResource());
    CopyTextureRegion(pCmdList, &desc);

    barriers[0] = Barrier::Transition(m_pTempColorTarget->GetResource(), 
        ResourceState::CopyDest, 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    barriers[1] = Barrier::Transition(m_pColorTarget->GetResource(), 
        ResourceState::CopySource, 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
    dispatchParameters.color  = ffxGetResource(m_pTempColorTarget->GetResource(), L"CAS_InputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchParameters.output = ffxGetResource(m_pColorTarget->GetResource(), L"CAS_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

    FfxErrorCode errorCode = ffxCasContextDispatch(&m_CasContext, &dispatchParameters);
    FFX_ASSERT(errorCode == FFX_OK);

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
}
