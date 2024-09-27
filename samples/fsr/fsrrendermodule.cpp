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

#include "fsrrendermodule.h"
#include "render/rendermodules/ui/uirendermodule.h"
#include "render/rasterview.h"
#include "render/dynamicresourcepool.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/rootsignature.h"
#include "render/swapchain.h"
#include "render/resourceviewallocator.h"
#include "core/backend_interface.h"
#include "core/scene.h"
#include "misc/assert.h"
#include "render/profiler.h"
#include "taa/taarendermodule.h"
#include "translucency/translucencyrendermodule.h"

#include <sstream>
#include <functional>

#define FFX_CPU
#include <FidelityFX/gpu/fsr3upscaler/ffx_fsr3upscaler_resources.h>

using namespace std;
using namespace cauldron;

FSRRenderModule* FSRRenderModule::s_FSRRMInstance(nullptr);
constexpr uint32_t g_NumThreadX = 32;
constexpr uint32_t g_NumThreadY = 32;

void FSRRenderModule::Init(const json& initData)
{
    CauldronAssert(ASSERT_CRITICAL, !s_FSRRMInstance, L"Multiple instances of FSRRenderModule being created.");
    s_FSRRMInstance = this;

    m_pTAARenderModule   = static_cast<TAARenderModule*>(GetFramework()->GetRenderModule("TAARenderModule"));
    m_pTransRenderModule = static_cast<TranslucencyRenderModule*>(GetFramework()->GetRenderModule("TranslucencyRenderModule"));
    m_pToneMappingRenderModule = static_cast<ToneMappingRenderModule*>(GetFramework()->GetRenderModule("ToneMappingRenderModule"));
    CauldronAssert(ASSERT_CRITICAL, m_pTAARenderModule, L"FidelityFX FSR Sample: Error: Could not find TAA render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pTransRenderModule, L"FidelityFX FSR Sample: Error: Could not find Translucency render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pToneMappingRenderModule, L"FidelityFX FSR Sample: Error: Could not find Tone Mapping render module.");

    // Fetch needed resources
    m_pColorTarget           = GetFramework()->GetColorTargetForCallback(GetName());
    m_pTonemappedColorTarget = GetFramework()->GetRenderTexture(L"SwapChainProxy");
    m_pDepthTarget           = GetFramework()->GetRenderTexture(L"DepthTarget");
    m_pMotionVectors         = GetFramework()->GetRenderTexture(L"GBufferMotionVectorRT");
    m_pReactiveMask          = GetFramework()->GetRenderTexture(L"ReactiveMask");
    m_pCompositionMask       = GetFramework()->GetRenderTexture(L"TransCompMask");
    CauldronAssert(ASSERT_CRITICAL, m_pMotionVectors && m_pReactiveMask && m_pCompositionMask, L"Could not get one of the needed resources for FSR Rendermodule.");

    // Set the format to use for the intermediate resource when RCAS is enabled
    switch (m_pTonemappedColorTarget->GetFormat())
    {
    case ResourceFormat::RGBA16_FLOAT:
        m_FSR1InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        break;

    case ResourceFormat::RGBA8_UNORM:
        m_FSR1InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
        break;

    case ResourceFormat::RGB10A2_UNORM:
        m_FSR1InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
        break;
        // For all other instances, just use floating point 32-bit format
    default:
        m_FSR1InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
        break;
    }

    // Get a CPU resource view that we'll use to map the render target to
    GetResourceViewAllocator()->AllocateCPURenderViews(&m_pRTResourceView);

    // Create render resolution opaque render target to use for auto-reactive mask generation
    TextureDesc desc = m_pColorTarget->GetDesc();
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    desc.Width = resInfo.RenderWidth;
    desc.Height = resInfo.RenderHeight;
    desc.Name = L"FSR_OpaqueTexture";
    m_pOpaqueTexture = GetDynamicResourcePool()->CreateRenderTexture(&desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight)
        {
            desc.Width  = renderingWidth;
            desc.Height = renderingHeight;
        });

    {
        TextureDesc           desc    = m_pTonemappedColorTarget->GetDesc();
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        desc.Width                    = resInfo.RenderWidth;
        desc.Height                   = resInfo.RenderHeight;
        desc.Name                     = L"FSR1_TmpTexture";
        m_pFsr1TmpTexture             = GetDynamicResourcePool()->CreateRenderTexture(
            &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
                desc.Width  = displayWidth;
                desc.Height = displayHeight;
            });
    }
    // Register additional exports for translucency pass
    BlendDesc      reactiveCompositionBlend = {
        true, Blend::InvDstColor, Blend::One, BlendOp::Add, Blend::One, Blend::Zero, BlendOp::Add, static_cast<uint32_t>(ColorWriteMask::Red)};

    OptionalTransparencyOptions transOptions;
    transOptions.OptionalTargets.push_back(std::make_pair(m_pReactiveMask, reactiveCompositionBlend));
    transOptions.OptionalTargets.push_back(std::make_pair(m_pCompositionMask, reactiveCompositionBlend));
    transOptions.OptionalAdditionalOutputs = L"float ReactiveTarget : SV_TARGET1; float CompositionTarget : SV_TARGET2;";
    transOptions.OptionalAdditionalExports =
        L"float hasAnimatedTexture = 0.f; output.ReactiveTarget = ReactiveMask; output.CompositionTarget = max(Alpha, hasAnimatedTexture);";

    // Add additional exports for FSR to translucency pass
    m_pTransRenderModule->AddOptionalTransparencyOptions(transOptions);

    // Create temporary resource for spacial upscale
    {
        // Create the render target and validate it was created
        TextureDesc desc = m_pColorTarget->GetDesc();
        desc.Name        = L"UpscaleIntermediateTarget";
        desc.Width       = m_pColorTarget->GetDesc().Width;
        desc.Height      = m_pColorTarget->GetDesc().Height;

        m_pTempTexture = GetDynamicResourcePool()->CreateRenderTexture(
            &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
                desc.Width  = displayWidth;
                desc.Height = displayHeight;
            });
        CauldronAssert(ASSERT_CRITICAL, m_pTempTexture, L"Couldn't create intermediate texture.");

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
        std::wstring       shaderPath = L"upscalecs.hlsl";
        DefineList         defineList;
        // Setup the shaders to build on the pipeline object
        defineList.insert(std::make_pair(L"NUM_THREAD_X", std::to_wstring(g_NumThreadX)));
        defineList.insert(std::make_pair(L"NUM_THREAD_Y", std::to_wstring(g_NumThreadY)));

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
            m_pParameters->SetTextureUAV(m_pColorTarget, ViewDimension::Texture2D, 0);
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
            m_pParametersBicubic->SetTextureUAV(m_pColorTarget, ViewDimension::Texture2D, 0);
            // Set our texture to the right parameter slot
            m_pParametersBicubic->SetTextureSRV(m_pTempTexture, ViewDimension::Texture2D, 0);

            // Update sampler bindings as well
            m_pParametersBicubic->SetSampler(m_pLinearSampler, 0);
        }
    }
       
    // Create raster views on the reactive mask and composition masks (for clearing and rendering)
    m_RasterViews.resize(2);
    m_RasterViews[0] = GetRasterViewAllocator()->RequestRasterView(m_pReactiveMask, ViewDimension::Texture2D);
    m_RasterViews[1] = GetRasterViewAllocator()->RequestRasterView(m_pCompositionMask, ViewDimension::Texture2D);

    // Set our render resolution function as that to use during resize to get render width/height from display width/height
    m_pUpdateFunc = [this](uint32_t displayWidth, uint32_t displayHeight) { return this->UpdateResolution(displayWidth, displayHeight); };

    //////////////////////////////////////////////////////////////////////////
    // Register additional execution callbacks during the frame

    // Register a post-lighting callback to copy opaque texture
    ExecuteCallback callbackPreTrans = [this](double deltaTime, CommandList* pCmdList) {
        this->PreTransCallback(deltaTime, pCmdList);
    };
    ExecutionTuple callbackPreTransTuple = std::make_pair(L"FSRRenderModule::PreTransCallback", std::make_pair(this, callbackPreTrans));
    GetFramework()->RegisterExecutionCallback(L"LightingRenderModule", false, callbackPreTransTuple);

    // Register a post-transparency callback to generate reactive mask
    ExecuteCallback callbackPostTrans = [this](double deltaTime, CommandList* pCmdList) {
        this->PostTransCallback(deltaTime, pCmdList);
    };
    ExecutionTuple callbackPostTransTuple = std::make_pair(L"FSRRenderModule::PostTransCallback", std::make_pair(this, callbackPostTrans));
    GetFramework()->RegisterExecutionCallback(L"TranslucencyRenderModule", false, callbackPostTransTuple);

    m_curUiTextureIndex     = 0;
    
    // Get the proper UI color target
    m_pUiTexture[0] = GetFramework()->GetRenderTexture(L"UITarget0");
    m_pUiTexture[1] = GetFramework()->GetRenderTexture(L"UITarget1");
    
    // Create FrameInterpolationSwapchain
    // Separate from FSR3 generation so it can be done when the engine creates the swapchain
    // should not be created and destroyed with FSR3, as it requires a switch to windowed mode

    SDKWrapper::ffxSetupFrameInterpolationSwapChain();

    // Fetch hudless texture resources
    m_pHudLessTexture[0] = GetFramework()->GetRenderTexture(L"HudlessTarget0");
    m_pHudLessTexture[1] = GetFramework()->GetRenderTexture(L"HudlessTarget1");

    // Start disabled as this will be enabled externally
    cauldron::RenderModule::SetModuleEnabled(false);
    GetFramework()->ConfigureRuntimeShaderRecompiler(
        [this](void) {
            m_PreShaderReloadEnabledState = ModuleEnabled();
            if (m_PreShaderReloadEnabledState)
            {
                EnableModule(false);
                SDKWrapper::ffxRestoreApplicationSwapChain();
            }
        },
        [this](void) {
            if (m_PreShaderReloadEnabledState)
            {
                SDKWrapper::ffxSetupFrameInterpolationSwapChain();
                EnableModule(true);
            }
        });

    {
        // Register upscale method picker picker
        UISection* uiSection = GetUIManager()->RegisterUIElements("Upscaling", UISectionType::Sample);
        InitUI(uiSection);
    }
    //////////////////////////////////////////////////////////////////////////
    // Finish up init

    // That's all we need for now
    SetModuleReady(true);
}

