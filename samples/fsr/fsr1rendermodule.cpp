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


#include "fsr1rendermodule.h"
#include "validation_remap.h"
#include "render/device.h"
#include "render/dynamicresourcepool.h"
#include "render/profiler.h"
#include "render/swapchain.h"
#include "render/uploadheap.h"
#include "core/scene.h"

#include <functional>

using namespace cauldron;

void FSR1RenderModule::Init(const json& initData)
{
    //////////////////////////////////////////////////////////////////////////
    // Resource setup

    // Fetch needed resources
    m_pColorTarget = GetFramework()->GetColorTargetForCallback(GetName());
    switch (GetFramework()->GetSwapChain()->GetSwapChainDisplayMode())
    {
        case DisplayMode::DISPLAYMODE_LDR:
            m_pColorTarget = GetFramework()->GetRenderTexture(L"LDR8Color");
            break;
        case DisplayMode::DISPLAYMODE_HDR10_2084:
        case DisplayMode::DISPLAYMODE_FSHDR_2084:
            m_pColorTarget = GetFramework()->GetRenderTexture(L"HDR10Color");
            break;
        case DisplayMode::DISPLAYMODE_HDR10_SCRGB:
        case DisplayMode::DISPLAYMODE_FSHDR_SCRGB:
            m_pColorTarget = GetFramework()->GetRenderTexture(L"HDR16Color");
            break;
    }


    // Set the format to use for the intermediate resource when RCAS is enabled
    switch (m_pColorTarget->GetFormat())
    {
    case ResourceFormat::RGBA16_FLOAT:
        m_InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        break;
        // For all other instances, just use floating point 32-bit format
    default:
        m_InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
        break;
    }

    // Setup a temp render target for when RCas is enabled
    TextureDesc desc = m_pColorTarget->GetDesc();
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    desc.Width = resInfo.RenderWidth;
    desc.Height = resInfo.RenderHeight;
    desc.Name = L"FSR1_Copy_Color";
    m_pTempColorTarget = GetDynamicResourcePool()->CreateRenderTexture(&desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight)
        {
            desc.Width = renderingWidth;
            desc.Height = renderingHeight;
        });

    // Set our render resolution function as that to use during resize to get render width/height from display width/height
    m_pUpdateFunc = [this](uint32_t displayWidth, uint32_t displayHeight) { return this->UpdateResolution(displayWidth, displayHeight); };

    // UI
    InitUI();

    // Start disabled as this will be enabled externally
    SetModuleEnabled(false);

    // That's all we need for now
    SetModuleReady(true);
}

FSR1RenderModule::~FSR1RenderModule()
{
    // Protection
    if (ModuleEnabled())
        EnableModule(false);    // Destroy FSR context
}

void FSR1RenderModule::EnableModule(bool enabled)
{
    // If disabling the render module, we need to disable the upscaler with the framework
    if (enabled)
    {
        // Setup everything needed when activating FSR
        // Will also enable upscaling
        UpdatePreset(nullptr);

        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        // Setup FidelityFX interface.
        const size_t scratchBufferSize = ffxGetScratchMemorySize(FFX_FSR1_CONTEXT_COUNT);
        void*        scratchBuffer     = calloc(scratchBufferSize, 1);
        FfxErrorCode errorCode         = ffxGetInterface(&m_InitializationParameters.backendInterface, GetDevice(), scratchBuffer, scratchBufferSize, FFX_FSR1_CONTEXT_COUNT);
        CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Could not initialize the FidelityFX SDK backend");

        // Create the FSR1 context
        UpdateFSR1Context(true);

        // ... and register UI elements for active upscaler
        GetUIManager()->RegisterUIElements(m_UISection);
    }
    else
    {
        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        GetFramework()->EnableUpscaling(false);

        // Destroy the FSR1 context
        UpdateFSR1Context(false);

        // Destroy the FidelityFX interface memory
        free(m_InitializationParameters.backendInterface.scratchBuffer);

        // Deregister UI elements for inactive upscaler
        GetUIManager()->UnRegisterUIElements(m_UISection);
    }
}

void FSR1RenderModule::InitUI()
{
    // Build UI options, but don't register them yet. Registration/Deregistration will be controlled by enabling/disabling the render module
    m_UISection.SectionName = "Upscaling";  // We will piggy-back on existing upscaling section"
    m_UISection.SectionType = UISectionType::Sample;

    // Setup scale preset options
    const char* preset[] = { "Ultra Quality (1.3x)", "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Custom" };
    std::vector<std::string> presetComboOptions;
    presetComboOptions.assign(preset, preset + _countof(preset));
    std::function<void(void*)> presetCallback = [this](void* pParams) { this->UpdatePreset(static_cast<int32_t*>(pParams)); };
    m_UISection.AddCombo("Scale Preset", reinterpret_cast<int32_t*>(&m_ScalePreset), &presetComboOptions, presetCallback);

    // Setup mip bias
    std::function<void(void*)> mipBiasCallback = [this](void* pParams) { this->UpdateMipBias(static_cast<float*>(pParams)); };
    m_UISection.AddFloatSlider("Mip LOD Bias", &m_MipBias, -5.f, 0.f, mipBiasCallback);

    // Setup scale factor (disabled for all but custom)
    std::function<void(void*)> ratioCallback = [this](void* pParams) { this->UpdateUpscaleRatio(static_cast<float*>(pParams)); };
    m_UISection.AddFloatSlider("Custom Scale", &m_UpscaleRatio, 1.f, 3.f, ratioCallback, &m_UpscaleRatioEnabled);

    // Sharpening
    std::function<void(void*)> sharpeningCallback = [this](void* pParams) { this->UpdateRCasSetting(static_cast<bool*>(pParams)); };
    m_UISection.AddCheckBox("RCAS Sharpening", &m_RCASSharpen, sharpeningCallback);
    m_UISection.AddFloatSlider("Sharpness", &m_Sharpness, 0.f, 2.f, nullptr, &m_RCASSharpen);
}

