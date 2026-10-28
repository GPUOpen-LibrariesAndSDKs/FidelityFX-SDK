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

#include "fsrapirendermodule.h"
#include <cauldron2/framework/render/rendermodules/ui/uirendermodule.h>
#include <cauldron2/framework/render/rasterview.h>
#include <cauldron2/framework/render/dynamicresourcepool.h>
#include <cauldron2/framework/render/dx12/dynamicbufferpool_dx12.h>
#include <cauldron2/framework/render/dx12/parameterset_dx12.h>
#include <cauldron2/framework/render/dx12/pipelineobject_dx12.h>
#include <cauldron2/framework/render/dx12/rootsignaturedesc_dx12.h>
#include <cauldron2/framework/render/dx12/commandlist_dx12.h>
#include <cauldron2/framework/render/dx12/device_dx12.h>
#include <cauldron2/framework/render/dx12/swapchain_dx12.h>
#include <cauldron2/framework/render/dx12/resourceviewallocator_dx12.h>
#include <cauldron2/framework/core/backend_interface.h>
#include <cauldron2/framework/core/scene.h>
#include <cauldron2/framework/misc/assert.h>
#include <cauldron2/framework/render/profiler.h>
#include <cauldron2/rendermodules/translucency/translucencyrendermodule.h>

#include <cauldron2/framework/core/framework.h>
#include <cauldron2/framework/core/win/framework_win.h>
#include <cauldron2/framework/render/dx12/device_dx12.h>
#include <cauldron2/framework/render/dx12/commandlist_dx12.h>

#include <FidelityFX/api/include/dx12/ffx_api_dx12.hpp>
#include <FidelityFX/framegeneration/include/dx12/ffx_api_framegeneration_dx12.hpp>

#include <functional>

using namespace cauldron;

[[nodiscard]]
FSRRenderModule::FSRRenderModule()
    : RenderModule(L"FSRApiRenderModule")
{
}

void FSRRenderModule::Init(const json& initData)
{
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

    // Create render resolution opaque render target to use for auto-reactive mask generation
    TextureDesc desc = m_pColorTarget->GetDesc();
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    desc.Width = resInfo.RenderWidth;
    desc.Height = resInfo.RenderHeight;
    desc.Name = L"FSR_OpaqueTexture";
    m_pOpaqueTexture = GetDynamicResourcePool()->CreateRenderTexture(&desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight)
        {
            desc.Width = renderingWidth;
            desc.Height = renderingHeight;
        });

    // Register additional exports for translucency pass
    BlendDesc      reactiveCompositionBlend = {
        true, Blend::InvDstColor, Blend::One, BlendOp::Add, Blend::One, Blend::Zero, BlendOp::Add, static_cast<uint32_t>(ColorWriteMask::Red)};

    OptionalTransparencyOptions transOptions;
    transOptions.OptionalTargets.emplace_back(m_pReactiveMask, reactiveCompositionBlend);
    transOptions.OptionalTargets.emplace_back(m_pCompositionMask, reactiveCompositionBlend);
    transOptions.OptionalAdditionalOutputs = L"float ReactiveTarget : SV_TARGET1; float CompositionTarget : SV_TARGET2;";
    transOptions.OptionalAdditionalExports =
        L"float hasAnimatedTexture = 0.f; output.ReactiveTarget = ReactiveMask; output.CompositionTarget = max(Alpha, hasAnimatedTexture);";

    // Add additional exports for FSR to translucency pass
    m_pTransRenderModule->AddOptionalTransparencyOptions(transOptions);

    // Create temporary texture to copy color into before upscale
    {
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
    }

    // Set our render resolution function as that to use during resize to get render width/height from display width/height
    m_pUpdateFunc = [this](uint32_t displayWidth, uint32_t displayHeight) { return this->UpdateResolution(displayWidth, displayHeight); };

    // Start disabled as this will be enabled externally
    RenderModule::SetModuleEnabled(false);

    {
        // Register upscale method picker picker
        m_pUISection = GetUIManager()->RegisterUIElements("FSR Upscaling", UISectionType::Sample);
        InitUI(m_pUISection);
    }

    //////////////////////////////////////////////////////////////////////////
    // Finish up init

    // That's all we need for now
    SetModuleReady(true);

    SwitchUpscaler(Upscaler_FSRAPI);
}

void FSRRenderModule::ShowUI(bool show)
{
    if (m_pUISection)
    {
        m_pUISection->Show(show);
    }
}

FSRRenderModule::~FSRRenderModule()
{
    // Destroy the FSR context
    UpdateFSRContext(false);
}

