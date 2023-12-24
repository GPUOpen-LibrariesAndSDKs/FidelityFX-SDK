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


#include "cacaorendermodule.h"
#include "validation_remap.h"

#include "core/framework.h"
#include "core/components/cameracomponent.h"
#include "core/loaders/textureloader.h"
#include "core/scene.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/profiler.h"
#include "render/rasterview.h"

#define FFX_CACAO_ARRAY_SIZE(xs) (sizeof(xs) / sizeof(xs[0]))

#include <limits>

using namespace cauldron;

void CACAORenderModule::Init(const json& initData)
{    
    m_PresetId = 0;
    m_CacaoSettings = s_FfxCacaoPresets[m_PresetId].settings;
    m_GenerateNormals = m_CacaoSettings.generateNormals;
    m_UseDownsampledSSAO = s_FfxCacaoPresets[m_PresetId].useDownsampledSsao;
    m_OutputToCallbackTarget = true;

    // Fetch needed resource
    m_pColorTarget  = GetFramework()->GetRenderTexture(L"GBufferAoRoughnessMetallicRT");
    m_pCallbackColorTarget = GetFramework()->GetRenderTexture(L"HDR11Color");
    m_pColorRasterView     = GetRasterViewAllocator()->RequestRasterView(m_pCallbackColorTarget, ViewDimension::Texture2D);
    m_pDepthTarget = GetFramework()->GetRenderTexture(L"DepthTarget");
    m_pNormalTarget = GetFramework()->GetRenderTexture(L"GBufferNormalTarget");

    //////////////////////////////////////////////////////////////////////////
    // Final pass resources for CACAO output only

    m_LinearSamplerDesc.Filter        = FilterFunc::MinMagLinearMipPoint;
    m_LinearSamplerDesc.MaxAnisotropy = 1u;

    RootSignatureDesc prepareOutputSignatureDesc;
    prepareOutputSignatureDesc.AddTextureSRVSet(0, ShaderBindStage::Pixel, 1);
    prepareOutputSignatureDesc.AddStaticSamplers(0, ShaderBindStage::Pixel, 1, &m_LinearSamplerDesc);

    m_pPrepareOutputRS = RootSignature::CreateRootSignature(L"CACAO_DisplayOutput", prepareOutputSignatureDesc);

    BlendDesc blendDesc                     = {};
    blendDesc.BlendEnabled                  = false;
    blendDesc.SourceBlendColor              = Blend::One;
    blendDesc.ColorOp                       = BlendOp::Add;
    blendDesc.DestBlendColor                = Blend::SrcAlpha;
    blendDesc.SourceBlendAlpha              = Blend::One;
    blendDesc.AlphaOp                       = BlendOp::Add;
    blendDesc.DestBlendAlpha                = Blend::One;
    const std::vector<BlendDesc> blendDescs = {blendDesc};

    // Setup the pipeline object
    PipelineDesc psoDesc;
    psoDesc.SetRootSignature(m_pPrepareOutputRS);
    psoDesc.AddShaderDesc(ShaderBuildDesc::Vertex(L"fullscreen.hlsl", L"FullscreenVS", ShaderModel::SM6_0, nullptr));
    psoDesc.AddShaderDesc(ShaderBuildDesc::Pixel(L"display_ssao.hlsl", L"ps_main", ShaderModel::SM6_0, nullptr));
    psoDesc.AddPrimitiveTopology(PrimitiveTopologyType::Triangle);
    psoDesc.AddBlendStates(blendDescs, false, false);
    psoDesc.AddRasterFormats(m_pCallbackColorTarget->GetFormat());  // Use the first raster set, as we just want the format and they are all the same

    m_pPrepareOutputPipeline = PipelineObject::CreatePipelineObject(L"CACAO_DisplayOutput", psoDesc);

    m_pParamSet = ParameterSet::CreateParameterSet(m_pPrepareOutputRS);
    m_pParamSet->SetTextureSRV(m_pColorTarget, ViewDimension::Texture2D, 0);

    // Initialize cacao context.
    InitContext();

    // Build UI
    InitUI();

    // Start disabled as this will be enabled externally
    SetModuleEnabled(true);

    // That's all we need for now
    SetModuleReady(true);
}

CACAORenderModule::~CACAORenderModule()
{
    // Protection
    if (ModuleEnabled())
        EnableModule(false);    // Destroy FSR context

    // Destroy CACAO Contexts
    ffxCacaoContextDestroy(&m_CacaoContext);
    ffxCacaoContextDestroy(&m_CacaoDownsampledContext);

    // Destroy the FidelityFX interface memory
    free(m_FfxInterface.scratchBuffer);
}

void CACAORenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    ffxCacaoContextDestroy(&m_CacaoContext);
    ffxCacaoContextDestroy(&m_CacaoDownsampledContext);
    CreateContext(resInfo);
}

void CACAORenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture sampleMarker(pCmdList, L"CACAO");

    CameraComponent* pCamera = GetScene()->GetCurrentCamera();
    FfxCacaoMat4x4 proj, normalsWorldToView;
    {
        FFX_ASSERT_MESSAGE(
            sizeof(FfxCacaoMat4x4) == sizeof(math::Matrix4),
            "Cannot memcpy math::Matrix4 type into FfxCacaoMat4x4 type due to size mismatch.");  //If untrue => cannot memcpy as the precision or structure must've changed.

        //Set projection matrix.
        memcpy(proj.elements, &pCamera->GetProjection(), sizeof(FfxCacaoMat4x4));

        //Set normalsWorldToView matrix.
        const math::Matrix4 zFlipMat = math::Matrix4(
            math::Vector4(1, 0, 0, 0), 
            math::Vector4(0, 1, 0, 0), 
            math::Vector4(0, 0, -1, 0), 
            math::Vector4(0, 0, 0, 1)
        );

        math::Matrix4 xNormalsWorldToView = zFlipMat * math::transpose(math::inverse(pCamera->GetView()));
            
        memcpy(normalsWorldToView.elements, &math::transpose(xNormalsWorldToView), sizeof(FfxCacaoMat4x4));
    }

    m_CacaoSettings.generateNormals = m_GenerateNormals;
    FfxErrorCode errorCode = ffxCacaoUpdateSettings(m_UseDownsampledSSAO ? &m_CacaoDownsampledContext : &m_CacaoContext, &m_CacaoSettings, m_UseDownsampledSSAO);
    CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Error returned from ffxCacaoUpdateSettings");

    FfxCacaoDispatchDescription dispatchDescription = {};
    dispatchDescription.commandList = ffxGetCommandList(pCmdList);
    dispatchDescription.depthBuffer = ffxGetResource(m_pDepthTarget->GetResource(), L"CacaoInputDepth", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchDescription.normalBuffer = ffxGetResource(m_pNormalTarget->GetResource(), L"CacaoInputNormal", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchDescription.outputBuffer    = ffxGetResource((m_pColorTarget->GetResource()), L"CacaoInputOutput", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchDescription.proj = &proj;
    dispatchDescription.normalsToView = &normalsWorldToView;
    dispatchDescription.normalUnpackMul = 2;
    dispatchDescription.normalUnpackAdd = -1;
    errorCode = ffxCacaoContextDispatch(m_UseDownsampledSSAO ? &m_CacaoDownsampledContext : &m_CacaoContext, &dispatchDescription);
    CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Error returned from ffxCacaoContextDispatch");

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    //For output to screen
    if (!m_OutputToCallbackTarget)
    {
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        const Barrier toRenderTargetBarrier = Barrier::Transition(
            m_pCallbackColorTarget->GetResource(), ResourceState::PixelShaderResource | ResourceState::NonPixelShaderResource, ResourceState::RenderTargetResource);
        ResourceBarrier(pCmdList, 1, &toRenderTargetBarrier);

        BeginRaster(pCmdList, 1, &m_pColorRasterView);

        m_pParamSet->Bind(pCmdList, m_pPrepareOutputPipeline);

        Viewport vp = {0.f, 0.f, resInfo.fDisplayWidth(), resInfo.fDisplayHeight(), 0.f, 1.f};
        SetViewport(pCmdList, &vp);
        Rect scissorRect = {0, 0, resInfo.RenderWidth, resInfo.RenderHeight};
        SetScissorRects(pCmdList, 1, &scissorRect);

        SetPipelineState(pCmdList, m_pPrepareOutputPipeline);
        DrawInstanced(pCmdList, 3);

        EndRaster(pCmdList);

        const Barrier toDefaultStateBarrier = Barrier::Transition(
            m_pCallbackColorTarget->GetResource(), ResourceState::RenderTargetResource, ResourceState::PixelShaderResource | ResourceState::NonPixelShaderResource);
        ResourceBarrier(pCmdList, 1, &toDefaultStateBarrier);
    }
}

void CACAORenderModule::InitContext()
{
    const size_t scratchBufferSize = ffxGetScratchMemorySize(FFX_CACAO_CONTEXT_COUNT * 2);
    void* scratchBuffer            = calloc(scratchBufferSize, 1);
    FfxErrorCode errorCode         = ffxGetInterface(&m_FfxInterface, GetDevice(), scratchBuffer, 
                                                     scratchBufferSize, FFX_CACAO_CONTEXT_COUNT * 2);
    CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Could not initialize FidelityFX SDK backend context.");

    CreateContext(GetFramework()->GetResolutionInfo());
}

void CACAORenderModule::InitUI()
{
    m_UISection.SectionName = "FFX CACAO";
    m_UISection.SectionType = UISectionType::Sample;

    std::function<void(void*)> PresetCallback = [this](void* pParams) {
        if (m_PresetId < FFX_CACAO_ARRAY_SIZE(s_FfxCacaoPresets))
        {
            m_CacaoSettings      = s_FfxCacaoPresets[m_PresetId].settings;
            m_UseDownsampledSSAO = s_FfxCacaoPresets[m_PresetId].useDownsampledSsao;
        }
    };
    m_UISection.AddCombo("Preset", &m_PresetId, &s_FfxCacaoPresetNames, PresetCallback);
    std::function<void(void*)> StateChangeCallback = [this](void* pParams) {
        if (m_PresetId < FFX_CACAO_ARRAY_SIZE(s_FfxCacaoPresets) &&
            (memcmp(&m_CacaoSettings, &s_FfxCacaoPresets[m_PresetId].settings, sizeof(m_CacaoSettings)) ||
             m_UseDownsampledSSAO != s_FfxCacaoPresets[m_PresetId].useDownsampledSsao))
        {
            m_PresetId = FFX_CACAO_ARRAY_SIZE(s_FfxCacaoPresets);
        }
    };

    FfxCacaoSettings* settings = &m_CacaoSettings;
    m_UISection.AddFloatSlider("Radius", &settings->radius, 0.0f, 10.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Shadow Multiplier", &settings->shadowMultiplier, 0.0f, 5.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Shadow Power", &settings->shadowPower, 0.5f, 5.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Shadow Clamp", &settings->shadowClamp, 0.0f, 1.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Horizon Angle Threshold", &settings->horizonAngleThreshold, 0.0f, 0.2f, StateChangeCallback);
    m_UISection.AddFloatSlider("Fade Out From", &settings->fadeOutFrom, 1.0f, 20.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Fade Out To", &settings->fadeOutTo, 1.0f, 40.0f, StateChangeCallback);

    const char*              qualityLevel[] = {"Lowest", "Low", "Medium", "High", "Highest"};
    std::vector<std::string> qualityLevelComboOptions;
    for (int32_t i = 0; i < FFX_CACAO_ARRAY_SIZE(qualityLevel); ++i)
        qualityLevelComboOptions.push_back(qualityLevel[i]);
    m_UISection.AddCombo("Quality Level", (int*)&settings->qualityLevel, &qualityLevelComboOptions, StateChangeCallback);

    if (settings->qualityLevel == FFX_CACAO_QUALITY_HIGHEST)
    {
        m_UISection.AddFloatSlider("Adaptive Quality Limit", &settings->adaptiveQualityLimit, 0.5f, 1.0f, StateChangeCallback);
    }
    m_UISection.AddIntSlider("Blur Pass Count", (int*)&settings->blurPassCount, 0, 8, StateChangeCallback);
    m_UISection.AddFloatSlider("Sharpness", &settings->sharpness, 0.0f, 1.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Detail Shadow Strength", &settings->detailShadowStrength, 0.0f, 5.0f, StateChangeCallback);

    m_UISection.AddCheckBox("Generate Normal Buffer From Depth Buffer", &m_GenerateNormals, StateChangeCallback);

    std::function<void(void*)> DownsampleCallback = [this, StateChangeCallback](void* pParams) {
        StateChangeCallback(pParams);
    };
    m_UISection.AddCheckBox("Use Downsampled SSAO", &m_UseDownsampledSSAO, DownsampleCallback);
    m_UISection.AddFloatSlider("Bilateral Sigma Squared", &settings->bilateralSigmaSquared, 0.0f, 10.0f, StateChangeCallback);
    m_UISection.AddFloatSlider("Bilateral Similarity Distance Sigma", &settings->bilateralSimilarityDistanceSigma, 0.1f, 1.0f, StateChangeCallback);
}

void CACAORenderModule::EnableModule(bool enabled){
    RenderModule::EnableModule(enabled);
    if(enabled){
        GetUIManager()->RegisterUIElements(m_UISection);
    }else {
        GetUIManager()->UnRegisterUIElements(m_UISection);
    }
}


void CACAORenderModule::CreateContext(const ResolutionInfo& resInfo)
{
    FfxCacaoContextDescription description = {};
    description.backendInterface           = m_FfxInterface;
    description.width                      = resInfo.RenderWidth;
    description.height                     = resInfo.RenderHeight;
    description.useDownsampledSsao         = false;
    FfxErrorCode errorCode                 = ffxCacaoContextCreate(&m_CacaoContext, &description);
    CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Could not initialize FidelityFX SDK backend context.");
    description.useDownsampledSsao = true;
    errorCode = ffxCacaoContextCreate(&m_CacaoDownsampledContext, &description);
    CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Could not initialize FidelityFX SDK downsampled backend context.");
}

void CACAORenderModule::SetOutputToCallbackTarget(const bool outputToCallbackTarget)
{
    m_OutputToCallbackTarget = outputToCallbackTarget;
}
