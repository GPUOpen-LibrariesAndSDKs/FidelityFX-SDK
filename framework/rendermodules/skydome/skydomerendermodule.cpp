// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "skydomerendermodule.h"
#include "core/contentmanager.h"
#include "core/framework.h"
#include "core/loaders/textureloader.h"
#include "core/uimanager.h"
#include "core/scene.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/profiler.h"
#include "render/resourceview.h"
#include "render/rootsignature.h"
#include "render/sampler.h"

using namespace std::experimental;
using namespace cauldron;

constexpr uint32_t g_NumThreadX = 8;
constexpr uint32_t g_NumThreadY = 8;

void SkyDomeRenderModule::Init(const json& initData)
{
    // Init the right version
    m_IsProcedural = initData.value("Procedural", m_IsProcedural);

    // Read in procedural defaults
    GetScene()->GetSkydomeHour() = initData.value("Hour", GetScene()->GetSkydomeHour());
    GetScene()->GetSkydomeMinute() = initData.value("Minute", GetScene()->GetSkydomeMinute());
    m_ProceduralConstantData.Rayleigh = initData.value("Rayleigh", 2.f);
    m_ProceduralConstantData.Turbidity = initData.value("Turbidity", 10.f);
    m_ProceduralConstantData.MieCoefficient = initData.value("Mie", 0.005f);
    m_ProceduralConstantData.Luminance = initData.value("Luminance", 3.5f);
    m_ProceduralConstantData.MieDirectionalG = initData.value("MieDir", 0.8f);

    if (m_IsProcedural)
        InitProcedural();
    else
        InitSkyDome();
}

SkyDomeRenderModule::~SkyDomeRenderModule()
{
    delete m_pRootSignature;
    delete m_pPipelineObj;
    delete m_pParameters;
}

void SkyDomeRenderModule::InitSkyDome()
{
    // We are now ready for use
    SetModuleReady(false);

    // create sampler
    SamplerDesc samplerDesc;
    samplerDesc.AddressW = AddressMode::Wrap;

    // root signature
    RootSignatureDesc signatureDesc;
    signatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1);
    signatureDesc.AddConstantBufferView(1, ShaderBindStage::Compute, 1);
    signatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);
    signatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);

    signatureDesc.AddStaticSamplers(0, ShaderBindStage::Compute, 1, &samplerDesc);

    m_pRootSignature = RootSignature::CreateRootSignature(L"SkyDomeRenderPass_RootSignature", signatureDesc);

    m_pParameters = ParameterSet::CreateParameterSet(m_pRootSignature);
    m_pParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(UpscalerInformation), 0);
    m_pParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(SkydomeCBData), 1);

    bool loadingTexture = false;
    if (m_pSkyTexture == nullptr)
    {
        // Load the texture data from which to create the texture
        TextureLoadCompletionCallbackFn CompletionCallback = [this](const std::vector<const Texture*>& textures, void* additionalParams = nullptr)
        {
            this->TextureLoadComplete(textures, additionalParams);
        };
        filesystem::path specPath = GetConfig()->StartupContent.SkyMap;
        GetContentManager()->LoadTexture(TextureLoadInfo(specPath), CompletionCallback);
        loadingTexture = true;
    }

    // Get the render target
    m_pRenderTarget = GetFramework()->GetColorTargetForCallback(GetName());
    CauldronAssert(ASSERT_CRITICAL, m_pRenderTarget != nullptr, L"Couldn't find or create the render target of SkyDomeRenderModule.");

    m_pParameters->SetTextureUAV(m_pRenderTarget, ViewDimension::Texture2D, 0);

    // Setup the pipeline object
    PipelineDesc psoDesc;
    psoDesc.SetRootSignature(m_pRootSignature);

    DefineList defineList;
    defineList.insert(std::make_pair(L"NUM_THREAD_X", std::to_wstring(g_NumThreadX)));
    defineList.insert(std::make_pair(L"NUM_THREAD_Y", std::to_wstring(g_NumThreadY)));

    // Setup the shaders to build on the pipeline object
    std::wstring shaderPath = L"skydome.hlsl";
    psoDesc.AddShaderDesc(ShaderBuildDesc::Compute(shaderPath.c_str(), L"MainCS", ShaderModel::SM6_6, &defineList));

    m_pPipelineObj = PipelineObject::CreatePipelineObject(L"SkydomeRenderPass_PipelineObj", psoDesc);

    m_ConstantData.ClipToWorld = Mat4::identity();

    // We are now ready for use
    if (!loadingTexture)
        SetModuleReady(true);
}