void FSRRenderModule::EnableModule(bool enabled)
{
    // If disabling the render module, we need to disable the upscaler with the framework
    if (!enabled)
    {
        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        // Destroy the FSR context
        UpdateFSRContext(false);

        if (GetFramework()->UpscalerEnabled())
            GetFramework()->EnableUpscaling(false);

        CameraComponent::SetJitterCallbackFunc(nullptr);
    }
    else
    {
        // Setup everything needed when activating FSR
        // Will also enable upscaling
        UpdatePreset(nullptr);

        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        // Create the FSR context
        UpdateFSRContext(true);

        if (m_UpscaleMethod == Upscaler_FSRAPI)
        {
            // Set the jitter callback to use
            CameraJitterCallback jitterCallback = [this](Vec2& values) {
                // Increment jitter index for frame
                ++m_JitterIndex;

                // Update FSR jitter for built in TAA
                const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

                ffx::ReturnCode                     retCode;
                int32_t                             jitterPhaseCount;
                ffx::QueryDescUpscaleGetJitterPhaseCount getJitterPhaseDesc{};
                getJitterPhaseDesc.displayWidth   = resInfo.DisplayWidth;
                getJitterPhaseDesc.renderWidth    = resInfo.RenderWidth;
                getJitterPhaseDesc.pOutPhaseCount = &jitterPhaseCount;

                retCode = ffx::Query(m_UpscalingContext, getJitterPhaseDesc);
                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                    L"ffxQuery(UpscalingContext,GETJITTERPHASECOUNT) returned %d", (uint32_t)retCode);

                ffx::QueryDescUpscaleGetJitterOffset getJitterOffsetDesc{};
                getJitterOffsetDesc.index                              = m_JitterIndex;
                getJitterOffsetDesc.phaseCount                         = jitterPhaseCount;
                getJitterOffsetDesc.pOutX                              = &m_JitterX;
                getJitterOffsetDesc.pOutY                              = &m_JitterY;

                retCode = ffx::Query(m_UpscalingContext, getJitterOffsetDesc);

                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                    L"ffxQuery(UpscalingContext,FSR_GETJITTEROFFSET) returned %d", (uint32_t)retCode);

                values = Vec2(-2.f * m_JitterX / resInfo.RenderWidth, 2.f * m_JitterY / resInfo.RenderHeight);
            };
            CameraComponent::SetJitterCallbackFunc(jitterCallback);
        }

        ClearReInit();
        SetModuleReady(true);
    }
    m_Enabled = enabled;
}

void FSRRenderModule::InitUI(UISection* pUISection)
{
    pUISection->RegisterUIElement<UICheckBox>("Enable", (bool&)m_UiEnabled, [this](bool cur, bool old) { SetModuleEnabled(true); SetModuleReady(false); });

    std::vector<const char*> presetComboOptions = { "Native AA (1.0x)", "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Ultra Performance (3x)" };
    pUISection->RegisterUIElement<UICombo>("Scale Preset", (int32_t&)m_ScalePreset, std::move(presetComboOptions), [this](int32_t cur, int32_t old) { UpdatePreset(&old); });

    EnableModule(true);
}