FSRRenderModule::~FSRRenderModule()
{
    UpdateFSRContexts(false);
    if (m_FSR1InitializationParameters.backendInterface.scratchBuffer)
    {
        free(m_FSR1InitializationParameters.backendInterface.scratchBuffer);
        m_FSR1InitializationParameters.backendInterface.scratchBuffer = nullptr;
    }
    if (m_FSR2InitializationParameters.backendInterface.scratchBuffer)
    {
        free(m_FSR2InitializationParameters.backendInterface.scratchBuffer);
        m_FSR2InitializationParameters.backendInterface.scratchBuffer = nullptr;
    }

    if (m_ffxBackendInitialized == true)
    {
        for (auto i = 0; i < FSR3_BACKEND_COUNT; i++)
        {
            free(m_ffxFsr3Backends[i].scratchBuffer);
            m_ffxFsr3Backends[i].scratchBuffer = nullptr;
        }
        m_ffxBackendInitialized = false;
    }

    delete m_pParameters;
    delete m_pPointSampler;
    delete m_pLinearSampler;
    delete m_pPipelineObj;
    delete m_pRootSignature;
    delete m_pParametersBicubic;
    delete m_pPipelineObjBicubic;
    delete m_pRootSignatureBicubic;

    m_pParameters = nullptr;
    m_pPointSampler = nullptr;
    m_pLinearSampler = nullptr;
    m_pPipelineObj = nullptr;
    m_pRootSignature = nullptr;
    m_pParametersBicubic = nullptr;
    m_pPipelineObjBicubic = nullptr;
    m_pRootSignatureBicubic = nullptr;

    // Restore the application's swapchain
    SDKWrapper::ffxRestoreApplicationSwapChain();

    // Clear out our FSR3Instance so it can be reset if needed
    s_FSRRMInstance = nullptr;
}

void FSRRenderModule::EnableModule(bool enabled)
{
    // If disabling the render module, we need to disable the upscaler with the framework
    if (!enabled)
    {
        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        // Destroy the upscaling context
        UpdateFSRContexts(false);

        if (GetFramework()->UpscalerEnabled())
            GetFramework()->EnableUpscaling(false);
        if (GetFramework()->FrameInterpolationEnabled())
            GetFramework()->EnableFrameInterpolation(false);

        UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        uimod->SetAsyncRender(false);
        uimod->SetRenderToTexture(false);
        uimod->SetCopyHudLessTexture(false);

        // Destroy the FidelityFX interface memory
        if (m_FSR1InitializationParameters.backendInterface.scratchBuffer)
        {
            free(m_FSR1InitializationParameters.backendInterface.scratchBuffer);
            m_FSR1InitializationParameters.backendInterface.scratchBuffer = nullptr;
        }
        if (m_FSR2InitializationParameters.backendInterface.scratchBuffer)
        {
            free(m_FSR2InitializationParameters.backendInterface.scratchBuffer);
            m_FSR2InitializationParameters.backendInterface.scratchBuffer = nullptr;
        }

        if (m_ffxBackendInitialized == true)
        {
            for (auto i = 0; i < FSR3_BACKEND_COUNT; i++)
            {
                free(m_ffxFsr3Backends[i].scratchBuffer);
                m_ffxFsr3Backends[i].scratchBuffer = nullptr;
            }
            m_ffxBackendInitialized = false;
        }

        CameraComponent::SetJitterCallbackFunc(nullptr);
    }
    else
    {
        UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        uimod->SetAsyncRender(s_uiRenderMode == 2);
        uimod->SetRenderToTexture(s_uiRenderMode == 1);
        uimod->SetCopyHudLessTexture(s_uiRenderMode == 3);

        // Setup everything needed when activating FSR
        // Will also enable upscaling
        UpdatePreset(nullptr);
        
        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        // Setup FSR1 FidelityFX interface.
        if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR1)
        {
            const size_t scratchBufferSize = SDKWrapper::ffxGetScratchMemorySize(FFX_FSR1_CONTEXT_COUNT);
            void*        scratchBuffer     = calloc(scratchBufferSize, 1);
            FfxErrorCode errorCode         = SDKWrapper::ffxGetInterface(
                &m_FSR1InitializationParameters.backendInterface, GetDevice(), scratchBuffer, scratchBufferSize, FFX_FSR1_CONTEXT_COUNT);
            CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Could not initialize the FidelityFX SDK backend");

            CauldronAssert(ASSERT_CRITICAL, m_FSR1InitializationParameters.backendInterface.fpGetSDKVersion(&m_FSR1InitializationParameters.backendInterface) ==
                                   FFX_SDK_MAKE_VERSION(1, 1, 1), L"FidelityFX FSR 2.1 sample requires linking with a 1.1.1 version SDK backend");
            CauldronAssert(ASSERT_CRITICAL, ffxFsr1GetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 2, 0),
                               L"FidelityFX FSR 2.1 sample requires linking with a 1.2 version FidelityFX FSR1 library");

            // Use our thread-safe buffer allocator instead of the default one
            m_FSR1InitializationParameters.backendInterface.fpRegisterConstantBufferAllocator(&m_FSR1InitializationParameters.backendInterface,
                                                                                          SDKWrapper::ffxAllocateConstantBuffer);

        }
        
        // Setup FSR2 FidelityFX interface.
        if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR2)
        {
            const size_t scratchBufferSize = SDKWrapper::ffxGetScratchMemorySize(FFX_FSR2_CONTEXT_COUNT);
            void*        scratchBuffer     = calloc(scratchBufferSize, 1);
            FfxErrorCode errorCode         = SDKWrapper::ffxGetInterface(
                &m_FSR2InitializationParameters.backendInterface, GetDevice(), scratchBuffer, scratchBufferSize, FFX_FSR2_CONTEXT_COUNT);
            CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Could not initialize the FidelityFX SDK backend");

            CauldronAssert(ASSERT_CRITICAL, m_FSR2InitializationParameters.backendInterface.fpGetSDKVersion(&m_FSR2InitializationParameters.backendInterface) ==
                                   FFX_SDK_MAKE_VERSION(1, 1, 1), L"FidelityFX FSR 2.1 sample requires linking with a 1.1.1 version SDK backend");
            CauldronAssert(ASSERT_CRITICAL, ffxFsr2GetEffectVersion() == FFX_SDK_MAKE_VERSION(2, 3, 2),
                               L"FidelityFX FSR 2.1 sample requires linking with a 2.3.2 version FidelityFX FSR2 library");

            // Use our thread-safe buffer allocator instead of the default one
            m_FSR2InitializationParameters.backendInterface.fpRegisterConstantBufferAllocator(&m_FSR2InitializationParameters.backendInterface,
                                                                                          SDKWrapper::ffxAllocateConstantBuffer);
        }

        // Setup FSR3 FidelityFX interface.
        if (!m_ffxBackendInitialized)
        {
            FfxErrorCode errorCode = 0;

            int effectCounts[] = {1, 2};
            for (auto i = 0; i < FSR3_BACKEND_COUNT; i++)
            {
                const size_t scratchBufferSize = SDKWrapper::ffxGetScratchMemorySize(effectCounts[i]);
                void*        scratchBuffer     = calloc(scratchBufferSize, 1);
                memset(scratchBuffer, 0, scratchBufferSize);
                errorCode |= SDKWrapper::ffxGetInterface(&m_ffxFsr3Backends[i], GetDevice(), scratchBuffer, scratchBufferSize, effectCounts[i]);

                if (errorCode == FFX_OK)
                {
                    // validate backend version being used
                    CauldronAssert(ASSERT_CRITICAL, m_ffxFsr3Backends[i].fpGetSDKVersion(&m_ffxFsr3Backends[i]) == FFX_SDK_MAKE_VERSION(1, 1, 1),
                                       L"FidelityFX FSR 2.1 sample requires linking with a 1.1 version SDK backend");
                    CauldronAssert(ASSERT_CRITICAL, ffxFsr3UpscalerGetEffectVersion() == FFX_SDK_MAKE_VERSION(3, 1, 1),
                                       L"FidelityFX FSR 2.1 sample requires linking with a 3.1.0 version FidelityFX FSR3Upscaler library");
                    CauldronAssert(ASSERT_CRITICAL, ffxFsr3GetEffectVersion() == FFX_SDK_MAKE_VERSION(3, 1, 1),
                                       L"FidelityFX FSR 2.1 sample requires linking with a 3.1.0 version FidelityFX FSR3 library");
                    CauldronAssert(ASSERT_CRITICAL, ffxFrameInterpolationGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 1, 0),
                                       L"FidelityFX FSR 2.1 sample requires linking with a 1.1.1 version FidelityFX FrameInterpolation library");
                    CauldronAssert(ASSERT_CRITICAL, ffxOpticalflowGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 1, 1),
                                       L"FidelityFX FSR 2.1 sample requires linking with a 1.0.1 version FidelityFX OpticalFlow library");

                    // Use our thread-safe buffer allocator instead of the default one
                    m_ffxFsr3Backends[i].fpRegisterConstantBufferAllocator(&m_ffxFsr3Backends[i], SDKWrapper::ffxAllocateConstantBuffer);
                }
            }

            m_ffxBackendInitialized = (errorCode == FFX_OK);
            CAULDRON_ASSERT(m_ffxBackendInitialized);

            m_FSR3InitializationParameters.backendInterfaceUpscaling       = m_ffxFsr3Backends[FSR3_BACKEND_UPSCALING];
            m_FSR3InitializationParameters.backendInterfaceFrameInterpolation = m_ffxFsr3Backends[FSR3_BACKEND_FRAME_INTERPOLATION];
        }

        // Create the FSR contexts
        UpdateFSRContexts(true);

        if (m_UpscaleMethod > UpscaleRM::UpscaleMethod::FSR1)
        {
            // Set the jitter callback to use
            CameraJitterCallback jitterCallback = [this](Vec2& values) {
                // Increment jitter index for frame
                ++m_JitterIndex;

                // Update FSR3 jitter for built in TAA
                const ResolutionInfo& resInfo          = GetFramework()->GetResolutionInfo();
                const int32_t         jitterPhaseCount = ffxFsr3GetJitterPhaseCount(resInfo.RenderWidth, resInfo.DisplayWidth);
                ffxFsr3GetJitterOffset(&m_JitterX, &m_JitterY, m_JitterIndex, jitterPhaseCount);

                values = Vec2(-2.f * m_JitterX / resInfo.RenderWidth, 2.f * m_JitterY / resInfo.RenderHeight);
            };
            CameraComponent::SetJitterCallbackFunc(jitterCallback);
        }
        ClearReInit();
    }

    // Show or hide UI elements for active upscaler
    for (auto& i : m_UIElements)
    {
        i->Show(enabled);
    }
}