void FSR1RenderModule::UpdatePreset(const int32_t* pOldPreset)
{
    switch (m_ScalePreset)
    {
    case FSR1ScalePreset::UltraQuality:
        m_UpscaleRatio = 1.3f;
        break;
    case FSR1ScalePreset::Quality:
        m_UpscaleRatio = 1.5f;
        break;
    case FSR1ScalePreset::Balanced:
        m_UpscaleRatio = 1.7f;
        break;
    case FSR1ScalePreset::Performance:
        m_UpscaleRatio = 2.0f;
        break;
    case FSR1ScalePreset::Custom:
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update whether we can update the custom scale slider
    m_UpscaleRatioEnabled = (m_ScalePreset == FSR1ScalePreset::Custom);

    // Update mip bias
    float oldValue = m_MipBias;
    if (m_ScalePreset != FSR1ScalePreset::Custom)
        m_MipBias = cMipBias[static_cast<uint32_t>(m_ScalePreset)];
    else
        m_MipBias = CalculateMipBias(m_UpscaleRatio);
    UpdateMipBias(&oldValue);

    // Update resolution since rendering ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSR1RenderModule::UpdateUpscaleRatio(const float* pOldRatio)
{
    // Disable/Enable FSR1 since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSR1RenderModule::UpdateMipBias(const float* pOldBias)
{
    // Update the scene MipLODBias to use
    GetScene()->SetMipLODBias(m_MipBias);
}

void FSR1RenderModule::UpdateRCasSetting(const bool* pOldSetting)
{
    // Flush the GPU so we don't remove things in flight
    GetDevice()->FlushAllCommandQueues();

    // Need to destroy/create the FSR1 context when RCas is enabled/disabled
    // as everything will be setup for that scenario
    UpdateFSR1Context(false);
    UpdateFSR1Context(true);
}

void FSR1RenderModule::UpdateFSR1Context(bool enabled)
{
    if (enabled)
    {
        // HDR?
        if (GetSwapChain()->GetSwapChainDisplayMode() != DisplayMode::DISPLAYMODE_LDR)
            m_InitializationParameters.flags |= FFX_FSR1_ENABLE_HIGH_DYNAMIC_RANGE;
        else
            m_InitializationParameters.flags &= ~FFX_FSR1_ENABLE_HIGH_DYNAMIC_RANGE;

        // Apply RCAS?
        if (m_RCASSharpen)
            m_InitializationParameters.flags |= FFX_FSR1_ENABLE_RCAS;
        else
            m_InitializationParameters.flags &= ~FFX_FSR1_ENABLE_RCAS;

        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        m_InitializationParameters.maxRenderSize.width = resInfo.RenderWidth;
        m_InitializationParameters.maxRenderSize.height = resInfo.RenderHeight;
        m_InitializationParameters.displaySize.width = resInfo.DisplayWidth;
        m_InitializationParameters.displaySize.height = resInfo.DisplayHeight;

        // Create the FSR1 context
        FfxErrorCode errorCode = ffxFsr1ContextCreate(&m_FSR1Context, &m_InitializationParameters);
        CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Couldn't create the FidelityFX SDK FSR1 context.");
    }
    else
    {
        // Destroy the FSR1 context
        ffxFsr1ContextDestroy(&m_FSR1Context);
    }
}

ResolutionInfo FSR1RenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    return { static_cast<uint32_t>((float)displayWidth / m_UpscaleRatio),
             static_cast<uint32_t>((float)displayHeight / m_UpscaleRatio),
             displayWidth, displayHeight };
}

void FSR1RenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    // Need to recreate the FSR1 context on resource resize
    UpdateFSR1Context(false);   // Destroy
    UpdateFSR1Context(true);    // Re-create
}

void FSR1RenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture sampleMarker(pCmdList, L"FFX FSR1");
    const ResolutionInfo&   resInfo = GetFramework()->GetResolutionInfo();
    CameraComponent*        pCamera = GetScene()->GetCurrentCamera();

    FfxFsr1DispatchDescription dispatchParameters = {};
    dispatchParameters.commandList = ffxGetCommandList(pCmdList);
    dispatchParameters.renderSize = { resInfo.RenderWidth, resInfo.RenderHeight };
    dispatchParameters.enableSharpening = m_RCASSharpen;
    dispatchParameters.sharpness = m_Sharpness;

    // We need to copy the low-res results to a temp target regardless as FSR1 doesn't deal well with resources that are
    // rendered with limited scisor/viewport rects
    std::vector<Barrier> barriers;
    barriers.push_back(Barrier::Transition(m_pTempColorTarget->GetResource(), 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
        ResourceState::CopyDest));
    barriers.push_back(Barrier::Transition(m_pColorTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopySource));
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
    dispatchParameters.color = ffxGetResource(m_pTempColorTarget->GetResource(), L"FSR1_InputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchParameters.output = ffxGetResource(m_pColorTarget->GetResource(), L"FSR1_OutputUpscaledColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

    FfxErrorCode errorCode = ffxFsr1ContextDispatch(&m_FSR1Context, &dispatchParameters);
    FFX_ASSERT(errorCode == FFX_OK);

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
}
