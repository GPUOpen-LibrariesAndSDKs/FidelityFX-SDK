// AMD Cauldron code
//
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "tonemappingrendermodule.h"

#include "core/framework.h"
#include "core/scene.h"
#include "core/uimanager.h"
#include "render/profiler.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/rootsignature.h"
#include "render/color_conversion.h"
#include "render/swapchain.h"

using namespace cauldron;

constexpr uint32_t g_NumThreadX = 8;
constexpr uint32_t g_NumThreadY = 8;

ToneMappingRenderModule::ToneMappingRenderModule() :
    RenderModule(L"ToneMappingRenderModule")
{
    GetFramework()->SetTonemapper(this);
}

ToneMappingRenderModule::ToneMappingRenderModule(const wchar_t* pName) :
    RenderModule(pName)
{
    GetFramework()->SetTonemapper(this);
}

void ToneMappingRenderModule::Init(const json& InitData)
{
    // root signature
    RootSignatureDesc signatureDesc;
    signatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1);
    signatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);
    signatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);

    m_pRootSignature = RootSignature::CreateRootSignature(L"ToneMappingRenderPass_RootSignature", signatureDesc);

    // Get the proper post tone map color target
    switch (GetFramework()->GetSwapChain()->GetSwapChainDisplayMode())
    {
        case DisplayMode::DISPLAYMODE_LDR:
            m_pRenderTarget = GetFramework()->GetRenderTexture(L"LDR8Color");
            break;
        case DisplayMode::DISPLAYMODE_HDR10_2084:
        case DisplayMode::DISPLAYMODE_FSHDR_2084:
            m_pRenderTarget = GetFramework()->GetRenderTexture(L"HDR10Color");
            break;
        case DisplayMode::DISPLAYMODE_HDR10_SCRGB:
        case DisplayMode::DISPLAYMODE_FSHDR_SCRGB:
            m_pRenderTarget = GetFramework()->GetRenderTexture(L"HDR16Color");
            break;
    }
    CauldronAssert(ASSERT_CRITICAL, m_pRenderTarget != nullptr, L"Couldn't find the render target for the tone mapper");

    // Get the input texture
    m_pTexture = GetFramework()->GetRenderTexture(L"HDR11Color");

    // Setup the pipeline object
    PipelineDesc psoDesc;
    psoDesc.SetRootSignature(m_pRootSignature);

    // Setup the shaders to build on the pipeline object
    std::wstring shaderPath = L"tonemapping.hlsl";
    DefineList   defineList;

    defineList.insert(std::make_pair(L"NUM_THREAD_X", std::to_wstring(g_NumThreadX)));
    defineList.insert(std::make_pair(L"NUM_THREAD_Y", std::to_wstring(g_NumThreadY)));
    psoDesc.AddShaderDesc(ShaderBuildDesc::Compute(shaderPath.c_str(), L"MainCS", ShaderModel::SM6_0, &defineList));

    m_pPipelineObj = PipelineObject::CreatePipelineObject(L"ToneMappingRenderPass_PipelineObj", psoDesc);

    m_pParameters = ParameterSet::CreateParameterSet(m_pRootSignature);
    m_pParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(TonemapperCBData), 0);
    
    // Set our texture to the right parameter slot
    m_pParameters->SetTextureSRV(m_pTexture, ViewDimension::Texture2D, 0);
    m_pParameters->SetTextureUAV(m_pRenderTarget, ViewDimension::Texture2D, 0);

    // Register UI for Tone mapping as part of post processing
    UISection uiSection;
    uiSection.SectionName = "Post Processing";

    const char* tonemappers[] = { "AMD Tonemapper", "DX11DSK", "Reinhard", "Uncharted2Tonemap", "ACES", "No Tonemapper" };
    std::vector<std::string> comboOptions;
    comboOptions.assign(tonemappers, tonemappers + _countof(tonemappers));
    uiSection.AddCombo("Tone Mapper", (int32_t*)&m_ConstantData.ToneMapper, &comboOptions);

    m_ConstantData.Exposure = GetScene()->GetSceneExposure();
    std::function<void(void*)> exposureCallback = [this](void* pParams) { GetScene()->SetSceneExposure(this->m_ConstantData.Exposure); };
    uiSection.AddFloatSlider("Exposure", &m_ConstantData.Exposure, 0.f, 5.f, exposureCallback);
    GetUIManager()->RegisterUIElements(uiSection);

    // We are now ready for use
    SetModuleReady(true);
}

ToneMappingRenderModule::~ToneMappingRenderModule()
{
    delete m_pRootSignature;
    delete m_pPipelineObj;
    delete m_pParameters;
}

void ToneMappingRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture tonemappingMarker(pCmdList, L"ToneMapping");

        // Render modules expect resources coming in/going out to be in a shader read state
    Barrier barrier;
    barrier = Barrier::Transition(
        m_pRenderTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::UnorderedAccess);
    ResourceBarrier(pCmdList, 1, &barrier);

    m_ConstantData.MonitorDisplayMode = GetFramework()->GetSwapChain()->GetSwapChainDisplayMode();
    m_ConstantData.DisplayMaxLuminance = GetFramework()->GetSwapChain()->GetHDRMetaData().MaxLuminance;

    // Scene dependent
    ColorSpace inputColorSpace  = ColorSpace_REC709;

    // Display mode dependent
    // Both FSHDR_2084 and HDR10_2084 take rec2020 value
    // Difference is FSHDR needs to be gamut mapped using monitor primaries and then gamut converted to rec2020
    ColorSpace outputColorSpace = ColorSpace_REC2020;
    SetupGamutMapperMatrices(inputColorSpace, outputColorSpace, &m_ConstantData.ContentToMonitorRecMatrix);

    // Allocate a dynamic constant buffer and set
    BufferAddressInfo bufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(TonemapperCBData), &m_ConstantData);
    m_pParameters->UpdateRootConstantBuffer(&bufferInfo, 0);

    // bind all the parameters
    m_pParameters->Bind(pCmdList, m_pPipelineObj);

    // Set pipeline and dispatch
    SetPipelineState(pCmdList, m_pPipelineObj);

    const uint32_t numGroupX = DivideRoundingUp(m_pRenderTarget->GetDesc().Width, g_NumThreadX);
    const uint32_t numGroupY = DivideRoundingUp(m_pRenderTarget->GetDesc().Height, g_NumThreadY);
    Dispatch(pCmdList, numGroupX, numGroupY, 1);

    // Render modules expect resources coming in/going out to be in a shader read state
    barrier = Barrier::Transition(
        m_pRenderTarget->GetResource(), ResourceState::UnorderedAccess, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    ResourceBarrier(pCmdList, 1, &barrier);
}