void FSRRenderModule::InitUI(UISection* pUISection)
{
    // Setup upscale method options
    std::vector<const char*> comboOptions = {"Native", "Point", "Bilinear", "Bicubic", "FSR1", "FSR2", "FSR3"};

    // Add the section header
    pUISection->RegisterUIElement<UICombo>(
        "Method", (int32_t&)m_UIMethod, std::move(comboOptions), [this](int32_t cur, int32_t old) { SwitchUpscaler((UpscaleRM::UpscaleMethod)cur); });

    // Setup scale preset options
    std::vector<const char*> presetComboOptions = {
        "Native AA (1.0x)", "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Ultra Performance (3x)", "Custom", "DRS"};
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "Scale Preset", (int32_t&)m_ScalePreset, std::move(presetComboOptions), m_IsNonNative, [this](int32_t cur, int32_t old) { UpdatePreset(&old); }, false));
    
    // Setup mip bias
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Mip LOD Bias",
        m_MipBias,
        -5.f, 0.f,
        [this](float cur, float old) {
            UpdateMipBias(&old);
        },
        false
    ));

    // Setup scale factor (disabled for all but custom)
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Custom Scale",
        m_UpscaleRatio,
        1.f, 3.f,
        m_UpscaleRatioEnabled,
        [this](float cur, float old) {
            UpdateUpscaleRatio(&old);
        },
        false
    ));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Letterbox size", m_LetterboxRatio, 0.f, 1.f, [this](float cur, float old) { UpdateUpscaleRatio(&old); }, false));


    m_UIElements.emplace_back(pUISection->RegisterUIElement<UIText>("NOT_SET"));
    m_UIInfoTextIndex = m_UIElements.size() - 1;
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw upscaler debug view", m_DrawUpscalerDebugView, nullptr, false));

    // Reactive mask
    std::vector<const char*> maskComboOptions{ "Disabled", "Manual Reactive Mask Generation", "Autogen FSR2 Helper Function" };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>("Reactive Mask Mode", (int32_t&)m_MaskMode, std::move(maskComboOptions), m_EnableMaskOptions, nullptr, false));

    // Use mask
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Use Reactive Mask", m_UseReactiveMask, m_EnableMaskOptions, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Use Transparency and Composition Mask", m_UseTnCMask, m_EnableMaskOptions, nullptr, false));

    // Sharpening
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("RCAS Sharpening", m_RCASSharpen, nullptr, false, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>("Sharpness", m_Sharpness, 0.f, 1.f, m_RCASSharpen, nullptr, false));

    // Frame interpolation
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Frame Interpolation", m_FrameInterpolation, 
        [this](bool, bool)
        {
            m_OfUiEnabled = m_FrameInterpolation && s_enableSoftwareMotionEstimation;

            GetFramework()->EnableFrameInterpolation(m_FrameInterpolation);

            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        }, 
        false));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Support Async Compute", m_PendingEnableAsyncCompute,
        [this](bool, bool) 
        {
            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        }, 
        false));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Allow async compute", m_AllowAsyncCompute, m_PendingEnableAsyncCompute, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Use callback", m_UseCallback, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw frame generation tear lines", m_DrawFrameGenerationDebugTearLines, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw frame generation debug view", m_DrawFrameGenerationDebugView, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Present interpolated only", m_PresentInterpolatedOnly, m_FrameInterpolation, nullptr, false));
    
    std::vector<const char*>        uiRenderModeLabels = { "No UI handling (not recommended)", "UiTexture", "UiCallback", "Pre-Ui Backbuffer" };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "UI Composition Mode",
        s_uiRenderMode,
        std::move(uiRenderModeLabels), 
        m_FrameInterpolation,
        [this](int32_t, int32_t) 
        {
            UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
            uimod->SetAsyncRender(s_uiRenderMode == 2);
            uimod->SetRenderToTexture(s_uiRenderMode == 1);
            uimod->SetCopyHudLessTexture(s_uiRenderMode == 3);

            m_NeedReInit = true;
        },
        false));
    
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("DoubleBuffer UI resource in swapchain", m_DoublebufferInSwapchain, m_FrameInterpolation, nullptr, false));

    m_UIMethod = UpscaleRM::UpscaleMethod::FSR3;

    SwitchUpscaler(m_UIMethod);
}

