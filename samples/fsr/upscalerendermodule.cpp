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


#include "upscalerendermodule.h"

#include <functional>

#include "core/framework.h"
#include "render/dynamicresourcepool.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/profiler.h"
#include "render/rootsignature.h"
#include "render/texture.h"
#include "render/uploadheap.h"

using namespace cauldron;

void UpscaleRenderModule::Init(const json& initData)
{
    // Set our render resolution function as that to use during resize to get render width/height from display width/height
    m_pUpdateFunc = [this](uint32_t displayWidth, uint32_t displayHeight) { return this->UpdateResolution(displayWidth, displayHeight); };

    // UI
    InitUI();

    // Start disabled as this will be enabled externally
    SetModuleEnabled(false);

    std::wstring shaderPath = L"upscalecs.hlsl";
    DefineList defineList;
    // Setup the shaders to build on the pipeline object
    defineList.insert(std::make_pair(L"NUM_THREAD_X", std::to_wstring(g_NumThreadX)));
    defineList.insert(std::make_pair(L"NUM_THREAD_Y", std::to_wstring(g_NumThreadY)));

    // Get the input texture
    m_pRenderInOut = GetFramework()->GetColorTargetForCallback(GetName());

    // Create temporary resource
    {
        // Create the render target and validate it was created
        TextureDesc desc = m_pRenderInOut->GetDesc();
        desc.Name = L"UpscaleIntermediateTarget";

        desc.Width = m_pRenderInOut->GetDesc().Width;
        desc.Height = m_pRenderInOut->GetDesc().Height;

        m_pTempTexture = GetDynamicResourcePool()->CreateRenderTexture(&desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
            desc.Width = renderingWidth;
            desc.Height = renderingHeight;
            });
        CauldronAssert(ASSERT_CRITICAL, m_pTempTexture, L"Couldn't create intermediate texture.");
    }

    // Create the samplers
    {
        SamplerDesc pointSampler;
        pointSampler.Filter   = FilterFunc::MinMagMipPoint;
        pointSampler.AddressW = AddressMode::Wrap;
        m_pPointSampler       = Sampler::CreateSampler(L"UpscalerRM point sampler", pointSampler);

        SamplerDesc linearSampler;
        linearSampler.AddressW = AddressMode::Wrap;
        m_pLinearSampler       = Sampler::CreateSampler(L"UpsamplerRM linear sampler", linearSampler);
    }

    // Upsample
    {
        // root signature
        RootSignatureDesc signatureDesc;
        signatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1);
        signatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);
        signatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);

        signatureDesc.AddSamplerSet(0, ShaderBindStage::Compute, static_cast<int32_t>(UpscaleRM::UpscaleMethod::Count));

        m_pRootSignature = RootSignature::CreateRootSignature(L"UpscaleRenderPass_RootSignature", signatureDesc);

        // Setup the pipeline object
        PipelineDesc psoDesc;
        psoDesc.SetRootSignature(m_pRootSignature);

        // Setup the shaders to build on the pipeline object
        psoDesc.AddShaderDesc(ShaderBuildDesc::Compute(shaderPath.c_str(), L"MainCS", ShaderModel::SM6_0, &defineList));

        m_pPipelineObj = PipelineObject::CreatePipelineObject(L"UpscaleRenderPass_PipelineObj", psoDesc);

        m_pParameters = ParameterSet::CreateParameterSet(m_pRootSignature);
        m_pParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(UpscaleCBData), 0);
        // Set temp texture as output
        m_pParameters->SetTextureUAV(m_pRenderInOut, ViewDimension::Texture2D, 0);
        // Set our texture to the right parameter slot
        m_pParameters->SetTextureSRV(m_pTempTexture, ViewDimension::Texture2D, 0);

        // Update sampler bindings as well
        m_pParameters->SetSampler(m_pPointSampler, static_cast<int32_t>(UpscaleRM::UpscaleMethod::Point));
        m_pParameters->SetSampler(m_pLinearSampler, static_cast<int32_t>(UpscaleRM::UpscaleMethod::Bilinear));
    }

    // Bicubic
    {
        // root signature
        RootSignatureDesc signatureDesc;
        signatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1);
        signatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);
        signatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);

        signatureDesc.AddSamplerSet(0, ShaderBindStage::Compute, static_cast<int32_t>(UpscaleRM::UpscaleMethod::Count));

        m_pRootSignatureBicubic = RootSignature::CreateRootSignature(L"UpscaleBicubicRenderPass_RootSignature", signatureDesc);

        // Setup the pipeline object
        PipelineDesc psoDesc;
        psoDesc.SetRootSignature(m_pRootSignatureBicubic);

        // Setup the shaders to build on the pipeline object
        psoDesc.AddShaderDesc(ShaderBuildDesc::Compute(shaderPath.c_str(), L"BicubicMainCS", ShaderModel::SM6_0, &defineList));

        m_pPipelineObjBicubic = PipelineObject::CreatePipelineObject(L"UpscaleBicubicRenderPass_PipelineObj", psoDesc);

        m_pParametersBicubic = ParameterSet::CreateParameterSet(m_pRootSignature);
        m_pParametersBicubic->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(UpscaleCBData), 0);
        // Set temp texture as output
        m_pParametersBicubic->SetTextureUAV(m_pRenderInOut, ViewDimension::Texture2D, 0);
        // Set our texture to the right parameter slot
        m_pParametersBicubic->SetTextureSRV(m_pTempTexture, ViewDimension::Texture2D, 0);

        // Update sampler bindings as well
        m_pParametersBicubic->SetSampler(m_pLinearSampler, 0);
    }

    // We are now ready for use
    SetModuleReady(true);
}