void SkyDomeRenderModule::InitProcedural()
{
    SetModuleReady(false);

    // root signature
    RootSignatureDesc signatureDesc;
    signatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1);
    signatureDesc.AddConstantBufferView(1, ShaderBindStage::Compute, 1);
    signatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);
    signatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);

    m_pRootSignature = RootSignature::CreateRootSignature(L"SkyDomeProcRenderPass_RootSignature", signatureDesc);

    // Get the render target and view
    m_pRenderTarget = GetFramework()->GetColorTargetForCallback(GetName());
    CauldronAssert(ASSERT_CRITICAL, m_pRenderTarget != nullptr, L"Couldn't find or create the render target of SkyDomeRenderModule.");

    // Setup the pipeline object
    PipelineDesc psoDesc;
    psoDesc.SetRootSignature(m_pRootSignature);

    DefineList defineList;
    defineList.insert(std::make_pair(L"NUM_THREAD_X", std::to_wstring(g_NumThreadX)));
    defineList.insert(std::make_pair(L"NUM_THREAD_Y", std::to_wstring(g_NumThreadY)));

    // Setup the shaders to build on the pipeline object
    std::wstring ShaderPath = L"skydomeproc.hlsl";
    psoDesc.AddShaderDesc(ShaderBuildDesc::Compute(ShaderPath.c_str(), L"MainCS", ShaderModel::SM6_6, &defineList));

    m_pPipelineObj = PipelineObject::CreatePipelineObject(L"SkydomeProcRenderPass_PipelineObj", psoDesc);

    m_pParameters = ParameterSet::CreateParameterSet(m_pRootSignature);
    m_pParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(UpscalerInformation), 0);
    m_pParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(ProceduralCBData), 1);
    m_pParameters->SetTextureUAV(m_pRenderTarget, ViewDimension::Texture2D, 0);

    if (m_pSkyTexture == nullptr)
    {
        // Load the texture data from which to create the texture
        TextureLoadCompletionCallbackFn CompletionCallback = [this](const std::vector<const Texture*>& textures,
                                                                                                   void* additionalParams = nullptr) {
            this->TextureLoadComplete(textures, additionalParams);
            // Set Scenes IBL Texture
            // TODO: Probes system
            GetScene()->SetIBLTexture(m_pSkyTexture, IBLTexture::Irradiance);
        };

        filesystem::path texturePath = GetConfig()->StartupContent.DiffuseIBL;
        GetContentManager()->LoadTexture(TextureLoadInfo(texturePath), CompletionCallback);
    }

    // initial values
    m_ProceduralConstantData.ClipToWorld = Mat4::identity();
    m_ProceduralConstantData.SunDirection = Vec3(1.0f, 0.05f, 0.0f);

    UISection uiSection;
    uiSection.SectionName = "Procedural SkyDome";
    uiSection.AddIntSlider("Hour", &GetScene()->GetSkydomeHour(), 5, 19);
    uiSection.AddIntSlider("Minute", &GetScene()->GetSkydomeMinute(), 0, 59);
    uiSection.AddFloatSlider("Rayleigh", &m_ProceduralConstantData.Rayleigh, 0.0f, 10.0f);
    uiSection.AddFloatSlider("Turbidity", &m_ProceduralConstantData.Turbidity, 0.0f, 25.0f);
    uiSection.AddFloatSlider("Mie Coefficient", &m_ProceduralConstantData.MieCoefficient, 0.0f, 0.01f);
    uiSection.AddFloatSlider("Luminance", &m_ProceduralConstantData.Luminance, 0.0f, 25.0f);
    uiSection.AddFloatSlider("Mie Directional G", &m_ProceduralConstantData.MieDirectionalG, 0.0f, 1.0f);

    GetUIManager()->RegisterUIElements(uiSection);
}