void FSRRenderModule::SwitchUpscaler(UpscaleRM::UpscaleMethod newUpscaler)
{
    // Flush everything out of the pipe before disabling/enabling things
    GetDevice()->FlushAllCommandQueues();

    if (ModuleEnabled())
        EnableModule(false);

    // If old upscaler was FSR2, re-enable TAA render module as FSR2 disables it

    SetFilter(static_cast<UpscaleRM::UpscaleMethod>(newUpscaler));
    switch (newUpscaler)
    {
    case UpscaleRM::UpscaleMethod::Native:
    case UpscaleRM::UpscaleMethod::Point:
    case UpscaleRM::UpscaleMethod::Bilinear:
    case UpscaleRM::UpscaleMethod::Bicubic:
        m_pTAARenderModule->EnableModule(false);
        m_pToneMappingRenderModule->EnableModule(true);
        m_EnableMaskOptions = false;
        break;
    case UpscaleRM::UpscaleMethod::FSR1:
        m_pTAARenderModule->EnableModule(true);
        m_pToneMappingRenderModule->EnableModule(false);  // will execute manually before FSR1
        m_EnableMaskOptions = false;
        break;
    case UpscaleRM::UpscaleMethod::FSR2:
    case UpscaleRM::UpscaleMethod::FSR3:
        ClearReInit();
        // Also disable TAA render module
        m_pTAARenderModule->EnableModule(false);
        m_pToneMappingRenderModule->EnableModule(true);
        m_EnableMaskOptions = true;
        break;
    default:
        CauldronCritical(L"Unsupported upscaler requested.");
        break;
    }

    m_UpscaleMethod = newUpscaler;

    // Enable the new one
    EnableModule(true);
    ClearReInit();
}