UpscaleRenderModule::~UpscaleRenderModule()
{
    // Protection
    if (ModuleEnabled())
    {
        EnableModule(false); // Destroy
    }

    delete m_pParameters;
    delete m_pPointSampler;
    delete m_pLinearSampler;
    delete m_pPipelineObj;
    delete m_pRootSignature;
    delete m_pParametersBicubic;
    delete m_pPipelineObjBicubic;
    delete m_pRootSignatureBicubic;
}

void UpscaleRenderModule::EnableModule(bool enabled)
{
    SetModuleEnabled(enabled);

    // If disabling the render module, we need to disable the upscaler with the framework
    if (enabled)
    {
        GetFramework()->EnableUpscaling(true, m_pUpdateFunc);

        // ... and register UI elements for active upscaler
        GetUIManager()->RegisterUIElements(m_UISection);
    }
    else
    {
        GetFramework()->EnableUpscaling(false);

        // Deregister UI elements for inactive upscaler
        GetUIManager()->UnRegisterUIElements(m_UISection);
    }
}

ResolutionInfo UpscaleRenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    return { static_cast<uint32_t>((float)displayWidth / m_UpscaleRatio),
             static_cast<uint32_t>((float)displayHeight / m_UpscaleRatio),
             displayWidth, displayHeight };
}

void UpscaleRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    {
        std::vector<Barrier> barriers;
        barriers.push_back(Barrier::Transition(m_pTempTexture->GetResource(),    
            ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
            ResourceState::CopyDest));
        barriers.push_back(Barrier::Transition(m_pRenderInOut->GetResource(),    
            ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
            ResourceState::CopySource));
        ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    {
        GPUScopedProfileCapture sampleMarker(pCmdList, L"CopyToTemp");

        TextureCopyDesc desc(m_pRenderInOut->GetResource(), m_pTempTexture->GetResource());
        CopyTextureRegion(pCmdList, &desc);
    }

    {
        std::vector<Barrier> barriers;
        barriers.push_back(Barrier::Transition(m_pTempTexture->GetResource(), 
            ResourceState::CopyDest, 
            ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
        barriers.push_back(Barrier::Transition(m_pRenderInOut->GetResource(),  
            ResourceState::CopySource, 
            ResourceState::UnorderedAccess));
        ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    {
        GPUScopedProfileCapture sampleMarker(pCmdList, L"Upsampler (basic)");
        // Allocate a dynamic constant buffer and set
        m_ConstantData.Type = m_UpscaleMethod;

        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

        m_ConstantData.invRenderWidth = 1.0f / resInfo.RenderWidth;
        m_ConstantData.invRenderHeight = 1.0f / resInfo.RenderHeight;
        m_ConstantData.invDisplayWidth = 1.0f / resInfo.DisplayWidth;
        m_ConstantData.invDisplayHeight = 1.0f / resInfo.DisplayHeight;

        BufferAddressInfo bufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(UpscaleCBData), &m_ConstantData);

        if(m_UpscaleMethod != UpscaleRM::UpscaleMethod::Bicubic)
        {
            m_pParameters->UpdateRootConstantBuffer(&bufferInfo, 0);

            // bind all the parameters
            m_pParameters->Bind(pCmdList, m_pPipelineObj);

            // Set pipeline and dispatch
            SetPipelineState(pCmdList, m_pPipelineObj);
        }
        else
        {
            m_pParametersBicubic->UpdateRootConstantBuffer(&bufferInfo, 0);

            // bind all the parameters
            m_pParametersBicubic->Bind(pCmdList, m_pPipelineObjBicubic);

            // Set pipeline and dispatch
            SetPipelineState(pCmdList, m_pPipelineObjBicubic);
        }
        const uint32_t numGroupX = DivideRoundingUp(m_pRenderInOut->GetDesc().Width, g_NumThreadX);
        const uint32_t numGroupY = DivideRoundingUp(m_pRenderInOut->GetDesc().Height, g_NumThreadY);
        Dispatch(pCmdList, numGroupX, numGroupY, 1);

        {
            Barrier barrier = Barrier::Transition(m_pRenderInOut->GetResource(), ResourceState::UnorderedAccess, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
            ResourceBarrier(pCmdList, 1, &barrier);
        }
    }

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
}

void UpscaleRenderModule::OnResize(const ResolutionInfo& resInfo)
{
}

void UpscaleRenderModule::InitUI()
{
    // Build UI options, but don't register them yet. 
    // Registration/Deregistration will be controlled by enabling/disabling the render module
    m_UISection.SectionName = "Upscaling";  // We will piggy-back on existing upscaling section"
    m_UISection.SectionType = UISectionType::Sample;

    // Setup scale preset options
    const char* preset[] = { "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Ultra Performance (3x)", "Custom" };
    std::vector<std::string> presetComboOptions;
    presetComboOptions.assign(preset, preset + _countof(preset));
    std::function<void(void*)> presetCallback = [this](void* pParams) { this->UpdatePreset(static_cast<int32_t*>(pParams)); };
    m_UISection.AddCombo("Scale Preset", reinterpret_cast<int32_t*>(&m_ScalePreset), &presetComboOptions, presetCallback);

    // Setup scale factor (disabled for all but custom)
    std::function<void(void*)> ratioCallback = [this](void* pParams) { this->UpdateUpscaleRatio(static_cast<float*>(pParams)); };
    m_UISection.AddFloatSlider("Custom Scale", &m_UpscaleRatio, 1.f, 3.f, ratioCallback, &m_UpscaleRatioEnabled);
}

void UpscaleRenderModule::UpdatePreset(const int32_t* pOldPreset)
{
    switch (m_ScalePreset)
    {
    case ScalePreset::Quality:
        m_UpscaleRatio = 1.5f;
        break;
    case ScalePreset::Balanced:
        m_UpscaleRatio = 1.7f;
        break;
    case ScalePreset::Performance:
        m_UpscaleRatio = 2.0f;
        break;
    case ScalePreset::UltraPerformance:
        m_UpscaleRatio = 3.0f;
        break;
    case ScalePreset::Custom:
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update whether we can update the custom scale slider
    m_UpscaleRatioEnabled = (m_ScalePreset == ScalePreset::Custom);

    // Disable/Enable FSR2 since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void UpscaleRenderModule::UpdateUpscaleRatio(const float* pOldRatio)
{
    // Disable/Enable FSR2 since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}