void SkyDomeRenderModule::InitSunlight()
{
    // Init data first as it's needed when continuing on the main thread
    m_SunlightCompData.Name = L"SkyDome ProceduralSunlight";

    // Pull in luminance
    m_SunlightCompData.Intensity = m_ProceduralConstantData.Luminance;
    m_SunlightCompData.Color = Vec3(1.0f, 1.0f, 1.0f);
    m_SunlightCompData.ShadowResolution = 1024;

    // Need to create our content on a background thread so proper notifiers can be called
    std::function<void(void*)> createContent = [this](void*)
    {
        ContentBlock* pContentBlock = new ContentBlock();

        // Memory backing light creation
        EntityDataBlock* pLightDataBlock = new EntityDataBlock();
        pContentBlock->EntityDataBlocks.push_back(pLightDataBlock);
        pLightDataBlock->pEntity = new Entity(m_SunlightCompData.Name.c_str());
        m_pSunlight = pLightDataBlock->pEntity;
        CauldronAssert(ASSERT_CRITICAL, m_pSunlight, L"Could not allocate skydome sunlight entity");

        // Calculate transform
        Mat4 lookAt = Mat4::lookAt(Point3(m_ProceduralConstantData.SunDirection), Point3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
        Mat4 transform = InverseMatrix(lookAt);
        m_pSunlight->SetTransform(transform);

        LightComponentData* pLightComponentData = new LightComponentData(m_SunlightCompData);
        pLightDataBlock->ComponentsData.push_back(pLightComponentData);
        m_pSunlightComponent = LightComponentMgr::Get()->SpawnLightComponent(m_pSunlight, pLightComponentData);
        pLightDataBlock->Components.push_back(m_pSunlightComponent);

        GetContentManager()->StartManagingContent(L"ProceduralSkydomeLightEntity", pContentBlock, false);

        // We are now ready for use
        SetModuleReady(true);
    };

    // Queue a task to create needed content after setup (but before run)
    Task createContentTask(createContent, nullptr);
    GetFramework()->AddContentCreationTask(createContentTask);
}

void SkyDomeRenderModule::UpdateSunDirection()
{
    // Parameters based on Japan 6/15
    float Lat = DEG_TO_RAD(35.0f); // Latitude
    float Decl = DEG_TO_RAD(23.0f + (17.0f / 60.0f)); // Sun declination at 6/15
    float Hour = DEG_TO_RAD((GetScene()->GetSkydomeHour() + static_cast<float>(GetScene()->GetSkydomeMinute() / 60.0f) - 12.0f) * 15.0f); // Hour angle at E135
    // Sin of solar altitude angle (2 / PI - solar zenith angle)
    float SinH = sinf(Lat) * sinf(Decl) + cosf(Lat) * cosf(Decl) * cosf(Hour);
    float CosH = sqrtf(1.0f - SinH * SinH);
    // Sin/Cos of solar azimuth angle
    float SinA = cosf(Decl) * sinf(Hour) / CosH;
    float CosA = (SinH * sinf(Lat) - sinf(Decl)) / (CosH * cosf(Decl));

    // Polar to Cartesian
    float x = CosA * CosH;
    float y = SinH;
    float z = SinA * CosH;

    // Update Proc shader parameters
    m_ProceduralConstantData.SunDirection = normalize(Vec3(x, y, z));

    // Update sunlight
    Mat4 lookAt = Mat4::lookAt(Point3(m_ProceduralConstantData.SunDirection), Point3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    Mat4 transform = InverseMatrix(lookAt);

    m_pSunlight->SetTransform(transform);
    m_pSunlightComponent->GetData().Intensity = m_ProceduralConstantData.Luminance;
    m_pSunlightComponent->SetDirty();
}

void SkyDomeRenderModule::TextureLoadComplete(const std::vector<const Texture*>& textureList, void*)
{
    // First texture to be used as sky texture
    m_pSkyTexture = textureList[0];
    CauldronAssert(ASSERT_CRITICAL, m_pSkyTexture != nullptr, L"SkyDomeRenderModule: Required texture could not be loaded. Terminating sample.");

    // Set our texture to the right parameter slot
    m_pParameters->SetTextureSRV(m_pSkyTexture, ViewDimension::TextureCube, 0);

    // When initialized as procedural sky, initialize directional light as well
    if (m_IsProcedural) {
        InitSunlight();
    }
    else {
        // We are now ready for use
        SetModuleReady(true);
    }
}

void SkyDomeRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture skydomeMarker(pCmdList, L"SkyDome");

    // Render modules expect resources coming in/going out to be in a shader read state
    Barrier barrier = Barrier::Transition(m_pRenderTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::UnorderedAccess);
    ResourceBarrier(pCmdList, 1, &barrier);

    BufferAddressInfo upscaleInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(UpscalerInformation), &GetScene()->GetSceneInfo().UpscalerInfo.FullScreenScaleRatio);
    m_pParameters->UpdateRootConstantBuffer(&upscaleInfo, 0);

    // Allocate a dynamic constant buffer and set
    BufferAddressInfo bufferInfo;
    if (m_IsProcedural)
    {
        UpdateSunDirection();
        m_ProceduralConstantData.ClipToWorld = GetScene()->GetCurrentCamera()->GetInverseViewProjection();
        bufferInfo                           = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(ProceduralCBData), &m_ProceduralConstantData);
    }
    else
    {
        m_ConstantData.ClipToWorld = GetScene()->GetCurrentCamera()->GetInverseViewProjection();
        bufferInfo                 = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(SkydomeCBData), &m_ConstantData);
    }

    m_pParameters->UpdateRootConstantBuffer(&bufferInfo, 1);
    // Bind all the parameters
    m_pParameters->Bind(pCmdList, m_pPipelineObj);

    SetPipelineState(pCmdList, m_pPipelineObj);

    const uint32_t numGroupX = DivideRoundingUp(m_pRenderTarget->GetDesc().Width, g_NumThreadX);
    const uint32_t numGroupY = DivideRoundingUp(m_pRenderTarget->GetDesc().Height, + g_NumThreadY);
    Dispatch(pCmdList, numGroupX, numGroupY, 1);

    // Render modules expect resources coming in/going out to be in a shader read state
    barrier = Barrier::Transition(
        m_pRenderTarget->GetResource(), ResourceState::UnorderedAccess, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    ResourceBarrier(pCmdList, 1, &barrier);
}