void FSRRenderModule::UpdatePreset(const int32_t* pOldPreset)
{
    switch (m_ScalePreset)
    {
    case FSRScalePreset::DRS:
    case FSRScalePreset::NativeAA:
        m_UpscaleRatio = 1.0f;
        break;
    case FSRScalePreset::Quality:
        m_UpscaleRatio = 1.5f;
        break;
    case FSRScalePreset::Balanced:
        m_UpscaleRatio = 1.7f;
        break;
    case FSRScalePreset::Performance:
        m_UpscaleRatio = 2.0f;
        break;
    case FSRScalePreset::UltraPerformance:
        m_UpscaleRatio = 3.0f;
        break;
    case FSRScalePreset::Custom:
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update whether we can update the custom scale slider
    m_UpscaleRatioEnabled = (m_ScalePreset == FSRScalePreset::Custom);
    m_DynamicRes          = (m_ScalePreset == FSRScalePreset::DRS);

    // Update mip bias
    float oldValue = m_MipBias;
    if (m_ScalePreset != FSRScalePreset::Custom)
        m_MipBias = cMipBias[static_cast<uint32_t>(m_ScalePreset)];
    else
        m_MipBias = CalculateMipBias(m_UpscaleRatio);
    UpdateMipBias(&oldValue);

    // Update resolution since rendering ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);

    GetFramework()->EnableFrameInterpolation(m_FrameInterpolation);
}

void FSRRenderModule::UpdateUpscaleRatio(const float* pOldRatio)
{
    // Disable/Enable FSR3 since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSRRenderModule::UpdateMipBias(const float* pOldBias)
{
    // Update the scene MipLODBias to use
    GetScene()->SetMipLODBias(m_MipBias);
}

void FSRRenderModule::FfxMsgCallback(FfxMsgType type, const wchar_t* message)
{
    if (type == FFX_MESSAGE_TYPE_ERROR)
    {
        CauldronError(L"FSR3_API_DEBUG_ERROR: %s", message);
    }
    else if (type == FFX_MESSAGE_TYPE_WARNING)
    {
        CauldronWarning(L"FSR3_API_DEBUG_WARNING: %s", message);
    }
}

FfxErrorCode FSRRenderModule::UiCompositionCallback(const FfxPresentCallbackDescription* params, void*)
{
    if (s_FSRRMInstance->s_uiRenderMode != 2)
        return FFX_ERROR_INVALID_ARGUMENT;

    // Get a handle to the UIRenderModule for UI composition (only do this once as there's a search cost involved
    if (!s_FSRRMInstance->m_pUIRenderModule)
    {
        s_FSRRMInstance->m_pUIRenderModule = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        CauldronAssert(ASSERT_CRITICAL, s_FSRRMInstance->m_pUIRenderModule, L"Could not get a handle to the UIRenderModule.");
    }
    
    // Wrap everything in cauldron wrappers to allow backend agnostic execution of UI render.
    CommandList* pCmdList = CommandList::GetWrappedCmdListFromSDK(L"UI_CommandList", CommandQueue::Graphics, params->commandList);
    SetAllResourceViewHeaps(pCmdList); // Set the resource view heaps to the wrapped command list

    ResourceState rtResourceState = SDKWrapper::GetFrameworkState(params->outputSwapChainBuffer.state);
    ResourceState bbResourceState = SDKWrapper::GetFrameworkState(params->currentBackBuffer.state);

    TextureDesc  rtDesc = SDKWrapper::GetFrameworkTextureDescription(params->outputSwapChainBuffer.description);
    TextureDesc  bbDesc = SDKWrapper::GetFrameworkTextureDescription(params->currentBackBuffer.description);

    GPUResource* pRTResource = GPUResource::GetWrappedResourceFromSDK(L"UI_RenderTarget", params->outputSwapChainBuffer.resource, &rtDesc, rtResourceState);
    GPUResource* pBBResource = GPUResource::GetWrappedResourceFromSDK(L"BackBuffer", params->currentBackBuffer.resource, &bbDesc, bbResourceState);
    
    std::vector<Barrier> barriers;
    barriers = {Barrier::Transition(pRTResource, rtResourceState, ResourceState::CopyDest),
                Barrier::Transition(pBBResource, bbResourceState, ResourceState::CopySource)};
    ResourceBarrier(pCmdList, (uint32_t)barriers.size(), barriers.data());
        
    TextureCopyDesc copyDesc(pBBResource, pRTResource);
    CopyTextureRegion(pCmdList, &copyDesc);
    
    barriers[0].SourceState = barriers[0].DestState;
    barriers[0].DestState   = ResourceState::RenderTargetResource;
    swap(barriers[1].SourceState, barriers[1].DestState);
    ResourceBarrier(pCmdList, (uint32_t)barriers.size(), barriers.data());

    // Create and set RTV, required for async UI render.
    FfxResourceDescription rdesc = params->outputSwapChainBuffer.description;

    TextureDesc rtResourceDesc = TextureDesc::Tex2D(L"UI_RenderTarget",
                                                    SDKWrapper::GetFrameworkSurfaceFormat(rdesc.format),
                                                    rdesc.width,
                                                    rdesc.height,
                                                    rdesc.depth,
                                                    rdesc.mipCount,
                                                    ResourceFlags::AllowRenderTarget);
    s_FSRRMInstance->m_pRTResourceView->BindTextureResource(pRTResource, rtResourceDesc, ResourceViewType::RTV, ViewDimension::Texture2D, 0, 1, 0);
    
    s_FSRRMInstance->m_pUIRenderModule->ExecuteAsync(pCmdList, &s_FSRRMInstance->m_pRTResourceView->GetViewInfo(0));

    ResourceBarrier(pCmdList, 1, &Barrier::Transition(pRTResource, ResourceState::RenderTargetResource, rtResourceState));

    // Clean up wrapped resources for the frame
    delete pBBResource;
    delete pRTResource;
    delete pCmdList;

    return FFX_OK;
}

void FSRRenderModule::UpdateFSRContexts(bool enabled)
{
    std::stringstream infoText;

    if (enabled)
    {
        auto appendMemoryUsageText = [&](std::string text, FfxEffectMemoryUsage usage) {
            float totalUsageInMB = usage.totalUsageInBytes / 1000.0f / 1000.0f;
            float aliasableUsageInMB = usage.aliasableUsageInBytes / 1000.0f / 1000.0f;
            infoText << text << std::fixed << std::setprecision(2) << totalUsageInMB << " MB  (" << aliasableUsageInMB << " aliasable)" << std::endl;
        };

        FfxErrorCode errorCode = FFX_OK;
        // FSR1
        if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR1)
        {
            // HDR?
            if (GetSwapChain()->GetSwapChainDisplayMode() != DisplayMode::DISPLAYMODE_LDR)
                m_FSR1InitializationParameters.flags |= FFX_FSR1_ENABLE_HIGH_DYNAMIC_RANGE;
            else
                m_FSR1InitializationParameters.flags &= ~FFX_FSR1_ENABLE_HIGH_DYNAMIC_RANGE;

            // Apply RCAS?
            if (m_RCASSharpen)
                m_FSR1InitializationParameters.flags |= FFX_FSR1_ENABLE_RCAS;
            else
                m_FSR1InitializationParameters.flags &= ~FFX_FSR1_ENABLE_RCAS;

            const ResolutionInfo& resInfo                       = GetFramework()->GetResolutionInfo();
            m_FSR1InitializationParameters.maxRenderSize.width  = resInfo.DisplayWidth;
            m_FSR1InitializationParameters.maxRenderSize.height = resInfo.DisplayHeight;
            m_FSR1InitializationParameters.displaySize.width    = resInfo.DisplayWidth;
            m_FSR1InitializationParameters.displaySize.height   = resInfo.DisplayHeight;

            // Create the FSR1 context
            errorCode = ffxFsr1ContextCreate(&m_FSR1Context, &m_FSR1InitializationParameters);
            CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Couldn't create the FidelityFX SDK FSR1 context.");

            // get GPU memory usage
            FfxEffectMemoryUsage gpuMemoryUsageUpscaler;
            ffxFsr1ContextGetGpuMemoryUsage(&m_FSR1Context, &gpuMemoryUsageUpscaler);

            appendMemoryUsageText("VRAM FSR1 Upscaler:\t\t", gpuMemoryUsageUpscaler);
        }
        
        // FSR2
        if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR2)
        {
            const ResolutionInfo& resInfo                   = GetFramework()->GetResolutionInfo();
            m_FSR2InitializationParameters.maxRenderSize.width  = resInfo.DisplayWidth;
            m_FSR2InitializationParameters.maxRenderSize.height = resInfo.DisplayHeight;
            m_FSR2InitializationParameters.displaySize.width    = resInfo.DisplayWidth;
            m_FSR2InitializationParameters.displaySize.height   = resInfo.DisplayHeight;

            // Enable auto-exposure by default
            m_FSR2InitializationParameters.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;

            // Note, inverted depth and display mode are currently handled statically for the run of the sample.
            // If they become changeable at runtime, we'll need to modify how this information is queried
            static bool s_InvertedDepth = GetConfig()->InvertedDepth;

            // Setup inverted depth flag according to sample usage
            if (s_InvertedDepth)
                m_FSR2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_DEPTH_INFINITE;

            // Input data is HDR
            m_FSR2InitializationParameters.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;

// Do eror checking in debug
#if defined(_DEBUG)
            m_FSR2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
            m_FSR2InitializationParameters.fpMessage = &FSRRenderModule::FfxMsgCallback;
#endif  // #if defined(_DEBUG)

            // Create the FSR2 context
            FfxErrorCode errorCode = ffxFsr2ContextCreate(&m_FSR2Context, &m_FSR2InitializationParameters);
            CauldronAssert(ASSERT_CRITICAL, errorCode == FFX_OK, L"Couldn't create the FidelityFX SDK FSR2 context.");

            // get GPU memory usage
            FfxEffectMemoryUsage gpuMemoryUsageUpscaler;
            ffxFsr2ContextGetGpuMemoryUsage(&m_FSR2Context, &gpuMemoryUsageUpscaler);

            appendMemoryUsageText("VRAM FSR2 Upscaler:\t\t", gpuMemoryUsageUpscaler);
        }

        // always init FSR3 for the frame interpolation
        {
            const ResolutionInfo& resInfo                           = GetFramework()->GetResolutionInfo();
            m_FSR3InitializationParameters.maxRenderSize.width      = resInfo.DisplayWidth;
            m_FSR3InitializationParameters.maxRenderSize.height     = resInfo.DisplayHeight;
            m_FSR3InitializationParameters.maxUpscaleSize.width     = resInfo.DisplayWidth;
            m_FSR3InitializationParameters.maxUpscaleSize.height    = resInfo.DisplayHeight;
            m_FSR3InitializationParameters.displaySize.width        = resInfo.DisplayWidth;
            m_FSR3InitializationParameters.displaySize.height       = resInfo.DisplayHeight;
            m_FSR3InitializationParameters.flags                    = FFX_FSR3_ENABLE_AUTO_EXPOSURE;
            static bool s_InvertedDepth                             = GetConfig()->InvertedDepth;
            if (s_InvertedDepth)
                m_FSR3InitializationParameters.flags |= FFX_FSR3_ENABLE_DEPTH_INVERTED | FFX_FSR3_ENABLE_DEPTH_INFINITE;
            m_FSR3InitializationParameters.flags |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;

            m_EnableAsyncCompute = m_PendingEnableAsyncCompute;
            if (m_EnableAsyncCompute)
                m_FSR3InitializationParameters.flags |= FFX_FSR3_ENABLE_ASYNC_WORKLOAD_SUPPORT;

             if (m_UpscaleMethod != UpscaleRM::UpscaleMethod::FSR3)
                m_FSR3InitializationParameters.flags |= FFX_FSR3_ENABLE_INTERPOLATION_ONLY;

            // Do eror checking in debug
#if defined(_DEBUG)
            m_FSR3InitializationParameters.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
            m_FSR3InitializationParameters.fpMessage = &FSRRenderModule::FfxMsgCallback;
#endif  // #if defined(_DEBUG)


            // Create the FSR3 context
            {
                m_FSR3InitializationParameters.backBufferFormat = SDKWrapper::GetFfxSurfaceFormat(GetFramework()->GetSwapChain()->GetSwapChainFormat());

                // create the context.
                errorCode = ffxFsr3ContextCreate(&m_FSR3Context, &m_FSR3InitializationParameters);
                CAULDRON_ASSERT(errorCode == FFX_OK);

                // get GPU memory usage
                FfxEffectMemoryUsage gpuMemoryUsageUpscaler;
                FfxEffectMemoryUsage gpuMemoryUsageOpticalFlow;
                FfxEffectMemoryUsage gpuMemoryUsageFrameGeneration;
                ffxFsr3ContextGetGpuMemoryUsage(
                    &m_FSR3Context, &gpuMemoryUsageUpscaler, &gpuMemoryUsageOpticalFlow, &gpuMemoryUsageFrameGeneration);

                if (gpuMemoryUsageUpscaler.totalUsageInBytes > 0)
                {
                    appendMemoryUsageText("VRAM FSR3 Upscaler:\t\t", gpuMemoryUsageUpscaler);
                }
                if (gpuMemoryUsageOpticalFlow.totalUsageInBytes > 0)
                {
                    appendMemoryUsageText("VRAM Optical Flow:\t\t", gpuMemoryUsageOpticalFlow);
                }
                if (gpuMemoryUsageFrameGeneration.totalUsageInBytes > 0)
                {
                    appendMemoryUsageText("VRAM Frame Generation:\t", gpuMemoryUsageFrameGeneration);
                }

                FfxSwapchain ffxSwapChain = SDKWrapper::ffxGetSwapchain(GetSwapChain());

                // Configure frame generation
                FfxResource hudLessResource = SDKWrapper::ffxGetResource(
                    m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), L"FSR3_HudLessBackbuffer", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

                m_FrameGenerationConfig.frameGenerationEnabled  = false;
                m_FrameGenerationConfig.frameGenerationCallback = [](const FfxFrameGenerationDispatchDescription* desc, void*) -> FfxErrorCode { return ffxFsr3DispatchFrameGeneration(desc); };
                m_FrameGenerationConfig.presentCallback         = (s_uiRenderMode == 2) ? UiCompositionCallback : nullptr;
                m_FrameGenerationConfig.swapChain               = ffxSwapChain;
                m_FrameGenerationConfig.HUDLessColor            = (s_uiRenderMode == 3) ? hudLessResource : FfxResource({});

                errorCode = ffxFsr3ConfigureFrameGeneration(&m_FSR3Context, &m_FrameGenerationConfig);
                CAULDRON_ASSERT(errorCode == FFX_OK);
            }
        }
    }
    else
    {
        FfxSwapchain ffxSwapChain = SDKWrapper::ffxGetSwapchain(GetSwapChain());

        SDKWrapper::ffxRegisterFrameinterpolationUiResource(ffxSwapChain, FfxResource({}), 0);

        // disable frame generation before destroying context
        // also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
        m_FrameGenerationConfig.frameGenerationEnabled = false;
        m_FrameGenerationConfig.swapChain              = ffxSwapChain;
        m_FrameGenerationConfig.presentCallback        = nullptr;
        m_FrameGenerationConfig.HUDLessColor           = FfxResource({});
        ffxFsr3ConfigureFrameGeneration(&m_FSR3Context, &m_FrameGenerationConfig);

        // Destroy the FSR contexts
        if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR1)
            ffxFsr1ContextDestroy(&m_FSR1Context);

        if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR2)
            ffxFsr2ContextDestroy(&m_FSR2Context);

        ffxFsr3ContextDestroy(&m_FSR3Context);
    }

    m_UIElements[m_UIInfoTextIndex]->Show(infoText.str().length() > 0);
    m_UIElements[m_UIInfoTextIndex]->SetDesc(infoText.str().c_str());
}