void FSRRenderModule::SwitchUpscaler(int32_t newUpscaler)
{
    // Flush everything out of the pipe before disabling/enabling things
    GetDevice()->FlushAllCommandQueues();

    if (ModuleEnabled())
        EnableModule(false);

    // 0 = native, 1 = FFXAPI
    SetFilter(newUpscaler);
    switch (newUpscaler)
    {
    case 0:
        m_pTAARenderModule->EnableModule(false);
        m_pToneMappingRenderModule->EnableModule(true);
        break;
    case 1:
        ClearReInit();
        // Also disable TAA render module
        m_pTAARenderModule->EnableModule(false);
        m_pToneMappingRenderModule->EnableModule(true);
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
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update mip bias
    float oldValue = m_MipBias;
    m_MipBias = cMipBias[static_cast<uint32_t>(m_ScalePreset)];
    UpdateMipBias(&oldValue);

    // Update resolution since rendering ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSRRenderModule::UpdateUpscaleRatio(const float* pOldRatio)
{
    // Disable/Enable FSR since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSRRenderModule::UpdateMipBias(const float* pOldBias)
{
    // Update the scene MipLODBias to use
    GetScene()->SetMipLODBias(m_MipBias);
}

void FSRRenderModule::UpdateFSRContext(bool enabled)
{
    if (enabled)
    {
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        static bool s_InvertedDepth = GetConfig()->InvertedDepth;

        ffx::CreateBackendDX12Desc backendDesc{};
        backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backendDesc.device = GetDevice()->GetImpl()->DX12Device();

        if (m_UpscaleMethod == Upscaler_FSRAPI)
        {
            ffx::CreateContextDescUpscale createFsr{};
            ffx::CreateContextDescUpscaleVersion createFsrVersion{};
            createFsrVersion.version = FFX_UPSCALER_VERSION;
            ffx::ReturnCode retCode;
            ffxReturnCode_t retCode_t;

            createFsr.maxRenderSize  = {resInfo.RenderWidth, resInfo.RenderHeight};
            createFsr.maxUpscaleSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
            createFsr.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
            createFsr.flags |= FFX_UPSCALE_ENABLE_DEBUG_VISUALIZATION;

            if (s_InvertedDepth)
            {
                createFsr.flags |= FFX_UPSCALE_ENABLE_DEPTH_INVERTED | FFX_UPSCALE_ENABLE_DEPTH_INFINITE;
            }
            createFsr.flags |= FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
            createFsr.fpMessage = nullptr;

            // Create the FSR context
            {
                //Before creating any of FSR contexts, query VRAM size
                struct FfxApiEffectMemoryUsage gpuMemoryUsageUpscaler = {0};
                struct ffxQueryDescUpscaleGetGPUMemoryUsageV2 upscalerGetGPUMemoryUsageV2 = {0};
                upscalerGetGPUMemoryUsageV2.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2;
                upscalerGetGPUMemoryUsageV2.device = GetDevice()->GetImpl()->DX12Device();
                upscalerGetGPUMemoryUsageV2.maxRenderSize = { resInfo.RenderWidth, resInfo.RenderHeight };
                upscalerGetGPUMemoryUsageV2.maxUpscaleSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
                upscalerGetGPUMemoryUsageV2.flags = createFsr.flags;
                upscalerGetGPUMemoryUsageV2.gpuMemoryUsageUpscaler = &gpuMemoryUsageUpscaler;

                // lifetime of this must last until after CreateContext call!
                struct ffxOverrideVersion versionOverride = {0};
                versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
                if (m_FsrVersionIndex < m_FsrVersionIds.size() && m_overrideVersion)
                {
                    versionOverride.versionId = m_FsrVersionIds[m_FsrVersionIndex];
                    upscalerGetGPUMemoryUsageV2.header.pNext = &versionOverride.header;
                    retCode_t = ffxQuery(nullptr, &upscalerGetGPUMemoryUsageV2.header);
                    CauldronAssert(ASSERT_WARNING, retCode_t == FFX_API_RETURN_OK,
                        L"ffxQuery(nullptr,UpscaleGetGPUMemoryUsageV2, %S) returned %d", m_FsrVersionNames[m_FsrVersionIndex], retCode_t);
                    Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Upscaler version %S Query GPUMemoryUsageV2 VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", m_FsrVersionNames[m_FsrVersionIndex], gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f, gpuMemoryUsageUpscaler.aliasableUsageInBytes / 1048576.f);
                    retCode = ffx::CreateContext(m_UpscalingContext, nullptr, createFsr, createFsrVersion, backendDesc, versionOverride);
                }
                else
                {
                    retCode = ffx::Query(upscalerGetGPUMemoryUsageV2);
                    CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                        L"ffxQuery(nullptr,UpscaleGetGPUMemoryUsageV2) returned %d", (uint32_t)retCode);
                    Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Default Upscaler Query GPUMemoryUsageV2 totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f, gpuMemoryUsageUpscaler.aliasableUsageInBytes / 1048576.f);
                    retCode = ffx::CreateContext(m_UpscalingContext, nullptr, createFsr, createFsrVersion, backendDesc);
                }

                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't create the ffxapi upscaling context: %d", (uint32_t)retCode);
            }

            //Query created version
            ffxQueryGetProviderVersion getVersion = {0};
            getVersion.header.type                = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;

            retCode_t = ffxQuery(&m_UpscalingContext, &getVersion.header);
            CauldronAssert(ASSERT_WARNING, retCode_t == FFX_API_RETURN_OK,
                L"ffxQuery(UpscalingContext,GetProviderVersion) returned %d", retCode_t);

            m_currentUpscaleContextVersionId = getVersion.versionId;
            m_currentUpscaleContextVersionName = getVersion.versionName;

            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Upscaler Context versionid 0x%016llx, %S", m_currentUpscaleContextVersionId, m_currentUpscaleContextVersionName);

            for (uint32_t i = 0; i < m_FsrVersionIds.size(); i++)
            {
                if (m_FsrVersionIds[i] == m_currentUpscaleContextVersionId)
                {
                    m_FsrVersionIndex = i;
                }
            }
        }
    }
    else
    {
        if (m_UpscalingContext)
        {
            ffx::DestroyContext(m_UpscalingContext);
            m_UpscalingContext = nullptr;
        }
    }
}

ResolutionInfo FSRRenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    return {static_cast<uint32_t>((float)displayWidth / m_UpscaleRatio),
            static_cast<uint32_t>((float)displayHeight / m_UpscaleRatio),
            static_cast<uint32_t>((float)displayWidth),
            static_cast<uint32_t>((float)displayHeight),
            displayWidth, displayHeight };
}