ResolutionInfo FSRRenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    float thisFrameUpscaleRatio = 1.f / m_UpscaleRatio;

    m_MaxRenderWidth            = (uint32_t)((float)displayWidth * thisFrameUpscaleRatio * m_LetterboxRatio);
    m_MaxRenderHeight           = (uint32_t)((float)displayHeight * thisFrameUpscaleRatio * m_LetterboxRatio);
    m_MaxUpscaleWidth           = (uint32_t)((float)displayWidth * m_LetterboxRatio);
    m_MaxUpscaleHeight          = (uint32_t)((float)displayHeight *  m_LetterboxRatio);
    m_MipBias                   = CalculateMipBias(m_UpscaleRatio);
    UpdateMipBias(nullptr);

    return {m_MaxRenderWidth, m_MaxRenderHeight,
			m_MaxUpscaleWidth, m_MaxUpscaleHeight,
            displayWidth, displayHeight };
}

void FSRRenderModule::OnPreFrame()
{
    if (NeedsReInit())
    {
        SwitchUpscaler(m_UIMethod);
    }

    else if (m_DynamicRes)
    {
        // Quick dynamic resolution test: generate a resolution between performance mode and full resolution
        auto  duration   = std::chrono::system_clock::now().time_since_epoch();
        auto  seconds    = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        float sinTime    = ((float)(sin(seconds)) + 1.0f) / 2.0f;
        m_UpscaleRatio   = 1.f + sinTime;

         uint32_t renderWidth  = (uint32_t)((float)m_MaxRenderWidth / m_UpscaleRatio);
         uint32_t renderHeight = (uint32_t)((float)m_MaxRenderHeight / m_UpscaleRatio);
         m_MipBias    = CalculateMipBias(m_UpscaleRatio);

        // DRS: update render resolution (without triggering resize, as that would cause a GPU flush and recreation of resources and we don't want that)
        // this is likely to cause some issues in some samples if they assume the viewport/scissor rect is covering the whole render target
        GetFramework()->UpdateRenderResolution(renderWidth, renderHeight);

        UpdateMipBias(nullptr);
    }
}

void FSRRenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    // Need to recreate the FSR context on resource resize
    UpdateFSRContexts(false);   // Destroy
    UpdateFSRContexts(true);    // Re-create

    // Reset jitter index
    m_JitterIndex = 0;
}

void FSRRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    if (m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState() != ResourceState::NonPixelShaderResource)
    {
        Barrier barrier = Barrier::Transition(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(),
                                              m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState(),
                                              ResourceState::NonPixelShaderResource);
        ResourceBarrier(pCmdList, 1, &barrier);
    }

    if (m_pUiTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState() != ResourceState::ShaderResource)
    {
        std::vector<Barrier> barriers = {Barrier::Transition(m_pUiTexture[m_curUiTextureIndex]->GetResource(),
                                                             m_pUiTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState(),
                                                             ResourceState::ShaderResource)};
        ResourceBarrier(pCmdList, (uint32_t)barriers.size(), barriers.data());
    }

    GPUScopedProfileCapture sampleMarker(pCmdList, L"FFX Upscaler");
    const ResolutionInfo&   resInfo = GetFramework()->GetResolutionInfo();
    CameraComponent*        pCamera = GetScene()->GetCurrentCamera();

    GPUResource* pSwapchainBackbuffer = GetFramework()->GetSwapChain()->GetBackBufferRT()->GetCurrentResource();
    FfxResource backbuffer            = SDKWrapper::ffxGetResource(pSwapchainBackbuffer,L"SwapchainSurface", FFX_RESOURCE_STATE_PRESENT);

    // copy input source to temp so that the input and output texture of the upscalers is different 
    {
        std::vector<Barrier> barriers;
        barriers.push_back(Barrier::Transition(
            m_pTempTexture->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopyDest));
        barriers.push_back(Barrier::Transition(
            m_pColorTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopySource));
        ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    {
        GPUScopedProfileCapture sampleMarker(pCmdList, L"CopyToTemp");

        TextureCopyDesc desc(m_pColorTarget->GetResource(), m_pTempTexture->GetResource());
        CopyTextureRegion(pCmdList, &desc);
    }

    {
        std::vector<Barrier> barriers;
        barriers.push_back(Barrier::Transition(
            m_pTempTexture->GetResource(), ResourceState::CopyDest, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
        barriers.push_back(Barrier::Transition(
            m_pColorTarget->GetResource(), ResourceState::CopySource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
        ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    // Upscale the scene first
    {
        if (m_UpscaleMethod < UpscaleRM::UpscaleMethod::FSR1)
        {
            {
                {
                    std::vector<Barrier> barriers;                 
                    barriers.push_back(Barrier::Transition(
                        m_pColorTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::UnorderedAccess));
                    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());
                }

                GPUScopedProfileCapture sampleMarker(pCmdList, L"Upsampler (basic)");
                // Allocate a dynamic constant buffer and set
                m_ConstantData.Type = m_UpscaleMethod;

                const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

                m_ConstantData.invRenderWidth   = 1.0f / resInfo.RenderWidth;
                m_ConstantData.invRenderHeight  = 1.0f / resInfo.RenderHeight;
                m_ConstantData.invDisplayWidth  = 1.0f / resInfo.DisplayWidth;
                m_ConstantData.invDisplayHeight = 1.0f / resInfo.DisplayHeight;

                BufferAddressInfo bufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(UpscaleCBData), &m_ConstantData);

                if (m_UpscaleMethod != UpscaleRM::UpscaleMethod::Bicubic)
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
                const uint32_t numGroupX = DivideRoundingUp(m_pColorTarget->GetDesc().Width, g_NumThreadX);
                const uint32_t numGroupY = DivideRoundingUp(m_pColorTarget->GetDesc().Height, g_NumThreadY);
                Dispatch(pCmdList, numGroupX, numGroupY, 1);

                {
                    Barrier barrier = Barrier::Transition(m_pColorTarget->GetResource(),
                                                          ResourceState::UnorderedAccess,
                                                          ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
                    ResourceBarrier(pCmdList, 1, &barrier);
                }
            }
        }
        else if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR1)
        {
            m_pToneMappingRenderModule->Execute(deltaTime, pCmdList);

            FfxFsr1DispatchDescription dispatchParameters = {};
            dispatchParameters.commandList                = SDKWrapper::ffxGetCommandList(pCmdList);
            dispatchParameters.renderSize                 = {resInfo.RenderWidth, resInfo.RenderHeight};
            dispatchParameters.enableSharpening           = m_RCASSharpen;
            dispatchParameters.sharpness                  = m_Sharpness;

            // We need to copy the low-res results to a temp target regardless as FSR1 doesn't deal well with resources that are
            // rendered with limited scisor/viewport rects
            std::vector<Barrier> barriers;
            barriers.push_back(Barrier::Transition(m_pFsr1TmpTexture->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopyDest));
            barriers.push_back(Barrier::Transition(m_pTonemappedColorTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopySource));
            ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

            TextureCopyDesc desc(m_pTonemappedColorTarget->GetResource(), m_pFsr1TmpTexture->GetResource());
            CopyTextureRegion(pCmdList, &desc);

            barriers[0] = Barrier::Transition(m_pFsr1TmpTexture->GetResource(), ResourceState::CopyDest, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
            barriers[1] = Barrier::Transition(m_pTonemappedColorTarget->GetResource(), ResourceState::CopySource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
            ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

            // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
            dispatchParameters.color  = SDKWrapper::ffxGetResource(m_pFsr1TmpTexture->GetResource(), L"FSR1_InputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParameters.output = SDKWrapper::ffxGetResource(m_pTonemappedColorTarget->GetResource(), L"FSR1_OutputUpscaledColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

            FfxErrorCode errorCode = ffxFsr1ContextDispatch(&m_FSR1Context, &dispatchParameters);
            CAULDRON_ASSERT(errorCode == FFX_OK);
        }
        else if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR2)
        {
            // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
            FfxFsr2DispatchDescription dispatchParameters = {};
            dispatchParameters.commandList                = SDKWrapper::ffxGetCommandList(pCmdList);
            dispatchParameters.color =
                SDKWrapper::ffxGetResource(m_pTempTexture->GetResource(), L"FSR2_InputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParameters.depth = SDKWrapper::ffxGetResource(m_pDepthTarget->GetResource(), L"FSR2_InputDepth", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParameters.motionVectors =
                SDKWrapper::ffxGetResource(m_pMotionVectors->GetResource(), L"FSR2_InputMotionVectors", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParameters.exposure = SDKWrapper::ffxGetResource(nullptr, L"FSR2_InputExposure", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParameters.output = SDKWrapper::ffxGetResource(m_pColorTarget->GetResource(), L"FSR2_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

            if (m_UseReactiveMask && m_MaskMode != FSRMaskMode::Disabled)
            {
                if (m_MaskMode == FSRMaskMode::Auto)
                    dispatchParameters.enableAutoReactive = true;
                else
                    dispatchParameters.reactive = SDKWrapper::ffxGetResource(m_pReactiveMask->GetResource(), L"FSR2_InputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }
            else
            {
                dispatchParameters.reactive = SDKWrapper::ffxGetResource(nullptr, L"FSR2_EmptyInputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }

            if (m_UseTnCMask)
            {
                dispatchParameters.transparencyAndComposition =
                    SDKWrapper::ffxGetResource(m_pCompositionMask->GetResource(), L"FSR2_TransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }
            else
            {
                dispatchParameters.transparencyAndComposition =
                    SDKWrapper::ffxGetResource(nullptr, L"FSR2_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }

            // Jitter is calculated earlier in the frame using a callback from the camera update
            dispatchParameters.jitterOffset.x = -m_JitterX;
            dispatchParameters.jitterOffset.y = -m_JitterY;

            dispatchParameters.motionVectorScale.x = resInfo.fRenderWidth();
            dispatchParameters.motionVectorScale.y = resInfo.fRenderHeight();
            dispatchParameters.reset               = GetScene()->GetCurrentCamera()->WasCameraReset();
            dispatchParameters.enableSharpening    = m_RCASSharpen;
            dispatchParameters.sharpness           = m_Sharpness;

            // Cauldron keeps time in seconds, but FSR expects milliseconds
            dispatchParameters.frameTimeDelta = static_cast<float>(deltaTime * 1000.f);

            dispatchParameters.preExposure       = GetScene()->GetSceneExposure();
            dispatchParameters.renderSize.width  = resInfo.RenderWidth;
            dispatchParameters.renderSize.height = resInfo.RenderHeight;

            // Note, inverted depth and display mode are currently handled statically for the run of the sample.
            // If they become changeable at runtime, we'll need to modify how this information is queried
            static bool s_InvertedDepth = GetConfig()->InvertedDepth;

            // Setup camera params as required
            dispatchParameters.cameraFovAngleVertical = pCamera->GetFovY();
            if (s_InvertedDepth)
            {
                dispatchParameters.cameraFar  = pCamera->GetNearPlane();
                dispatchParameters.cameraNear = FLT_MAX;
            }
            else
            {
                dispatchParameters.cameraFar  = pCamera->GetFarPlane();
                dispatchParameters.cameraNear = pCamera->GetNearPlane();
            }

            FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_FSR2Context, &dispatchParameters);
            CAULDRON_ASSERT(errorCode == FFX_OK);
        }

        {
            // Update frame generation config
            FfxResource hudLessResource =
                SDKWrapper::ffxGetResource(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), L"FSR3_HudLessBackbuffer", FFX_RESOURCE_STATE_COMPUTE_READ);

            m_FrameGenerationConfig.swapChain              = SDKWrapper::ffxGetSwapchain(GetSwapChain());  // swapchain may be replaced internally at any time (for example when vsync changes)
            m_FrameGenerationConfig.frameGenerationEnabled = m_FrameInterpolation;
            m_FrameGenerationConfig.flags                  = 0;
            m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugTearLines ? FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
            m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugView ? FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
            m_FrameGenerationConfig.HUDLessColor            = (s_uiRenderMode == 3) ? hudLessResource : FfxResource({});
            m_FrameGenerationConfig.allowAsyncWorkloads     = m_AllowAsyncCompute && m_EnableAsyncCompute;
            // assume symmetric letterbox
            m_FrameGenerationConfig.interpolationRect.left   = (resInfo.DisplayWidth - resInfo.UpscaleWidth) / 2;
            m_FrameGenerationConfig.interpolationRect.top    = (resInfo.DisplayHeight - resInfo.UpscaleHeight) / 2;
            m_FrameGenerationConfig.interpolationRect.width  = resInfo.UpscaleWidth;
            m_FrameGenerationConfig.interpolationRect.height = resInfo.UpscaleHeight;
            m_FrameGenerationConfig.frameID                  = m_FrameID;

            if (m_UseCallback)
            {
                m_FrameGenerationConfig.frameGenerationCallback = [](const FfxFrameGenerationDispatchDescription* desc, void*) -> FfxErrorCode { return ffxFsr3DispatchFrameGeneration(desc); };
            }
            else
            {
                m_FrameGenerationConfig.frameGenerationCallback = nullptr;
            }
            m_FrameGenerationConfig.onlyPresentInterpolated = m_PresentInterpolatedOnly;

            ffxFsr3ConfigureFrameGeneration(&m_FSR3Context, &m_FrameGenerationConfig);
        }

        // FSR3: for other upscalers FSR3 is initialized with FFX_FSR3_ENABLE_INTERPOLATION_ONLY so in that case this will only prepare the data for interpolation
        // if interpolation is disabled it will do nothing
        {
            // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
            FfxFsr3DispatchUpscaleDescription dispatchParametersUpscaling = {};
            dispatchParametersUpscaling.commandList   = SDKWrapper::ffxGetCommandList(pCmdList);
            dispatchParametersUpscaling.color         = SDKWrapper::ffxGetResource(m_pTempTexture->GetResource(), L"FSR3_InputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParametersUpscaling.depth         = SDKWrapper::ffxGetResource(m_pDepthTarget->GetResource(), L"FSR3_InputDepth", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParametersUpscaling.motionVectors = SDKWrapper::ffxGetResource(m_pMotionVectors->GetResource(), L"FSR3_InputMotionVectors", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParametersUpscaling.exposure      = SDKWrapper::ffxGetResource(nullptr, L"FSR3_InputExposure", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParametersUpscaling.upscaleOutput = SDKWrapper::ffxGetResource(m_pColorTarget->GetResource(), L"FSR3_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchParametersUpscaling.flags         = 0;
            dispatchParametersUpscaling.flags |= m_DrawUpscalerDebugView ? FFX_FSR3_UPSCALER_FLAG_DRAW_DEBUG_VIEW : 0;

            if (m_UseReactiveMask && m_MaskMode != FSRMaskMode::Disabled)
            {
                dispatchParametersUpscaling.reactive = SDKWrapper::ffxGetResource(m_pReactiveMask->GetResource(), L"FSR3_InputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }
            else
            {
                dispatchParametersUpscaling.reactive = SDKWrapper::ffxGetResource(nullptr, L"FSR3_EmptyInputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }

            if (m_UseTnCMask)
            {
                dispatchParametersUpscaling.transparencyAndComposition =
                    SDKWrapper::ffxGetResource(m_pCompositionMask->GetResource(), L"FSR3_TransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }
            else
            {
                dispatchParametersUpscaling.transparencyAndComposition =
                    SDKWrapper::ffxGetResource(nullptr, L"FSR3_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            }

            // Jitter is calculated earlier in the frame using a callback from the camera update
            dispatchParametersUpscaling.jitterOffset.x      = -m_JitterX;
            dispatchParametersUpscaling.jitterOffset.y      = -m_JitterY;
            dispatchParametersUpscaling.motionVectorScale.x = resInfo.fRenderWidth();
            dispatchParametersUpscaling.motionVectorScale.y = resInfo.fRenderHeight();
            dispatchParametersUpscaling.reset               = false;
            dispatchParametersUpscaling.enableSharpening    = m_RCASSharpen;
            dispatchParametersUpscaling.sharpness           = m_Sharpness;

            // Cauldron keeps time in seconds, but FSR expects milliseconds
            dispatchParametersUpscaling.frameTimeDelta = (float)deltaTime * 1000.f;

            dispatchParametersUpscaling.preExposure       = GetScene()->GetSceneExposure();
            dispatchParametersUpscaling.renderSize.width  = resInfo.RenderWidth;
            dispatchParametersUpscaling.renderSize.height = resInfo.RenderHeight;
            dispatchParametersUpscaling.upscaleSize.width = resInfo.UpscaleWidth;
            dispatchParametersUpscaling.upscaleSize.height = resInfo.UpscaleHeight;

            // Note, inverted depth and display mode are currently handled statically for the run of the sample.
            // If they become changeable at runtime, we'll need to modify how this information is queried
            static bool s_InvertedDepth = GetConfig()->InvertedDepth;

            // Setup camera params as required
            dispatchParametersUpscaling.cameraFovAngleVertical = pCamera->GetFovY();
            if (s_InvertedDepth)
            {
                dispatchParametersUpscaling.cameraFar = pCamera->GetNearPlane();
                dispatchParametersUpscaling.cameraNear = FLT_MAX;
            }
            else
            {
                dispatchParametersUpscaling.cameraFar = pCamera->GetFarPlane();
                dispatchParametersUpscaling.cameraNear = pCamera->GetNearPlane();
            }

            FfxErrorCode errorCode = FFX_OK;
            if (m_UpscaleMethod == UpscaleRM::UpscaleMethod::FSR3)
            {
                errorCode = ffxFsr3ContextDispatchUpscale(&m_FSR3Context, &dispatchParametersUpscaling);
                CAULDRON_ASSERT(errorCode == FFX_OK);
            }

            if (m_FrameInterpolation)
            {
                FfxFsr3DispatchFrameGenerationPrepareDescription frameGenerationPrepareDispatchParams{};
                frameGenerationPrepareDispatchParams.cameraFar                 = dispatchParametersUpscaling.cameraFar;
                frameGenerationPrepareDispatchParams.cameraFovAngleVertical    = dispatchParametersUpscaling.cameraFovAngleVertical;
                frameGenerationPrepareDispatchParams.cameraNear                = dispatchParametersUpscaling.cameraNear;
                frameGenerationPrepareDispatchParams.commandList               = dispatchParametersUpscaling.commandList;
                frameGenerationPrepareDispatchParams.depth                     = dispatchParametersUpscaling.depth;
                frameGenerationPrepareDispatchParams.frameID                   = m_FrameID;
                frameGenerationPrepareDispatchParams.frameTimeDelta            = dispatchParametersUpscaling.frameTimeDelta;
                frameGenerationPrepareDispatchParams.jitterOffset              = dispatchParametersUpscaling.jitterOffset;
                frameGenerationPrepareDispatchParams.motionVectors             = dispatchParametersUpscaling.motionVectors;
                frameGenerationPrepareDispatchParams.motionVectorScale         = dispatchParametersUpscaling.motionVectorScale;
                frameGenerationPrepareDispatchParams.renderSize                = dispatchParametersUpscaling.renderSize;
                frameGenerationPrepareDispatchParams.viewSpaceToMetersFactor   = dispatchParametersUpscaling.viewSpaceToMetersFactor;

                errorCode = ffxFsr3ContextDispatchFrameGenerationPrepare(&m_FSR3Context, &frameGenerationPrepareDispatchParams);
                CAULDRON_ASSERT(errorCode == FFX_OK);
            }
        }

        FfxResource uiColor = (s_uiRenderMode == 1)
                                  ? SDKWrapper::ffxGetResource(m_pUiTexture[m_curUiTextureIndex]->GetResource(), L"FSR3_UiTexture", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ)
                                  : FfxResource({});

        FfxSwapchain ffxSwapChain = SDKWrapper::ffxGetSwapchain(GetSwapChain());
        SDKWrapper::ffxRegisterFrameinterpolationUiResource(ffxSwapChain, uiColor, m_DoublebufferInSwapchain ? FFX_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING : 0);

        // Dispatch frame generation, if not using the callback
        if (m_FrameInterpolation && !m_UseCallback)
        {
            FfxFrameGenerationDispatchDescription fgDesc{};
            SDKWrapper::ffxGetInterpolationCommandlist(ffxSwapChain, fgDesc.commandList);

            fgDesc.presentColor          = backbuffer;
            fgDesc.numInterpolatedFrames = 1;
            fgDesc.outputs[0]            = SDKWrapper::ffxGetFrameinterpolationTexture(ffxSwapChain);

            // assume symmetric letterbox
            fgDesc.interpolationRect.left   = (resInfo.DisplayWidth - resInfo.UpscaleWidth) / 2;
            fgDesc.interpolationRect.top    = (resInfo.DisplayHeight - resInfo.UpscaleHeight) / 2;
            fgDesc.interpolationRect.width  = resInfo.UpscaleWidth;
            fgDesc.interpolationRect.height = resInfo.UpscaleHeight;
            fgDesc.frameID                  = m_FrameID;
            ffxFsr3DispatchFrameGeneration(&fgDesc);
        }
    }

    // Increment frame ID, used by frame generation
    m_FrameID++;

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
}

void FSRRenderModule::PreTransCallback(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture sampleMarker(pCmdList, L"Pre-Trans (FSR3)");

    std::vector<Barrier> barriers;
    barriers.push_back(Barrier::Transition(m_pReactiveMask->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::RenderTargetResource));
    barriers.push_back(Barrier::Transition(m_pCompositionMask->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::RenderTargetResource));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // We need to clear the reactive and composition masks before any translucencies are rendered into them
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ClearRenderTarget(pCmdList, &m_RasterViews[0]->GetResourceView(), clearColor);
    ClearRenderTarget(pCmdList, &m_RasterViews[1]->GetResourceView(), clearColor);

    barriers.clear();
    barriers.push_back(Barrier::Transition(m_pReactiveMask->GetResource(), ResourceState::RenderTargetResource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
    barriers.push_back(Barrier::Transition(m_pCompositionMask->GetResource(), ResourceState::RenderTargetResource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    if (m_MaskMode != FSRMaskMode::Auto)
        return;

    barriers.clear();
    barriers.push_back(Barrier::Transition(m_pColorTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopySource));
    barriers.push_back(Barrier::Transition(m_pOpaqueTexture->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopyDest));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // Copy the color render target before we apply translucency
    TextureCopyDesc copyColor = TextureCopyDesc(m_pColorTarget->GetResource(), m_pOpaqueTexture->GetResource());
    CopyTextureRegion(pCmdList, &copyColor);

    barriers.clear();
    barriers.push_back(Barrier::Transition(m_pColorTarget->GetResource(), ResourceState::CopySource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
    barriers.push_back(Barrier::Transition(m_pOpaqueTexture->GetResource(), ResourceState::CopyDest, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // update intex for UI doublebuffering
    UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
    m_curUiTextureIndex   = m_DoublebufferInSwapchain ? 0 : (++m_curUiTextureIndex) & 1;
    uimod->SetUiSurfaceIndex(m_curUiTextureIndex);
}

void FSRRenderModule::PostTransCallback(double deltaTime, CommandList* pCmdList)
{
    if ((m_MaskMode != FSRMaskMode::Auto) || (m_UpscaleMethod < UpscaleRM::UpscaleMethod::FSR2))
        return;

    GPUScopedProfileCapture sampleMarker(pCmdList, L"Gen Reactive Mask (FSR3)");

    FfxFsr3GenerateReactiveDescription generateReactiveParameters = {};
    generateReactiveParameters.commandList = SDKWrapper::ffxGetCommandList(pCmdList);
    generateReactiveParameters.colorOpaqueOnly = SDKWrapper::ffxGetResource(m_pOpaqueTexture->GetResource(), L"FSR3_Input_Opaque_Color", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    generateReactiveParameters.colorPreUpscale = SDKWrapper::ffxGetResource(m_pColorTarget->GetResource(), L"FSR3_Input_PreUpscaleColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    generateReactiveParameters.outReactive = SDKWrapper::ffxGetResource(m_pReactiveMask->GetResource(), L"FSR3_InputReactiveMask", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    generateReactiveParameters.renderSize.width = resInfo.RenderWidth;
    generateReactiveParameters.renderSize.height = resInfo.RenderHeight;

    generateReactiveParameters.scale = 1.f;
    generateReactiveParameters.cutoffThreshold = 0.2f;
    generateReactiveParameters.binaryValue = 0.9f;
    generateReactiveParameters.flags = FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_APPLY_TONEMAP |
                                       FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_APPLY_THRESHOLD |
                                       FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX;

    FfxErrorCode errorCode = ffxFsr3ContextGenerateReactiveMask(&m_FSR3Context, &generateReactiveParameters);
    CAULDRON_ASSERT(errorCode == FFX_OK);

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);
}