void FSRRenderModule::OnPreFrame()
{
    if (m_UiEnabled != m_Enabled)
    {
        GetDevice()->FlushAllCommandQueues();
        EnableModule(m_UiEnabled);
        ClearReInit();
    }
    else if (NeedsReInit())
    {
        GetDevice()->FlushAllCommandQueues();

        // Need to recreate the FSR context
        EnableModule(false);
        EnableModule(true);

        ClearReInit();
    }
}

void FSRRenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    // Need to recreate the FSR context on resource resize
    UpdateFSRContext(false);   // Destroy
    UpdateFSRContext(true);    // Re-create

    // Reset jitter index
    m_JitterIndex = 0;
}

void FSRRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture sampleMarker(pCmdList, L"FSR Upscaling");
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    CameraComponent* pCamera = GetScene()->GetCurrentCamera();

    GPUResource* pSwapchainBackbuffer = GetFramework()->GetSwapChain()->GetBackBufferRT()->GetCurrentResource();
    FfxApiResource backbuffer = SDKWrapper::ffxGetResourceApi(pSwapchainBackbuffer, FFX_API_RESOURCE_STATE_PRESENT);

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

    // Note, inverted depth and display mode are currently handled statically for the run of the sample.
    // If they become changeable at runtime, we'll need to modify how this information is queried
    static bool s_InvertedDepth = GetConfig()->InvertedDepth;

    // Upscale the scene first
    if (m_UpscaleMethod == Upscaler_Native)
    {
        // Native, nothing to do
    }

    if (m_UpscaleMethod == Upscaler_FSRAPI)
    {
        // FFXAPI
        // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
        ffx::DispatchDescUpscale dispatchUpscale{};
        dispatchUpscale.commandList = pCmdList->GetImpl()->DX12CmdList();
        dispatchUpscale.color = SDKWrapper::ffxGetResourceApi(m_pTempTexture->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.depth = SDKWrapper::ffxGetResourceApi(m_pDepthTarget->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.motionVectors = SDKWrapper::ffxGetResourceApi(m_pMotionVectors->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.exposure = SDKWrapper::ffxGetResourceApi(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.output = SDKWrapper::ffxGetResourceApi(m_pColorTarget->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.reactive = SDKWrapper::ffxGetResourceApi(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.transparencyAndComposition = SDKWrapper::ffxGetResourceApi(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        // Jitter is calculated earlier in the frame using a callback from the camera update
        dispatchUpscale.jitterOffset.x = -m_JitterX;
        dispatchUpscale.jitterOffset.y = -m_JitterY;
        dispatchUpscale.motionVectorScale.x = resInfo.fRenderWidth();
        dispatchUpscale.motionVectorScale.y = resInfo.fRenderHeight();
        dispatchUpscale.reset = GetScene()->GetCurrentCamera()->WasCameraReset();
        dispatchUpscale.enableSharpening = true;
        dispatchUpscale.sharpness = 0.8f;

        // Cauldron keeps time in seconds, but FSR expects milliseconds
        dispatchUpscale.frameTimeDelta = static_cast<float>(deltaTime * 1000.0);

        dispatchUpscale.preExposure = GetScene()->GetSceneExposure();
        dispatchUpscale.renderSize.width = resInfo.RenderWidth;
        dispatchUpscale.renderSize.height = resInfo.RenderHeight;
        dispatchUpscale.upscaleSize.width = resInfo.UpscaleWidth;
        dispatchUpscale.upscaleSize.height = resInfo.UpscaleHeight;

        // Setup camera params as required
        dispatchUpscale.cameraFovAngleVertical = pCamera->GetFovY();
        dispatchUpscale.cameraFar  = pCamera->GetFarPlane();
        dispatchUpscale.cameraNear = pCamera->GetNearPlane();

        dispatchUpscale.flags = 0;

        ffx::ReturnCode retCode = ffx::Dispatch(m_UpscalingContext, dispatchUpscale);
        CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Dispatching FSR upscaling failed: %d", (uint32_t)retCode);
    }

    m_FrameID += 1;
    m_PreviousJitterX = m_JitterX;
    m_PreviousJitterY = m_JitterY;

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
}
