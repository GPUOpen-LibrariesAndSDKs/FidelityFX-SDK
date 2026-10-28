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
#include <cauldron2/framework/core/scene.h>
#include <cauldron2/framework/misc/assert.h>
#include <cauldron2/framework/render/profiler.h>
#include <cauldron2/framework/core/win/framework_win.h>
#include <cauldron2/framework/core/backend_interface.h>
#include <FidelityFX/api/include/ffx_api_matrix_helpers.hpp>

#if defined(_VK)
#include <cauldron2/framework/render/vk/dynamicbufferpool_vk.h>
#include <cauldron2/framework/render/vk/parameterset_vk.h>
#include <cauldron2/framework/render/vk/pipelineobject_vk.h>
#include <cauldron2/framework/render/vk/rootsignaturedesc_vk.h>
#include <cauldron2/framework/render/vk/commandlist_vk.h>
#include <cauldron2/framework/render/vk/device_vk.h>
#include <cauldron2/framework/render/vk/swapchain_vk.h>
#include <cauldron2/framework/render/vk/resourceviewallocator_vk.h>
#include <FidelityFX/api/include/vk/ffx_api_vk.hpp>
#include <FidelityFX/framegeneration/include/vk/ffx_api_framegeneration_vk.hpp>
#else
#include <cauldron2/framework/render/dx12/dynamicbufferpool_dx12.h>
#include <cauldron2/framework/render/dx12/parameterset_dx12.h>
#include <cauldron2/framework/render/dx12/pipelineobject_dx12.h>
#include <cauldron2/framework/render/dx12/rootsignaturedesc_dx12.h>
#include <cauldron2/framework/render/dx12/commandlist_dx12.h>
#include <cauldron2/framework/render/dx12/device_dx12.h>
#include <cauldron2/framework/render/dx12/swapchain_dx12.h>
#include <cauldron2/framework/render/dx12/resourceviewallocator_dx12.h>
#include <cauldron2/rendermodules/translucency/translucencyrendermodule.h>

#include <cauldron2/framework/core/framework.h>
#include <cauldron2/framework/render/dx12/device_dx12.h>
#include <cauldron2/framework/render/dx12/commandlist_dx12.h>

#include <FidelityFX/api/include/dx12/ffx_api_dx12.hpp>
#include <FidelityFX/framegeneration/include/dx12/ffx_api_framegeneration_dx12.hpp>

#endif

#include <functional>

using namespace std;
using namespace cauldron;

void RestoreApplicationSwapChain(bool recreateSwapchain = true);

const wchar_t* GetVersionQueryName(ffxStructType_t createDescType)
{
    switch (createDescType)
    {
    case FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE:
        return L"Upscale";
    case FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION:
        return L"FrameGeneration";
#if defined(_DX12)
    case FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12:
        return L"FrameGenerationSwapchainDX12";
#elif defined(_VK)
    case FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_VK:
        return L"FrameGenerationSwapchainVK";
#endif
    default:
        return L"Unknown";
    }
}

void QueryVersionNames(
    ffxStructType_t createDescType,
    void* device,
    std::vector<uint64_t>& versionIds,
    std::vector<std::string>& versionNames)
{
    ffxQueryDescGetVersions versionQuery{};
    versionQuery.header.type     = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    versionQuery.createDescType  = createDescType;
    versionQuery.device          = device;

    uint64_t versionCount   = 0;
    versionQuery.outputCount = &versionCount;
    ffxReturnCode_t retCode = ffxQuery(nullptr, &versionQuery.header);
    CauldronAssert(ASSERT_WARNING, retCode == FFX_API_RETURN_OK,
        L"ffxQuery(nullptr,GetVersions%sCount) returned %d", GetVersionQueryName(createDescType), retCode);

    versionIds.resize(versionCount);
    std::vector<const char*> queriedNames(versionCount);
    versionQuery.versionIds   = versionIds.data();
    versionQuery.versionNames = queriedNames.data();
    retCode = ffxQuery(nullptr, &versionQuery.header);
    CauldronAssert(ASSERT_WARNING, retCode == FFX_API_RETURN_OK,
        L"ffxQuery(nullptr,GetVersions%sIdsNames) returned %d", GetVersionQueryName(createDescType), retCode);

    versionNames.assign(queriedNames.begin(), queriedNames.end());
}

void FSRRenderModule::Init(const json&)
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
    m_pDistortionField[0]    = GetFramework()->GetRenderTexture(L"DistortionField0");
    m_pDistortionField[1]    = GetFramework()->GetRenderTexture(L"DistortionField1");
    m_pReactiveMask          = GetFramework()->GetRenderTexture(L"ReactiveMask");
    m_pCompositionMask       = GetFramework()->GetRenderTexture(L"TransCompMask");
    CauldronAssert(ASSERT_CRITICAL, m_pMotionVectors && m_pDistortionField[0] && m_pDistortionField[1] && m_pReactiveMask && m_pCompositionMask, L"Could not get one of the needed resources for FSR Rendermodule.");
    m_pInterpolationOutput = GetFramework()->GetRenderTexture(L"InterpolationOutput");

    // Get a CPU resource view that we'll use to map the render target to
    GetResourceViewAllocator()->AllocateCPURenderViews(&m_pRTResourceView);

    // Create render resolution opaque render target to use for auto-reactive mask generation
    TextureDesc desc = m_pColorTarget->GetDesc();
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    desc.Width = resInfo.RenderWidth;
    desc.Height = resInfo.RenderHeight;
    desc.Name = L"FSR_OpaqueTexture";
    m_pOpaqueTexture = GetDynamicResourcePool()->CreateRenderTexture(&desc, [](TextureDesc& desc, uint32_t, uint32_t, uint32_t renderingWidth, uint32_t renderingHeight)
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
        TextureDesc texDesc = m_pColorTarget->GetDesc();
        texDesc.Name        = L"UpscaleIntermediateTarget";
        texDesc.Width       = m_pColorTarget->GetDesc().Width;
        texDesc.Height      = m_pColorTarget->GetDesc().Height;

        m_pTempTexture = GetDynamicResourcePool()->CreateRenderTexture(
            &texDesc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t, uint32_t) {
                desc.Width  = displayWidth;
                desc.Height = displayHeight;
            });
        CauldronAssert(ASSERT_CRITICAL, m_pTempTexture, L"Couldn't create intermediate texture.");
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
    // Separate from FSR generation so it can be done when the engine creates the swapchain
    // should not be created and destroyed with FSR, as it requires a switch to windowed mode

#if defined(_VK)
    const cauldron::FIQueue* pAsyncComputeQueue = cauldron::GetDevice()->GetImpl()->GetFIAsyncComputeQueue();
    const cauldron::FIQueue* pPresentQueue = cauldron::GetDevice()->GetImpl()->GetFIPresentQueue();
    const cauldron::FIQueue* pImageAcquireQueue = cauldron::GetDevice()->GetImpl()->GetFIImageAcquireQueue();

    m_FrameInterpolationAvailable = pPresentQueue->queue != VK_NULL_HANDLE && pImageAcquireQueue->queue != VK_NULL_HANDLE;
    m_AsyncComputeAvailable = m_FrameInterpolationAvailable && pAsyncComputeQueue->queue != VK_NULL_HANDLE;
#else
    m_FrameInterpolationAvailable = true;
    m_AsyncComputeAvailable       = true;
#endif

    if (!m_FrameInterpolationAvailable)
    {
        m_FrameInterpolation = false;
        s_uiRenderMode       = UICompositionMode::No_UI_Handling;
        s_uiRenderModeNextFrame = UICompositionMode::No_UI_Handling;
		s_uiRenderModeAtDeactivation = UICompositionMode::No_UI_Handling;
        CauldronWarning(L"Frame interpolation isn't available on this device.");
    }
    if (!m_AsyncComputeAvailable)
    {
        m_EnableAsyncCompute        = false;
        m_PendingEnableAsyncCompute = false;
        m_AllowAsyncCompute         = false;

        CauldronWarning(L"Async compute Frame interpolation isn't available on this device.");
    }

    if (m_FrameInterpolationAvailable)
    {
#if defined(_VK)
        EnableFrameInterpolationSwapchain(true);
#elif defined(_DX12)
        EnableFrameInterpolationSwapchain(true);
        m_EnableFrameInterpolationSwapchain = true;
#endif
    }

    // Fetch hudless texture resources
    m_pHudLessTexture[0] = GetFramework()->GetRenderTexture(L"HudlessTarget0");
    m_pHudLessTexture[1] = GetFramework()->GetRenderTexture(L"HudlessTarget1");

    // Start disabled as this will be enabled externally
    cauldron::RenderModule::SetModuleEnabled(false);

    {
        // Register upscale method picker picker
        UISection* uiSection = GetUIManager()->RegisterUIElements("Upscaling", UISectionType::Sample);
        InitUI(uiSection);
    }

    //////////////////////////////////////////////////////////////////////////
    // Finish up init

    // That's all we need for now
    SetModuleReady(true);

    SwitchUpscaler(m_UiUpscaleMethod);
}

FSRRenderModule::~FSRRenderModule()
{
    // Destroy the FSR context
    UpdateFSRContext(false);

    if (m_SwapChainContext != nullptr)
    {
        // Restore the application's swapchain
        ffx::DestroyContext(m_SwapChainContext);
        m_SwapChainContext = nullptr;
        if(latencyWaitableObj)
        {
            CloseHandle(latencyWaitableObj);
            latencyWaitableObj = 0;
        }
        RestoreApplicationSwapChain(false);
    }
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
        if (GetFramework()->FrameInterpolationEnabled())
            GetFramework()->EnableFrameInterpolation(false);
        if (GetFramework()->IsSwapchainUsingFrameGenerationCallback())
		    GetFramework()->EnableSwapchainFrameGenerationCallback(false);

        UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        uimod->SetAsyncRender(false);
        uimod->SetRenderToTexture(false);
        uimod->SetCopyHudLessTexture(false);

        CameraComponent::SetJitterCallbackFunc(nullptr);
    }
    else
    {
        UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        uimod->SetAsyncRender(s_uiRenderMode == UICompositionMode::UiCallback);
        uimod->SetRenderToTexture(s_uiRenderMode == UICompositionMode::UiTexture);
        uimod->SetCopyHudLessTexture(s_uiRenderMode == UICompositionMode::PreUiBackbuffer);

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

                // Query jitter phase count from context
                retCode = ffx::Query(m_UpscalingContext, getJitterPhaseDesc);
                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                    L"ffxQuery(UpscalingContext,GETJITTERPHASECOUNT) returned %d", (uint32_t)retCode);

                ffx::QueryDescUpscaleGetJitterOffset getJitterOffsetDesc{};
                getJitterOffsetDesc.index                              = m_JitterIndex;
                getJitterOffsetDesc.phaseCount                         = jitterPhaseCount;
                getJitterOffsetDesc.pOutX                              = &m_JitterX;
                getJitterOffsetDesc.pOutY                              = &m_JitterY;

                // Query jitter offset from context
                retCode = ffx::Query(m_UpscalingContext, getJitterOffsetDesc);
                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                    L"ffxQuery(UpscalingContext,FSR_GETJITTEROFFSET) returned %d", (uint32_t)retCode);

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

FfxErrorCode waitCallback(wchar_t* fenceName, uint64_t fenceValueToWaitFor)
{
    CAUDRON_LOG_DEBUG(L"waiting on '%ls' with value %llu", fenceName, fenceValueToWaitFor);
    return FFX_API_RETURN_OK;
}

void FSRRenderModule::InitUI(UISection* pUISection)
{
    static const bool        alwaysTrue   = true;
    auto makeStringViews = [](const std::vector<std::string>& strings) {
        std::vector<const char*> views;
        views.reserve(strings.size());
        for (const std::string& string : strings)
        {
            views.push_back(string.c_str());
        }
        return views;
    };
    std::vector<const char*> comboOptions = {"Native", "FSR (ffxapi)"};
    pUISection->RegisterUIElement<UICombo>("Method", (int32_t&)m_UiUpscaleMethod, std::move(comboOptions), [this](int32_t cur, int32_t) { SwitchUpscaler(cur); });

    m_UIElements.emplace_back(
        pUISection->RegisterUIElement<UICheckBox>("Override FSR Version", m_overrideVersion, [this](int32_t, int32_t) { m_NeedReInit = true; }, false));

#if defined(_VK)
    VkDeviceContext deviceContext;
    deviceContext.vkDevice = GetDevice()->GetImpl()->VKDevice();
    deviceContext.vkPhysicalDevice = GetDevice()->GetImpl()->VKPhysicalDevice();
    deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
#endif // defined(_VK)
    // get FSR version info from ffxapi
    {
#if defined(_VK)
        void* device = &deviceContext;
#elif defined(_DX12)
        void* device = GetDevice()->GetImpl()->DX12Device();
#endif
        QueryVersionNames(
            FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,
            device,
            m_FsrVersionIds,
            m_FsrVersionNames);
    }

    ffx::CreateContextDescOverrideVersion versionOverride{};
    ffx::QueryDescUpscaleGetResourceRequirements upscaleResourceRequirementsDesc{};
    ffx::ReturnCode retCode;

#if defined(_DX12)
    // Chain ffxCreateBackendDX12Desc to provide device for null-context queries to reach external driver provider
    struct ffxCreateBackendDX12Desc backendDX12Desc = {};
    backendDX12Desc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backendDX12Desc.device = GetDevice()->GetImpl()->DX12Device();
#elif defined(_VK)
    struct ffxCreateBackendVKDesc backendVKDesc = {};
    backendVKDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
    backendVKDesc.deviceContext = deviceContext;
#endif

    for (size_t index = 0; index < m_FsrVersionIds.size(); index++)
    {
        versionOverride.versionId = m_FsrVersionIds[index];
        versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
        {
#if defined(_DX12)
            retCode = ffx::Query(upscaleResourceRequirementsDesc, versionOverride, backendDX12Desc);
#elif defined(_VK)
            retCode = ffx::Query(upscaleResourceRequirementsDesc, versionOverride, backendVKDesc);
#endif
            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                L"ffxQuery(nullptr,GetResourceRequirements,%S,backend) returned %d", m_FsrVersionNames[index].c_str(), (uint32_t)retCode);
        }

        CAUDRON_LOG_INFO(L"Upscaler %S: required_resources: %llu optional_resources: %llu", 
            m_FsrVersionNames[index].c_str(), upscaleResourceRequirementsDesc.required_resources, upscaleResourceRequirementsDesc.optional_resources);
    }

    std::vector<const char*> fsrVersionNames = makeStringViews(m_FsrVersionNames);
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "FSR Version", (int32_t&)m_FsrVersionIndex, fsrVersionNames, m_overrideVersion, [this](int32_t, int32_t) { m_NeedReInit = true; }, false));

    // Setup scale preset options
    std::vector<const char*> presetComboOptions = { "Native AA (1.0x)", "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Ultra Performance (3x)", "Custom", "Custom (DRS)" };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "Scale Preset", (int32_t&)m_ScalePreset, std::move(presetComboOptions), m_IsNonNative, [this](int32_t, int32_t old) { UpdatePreset(&old); }, false));
    
    // Setup mip bias
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Mip LOD Bias",
        m_MipBias,
        -5.f, 0.f,
        [this](float, float old) {
            UpdateMipBias(&old);
        },
        false
    ));

    // Setup scale factor (disabled for all but custom)
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Custom Scale",
        m_UpscaleRatio,
        1.f, 2.5f,
        m_UpscaleRatioEnabled,
        [this](float, float old) {
            UpdateUpscaleRatio(&old);
        },
        false
    ));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Aspect Ratio Letterbox X", m_LetterboxRatioX, 0.1f, 1.f, [this](float, float old) { UpdateUpscaleRatio(&old); }, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Aspect Ratio Letterbox Y", m_LetterboxRatioY, 0.1f, 1.f, [this](float, float old) { UpdateUpscaleRatio(&old); }, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Animating Letterbox Ratio X", m_GenerationRectRatioX, 0.1f, 1.f, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Animating Letterbox Ratio Y", m_GenerationRectRatioY, 0.1f, 1.f, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("App: Draw black letterboxing pixels", m_AppBlackOutsideGenerationRect, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Start animating letterbox", m_AnimateGenerationRect,
        [this](bool, bool) {
            // Reset animation timer state when toggled (keep slider ratio values)
            m_GenRectAnimationT = 0.0f;
            m_GenRectAnimationForward = true;
        },
        false, false));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UIButton>("Reset Upscaling", [this]() { m_ResetUpscale = true; }));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw upscaler debug view", m_DrawUpscalerDebugView, nullptr, false));

    // hint colorspace
    std::vector<const char*> presetColorSpaceComboOptions = {
        "Default (Linear)", "Assume Non-Linear", "Expect sRGB", "Expect PQ"};
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "Upscale Color Space",
        (int32_t&)m_colorSpace,
        std::move(presetColorSpaceComboOptions),
        m_EnableMaskOptions,
        [this](int32_t cur, int32_t) {
            if (cur == 0 || cur == 1)
            {
                m_NeedReInit = true;
            }
        },
        false));
    // Reactive mask
    std::vector<const char*> maskComboOptions{ "Disabled", "Manual Reactive Mask Generation", "Autogen FSR2 Helper Function" };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>("Reactive Mask Mode", (int32_t&)m_MaskMode, std::move(maskComboOptions), m_EnableMaskOptions, nullptr, false));

    // Use mask
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Use Transparency and Composition Mask", m_UseMask, m_EnableMaskOptions, nullptr, false));

    // Sharpening
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("RCAS Sharpening", m_RCASSharpen, nullptr, false, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>("Sharpness", m_Sharpness, 0.f, 1.f, m_RCASSharpen, nullptr, false));

    //Set Upscaler CB KeyValue post context creation
    std::vector<const char*>        configureUpscaleKeyLabels = { "fVelocity", "fReactivenessScale", "fShadingChangeScale", "fAccumulationAddedPerFrame", "fMinDisocclusionAccumulation"};
    
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "Upscaler CB Key to set",
        m_UpscalerCBKey,
        configureUpscaleKeyLabels,
        m_EnableMaskOptions,
        [this](int32_t, int32_t) { m_UpscalerCBValue =  m_UpscalerCBValueStore[m_UpscalerCBKey]; },
        m_EnableMaskOptions));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Upscaler CB Value to set",
        m_UpscalerCBValue,
        -1.f, 2.f,
        m_EnableMaskOptions,
        [this](float, float) { m_UpscalerCBValueStore[m_UpscalerCBKey] = m_UpscalerCBValue; SetUpscaleConstantBuffer(m_UpscalerCBKey, m_UpscalerCBValue); },
        m_EnableMaskOptions));

    // Debug Checker
    std::vector<const char*> debugMessageComboOptions{ "Disabled",
        "Enabled. nullptr message callback. GLOBALDEBUG_LEVEL_SILENCE",
        "Enabled. nullptr message callback. GLOBALDEBUG_LEVEL_ERRORS",
        "Enabled. nullptr message callback. GLOBALDEBUG_LEVEL_WARNINGS",
        "Enabled. Cauldron message callback. GLOBALDEBUG_LEVEL_SILENCE",
        "Enabled. Cauldron message callback. GLOBALDEBUG_LEVEL_ERRORS",
        "Enabled. Cauldron message callback. GLOBALDEBUG_LEVEL_WARNINGS"

    };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>("Debug Checker", (int32_t&)m_GlobalDebugCheckerMode, debugMessageComboOptions, 
        [this](int32_t nextVal, int32_t prevVal)
        { 
            if ((prevVal == FSRDebugCheckerMode::Disabled && nextVal != FSRDebugCheckerMode::Disabled) || (prevVal != FSRDebugCheckerMode::Disabled && nextVal == FSRDebugCheckerMode::Disabled))
            {
                SetGlobalDebugCheckerMode(static_cast<FSRDebugCheckerMode> (nextVal), true);
            }
            else
            {
                SetGlobalDebugCheckerMode(static_cast<FSRDebugCheckerMode> (nextVal), false);
            }
        }));

    // Frame interpolation
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISeparator>());
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Frame Interpolation", m_FrameInterpolation, m_FrameInterpolationAvailable,
        [this](bool, bool)
        {
            m_OfUiEnabled = m_FrameInterpolation && s_enableSoftwareMotionEstimation;
            
            GetFramework()->EnableFrameInterpolation(m_FrameInterpolation);
            
            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        }, 
        false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Frame Interpolation Swapchain", m_EnableFrameInterpolationSwapchain, m_FrameInterpolationAvailable,
        [this](bool, bool)
        {
            m_OfUiEnabled = m_FrameInterpolation && s_enableSoftwareMotionEstimation;

            if (!m_EnableFrameInterpolationSwapchain) {
				// save ui render mode before setting to no ui handling
                s_uiRenderModeAtDeactivation = s_uiRenderModeNextFrame;
                //if no proxy swapchain, then render UI directly to backbuffer for present
                s_uiRenderModeNextFrame = UICompositionMode::No_UI_Handling;
            }
            else
            {
				// restore ui render mode to what it was before deactivation
				s_uiRenderModeNextFrame = s_uiRenderModeAtDeactivation;
            }

            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        },
        false));

    // get FG version info from ffxapi
    if (m_FrameInterpolation)
    {
#if defined(_VK)
        void* device = &deviceContext;
#elif defined(_DX12)
        void* device = GetDevice()->GetImpl()->DX12Device();
#endif
        QueryVersionNames(
            FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION,
            device,
            m_FgVersionIds,
            m_FgVersionNames);

        m_currentFgContextVersion = m_FgVersionNames[m_FgVersionIndex].c_str();
        m_FrameGenerationDebugViewEnabled = m_FrameInterpolation && m_currentFgContextVersion.major < 4;

        // Check FG SwapChain versions
    }

    std::vector<const char*> fgVersionNames = makeStringViews(m_FgVersionNames);
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "Frame Generation Version", (int32_t&)m_FgVersionIndex, fgVersionNames, m_overrideVersion, [this](int32_t next, int32_t)
        {
            m_currentFgContextVersion = m_FgVersionNames[next].c_str();
            m_NeedReInit = true;
            m_FrameGenerationDebugViewEnabled = m_FrameInterpolation && m_currentFgContextVersion.major < 4;
        }, false));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Support Async Compute", m_PendingEnableAsyncCompute,
        m_AsyncComputeAvailable,
        [this](bool, bool) 
        {
            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        }, 
        false));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Allow async compute", m_AllowAsyncCompute, m_PendingEnableAsyncCompute, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Use callback",
        m_UseCallback,
        m_FrameInterpolation,
        [this](bool, bool)
        {
            GetFramework()->EnableSwapchainFrameGenerationCallback(m_UseCallback);
        },
        false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Use Distortion Field Input", m_UseDistortionField, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw frame generation tear lines", m_DrawFrameGenerationDebugTearLines, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw frame generation pacing lines", m_DrawFrameGenerationDebugPacingLines, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw frame generation reset indicators", m_DrawFrameGenerationDebugResetIndicators, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Draw frame generation debug view", m_DrawFrameGenerationDebugView, m_FrameGenerationDebugViewEnabled,nullptr,false));
    
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("Present interpolated only", m_PresentInterpolatedOnly, m_FrameInterpolation, nullptr, false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UIButton>("Reset Frame Interpolation", m_FrameInterpolation, [this]() { m_ResetFrameInterpolation = true; }));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UIButton>("Simulate present skip", m_FrameInterpolation, [this]() { m_SimulatePresentSkip = true; }));

    std::vector<const char*>        uiRenderModeLabels = { "No UI handling (not recommended)", "UiTexture", "UiCallback", "Pre-Ui Backbuffer" };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "UI Composition Mode",
        s_uiRenderModeNextFrame,
        uiRenderModeLabels, 
        m_FrameInterpolation,
        [this](int32_t, int32_t) 
        {
            // Ask main loop to re-initialize at start of next frame. Next frame set s_uiRenderMode = s_uiRenderModeNextFrame.
            m_NeedReInit = true;
        },
        false));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>("DoubleBuffer UI resource in swapchain", m_DoublebufferInSwapchain, m_FrameInterpolation, nullptr, false));

    std::vector<const char*>        waitCallbackModeLabels = { "nullptr", "CAUDRON_LOG_DEBUG(\"waitCallback\")"};
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "WaitCallback Mode",
        m_waitCallbackMode,
        waitCallbackModeLabels,
        m_EnableWaitCallbackModeUI,
        [this](int32_t, int32_t)
        {
#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueVK m_swapchainKeyValueConfig{};
#elif defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueDX12 m_swapchainKeyValueConfig{};
#endif
            m_swapchainKeyValueConfig.key = FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK;
            if (m_waitCallbackMode == 0)
            {
                m_swapchainKeyValueConfig.ptr = nullptr;
            }
            else if (m_waitCallbackMode == 1)
            {
                m_swapchainKeyValueConfig.ptr = waitCallback;
            }
            ffx::Configure(m_SwapChainContext, m_swapchainKeyValueConfig);

        },
        m_EnableMaskOptions));


    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Frame Pacing safetyMarginInMs",
        m_SafetyMarginInMs,
        0.0f, 1.0f,
        m_FrameInterpolation,
        [this](float, float) {
#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueVK m_swapchainKeyValueConfig{};
#elif defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueDX12 m_swapchainKeyValueConfig{};
#endif
            m_swapchainKeyValueConfig.key = FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING;
            m_swapchainKeyValueConfig.ptr = &framePacingTuning;

            framePacingTuning.safetyMarginInMs = m_SafetyMarginInMs;

            ffx::Configure(m_SwapChainContext, m_swapchainKeyValueConfig);
        },
        m_FrameInterpolation));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<float>>(
        "Frame Pacing varianceFactor",
        m_VarianceFactor,
        0.0f, 1.0f,
        m_FrameInterpolation,
        [this](float, float) {
#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueVK m_swapchainKeyValueConfig{};
#elif defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueDX12 m_swapchainKeyValueConfig{};
#endif
            m_swapchainKeyValueConfig.key = FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING;
            m_swapchainKeyValueConfig.ptr = &framePacingTuning;

            framePacingTuning.varianceFactor = m_VarianceFactor;

            ffx::Configure(m_SwapChainContext, m_swapchainKeyValueConfig);
        },
        m_FrameInterpolation));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>(
        "Frame Pacing allowHybridSpin", 
        m_AllowHybridSpin, 
        m_FrameInterpolation,
        [this](bool, bool) {
#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueVK m_swapchainKeyValueConfig{};
#elif defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueDX12 m_swapchainKeyValueConfig{};
#endif
            m_swapchainKeyValueConfig.key = FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING;
            m_swapchainKeyValueConfig.ptr = &framePacingTuning;

            framePacingTuning.allowHybridSpin = m_AllowHybridSpin;

            ffx::Configure(m_SwapChainContext, m_swapchainKeyValueConfig);
        }));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UISlider<int32_t>>(
        "hybridSpinTime in timer resolution units",
        (int32_t&) m_HybridSpinTime,
        0, 10,
        m_FrameInterpolation,
        [this](int32_t, int32_t) {
#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueVK m_swapchainKeyValueConfig{};
#elif defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueDX12 m_swapchainKeyValueConfig{};
#endif
            m_swapchainKeyValueConfig.key = FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING;
            m_swapchainKeyValueConfig.ptr = &framePacingTuning;

            framePacingTuning.hybridSpinTime = m_HybridSpinTime;

            ffx::Configure(m_SwapChainContext, m_swapchainKeyValueConfig);
        },
        m_FrameInterpolation));
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>(
        "allowWaitForSingleObjectOnFence",
        m_AllowWaitForSingleObjectOnFence,
        m_FrameInterpolation,
        [this](bool, bool) {
#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueVK m_swapchainKeyValueConfig{};
#elif defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainKeyValueDX12 m_swapchainKeyValueConfig{};
#endif
            m_swapchainKeyValueConfig.key = FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING;
            m_swapchainKeyValueConfig.ptr = &framePacingTuning;

            framePacingTuning.allowWaitForSingleObjectOnFence = m_AllowWaitForSingleObjectOnFence;

            ffx::Configure(m_SwapChainContext, m_swapchainKeyValueConfig);
        }));

    // Camera animation options
    std::vector<const char*> uiAnimationModesLabels = { "None", "Sinusoidal" /*, "Circle"*/ };
    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICombo>(
        "Camera animation mode", (int32_t&)m_CameraAnimationMode, uiAnimationModesLabels, m_FrameInterpolation,
        [this](int32_t, int32_t)
        {
            auto pCamera = cauldron::GetScene()->GetCurrentCamera();
            pCamera->SetAnimation(static_cast<CameraAnimation>(m_CameraAnimationMode),
                static_cast<CameraAnimationRotationDirection>(m_ChangeCameraAnimationDirection), // there are only two directions, so use the bool directly
                m_EnableCameraAnimationNoise);
        }, false));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>(
        "Change animation direction",
        m_ChangeCameraAnimationDirection,
        m_FrameInterpolation,
        [this](bool, bool) {
            auto pCamera = cauldron::GetScene()->GetCurrentCamera();
            pCamera->SetAnimation(static_cast<CameraAnimation>(m_CameraAnimationMode),
                static_cast<CameraAnimationRotationDirection>(m_ChangeCameraAnimationDirection), // there are only two directions, so use the bool directly
                m_EnableCameraAnimationNoise);
        }));

    m_UIElements.emplace_back(pUISection->RegisterUIElement<UICheckBox>(
        "Enable camera animation noise",
        m_EnableCameraAnimationNoise,
        m_FrameInterpolation,
        [this](bool, bool) {
            auto pCamera = cauldron::GetScene()->GetCurrentCamera();
            pCamera->SetAnimation(static_cast<CameraAnimation>(m_CameraAnimationMode),
                static_cast<CameraAnimationRotationDirection>(m_ChangeCameraAnimationDirection), // there are only two directions, so use the bool directly
                m_EnableCameraAnimationNoise);
        }));

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
        m_EnableMaskOptions = false;
        break;
    case 1:
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

    m_EnableWaitCallbackModeUI = m_EnableMaskOptions && m_FrameInterpolationAvailable;

    m_UpscaleMethod = newUpscaler;

    // Enable the new one
    EnableModule(true);
    ClearReInit();
}

void FSRRenderModule::UpdatePreset(const int32_t* pOldPreset)
{
    if ((pOldPreset && *pOldPreset == static_cast<int32_t>(FSRScalePreset::CustomDRS)) || m_ScalePreset == FSRScalePreset::CustomDRS)
    {
        m_NeedReInit = true;
    }

    switch (m_ScalePreset)
    {
    case FSRScalePreset::NativeAA:
    case FSRScalePreset::Quality:
    case FSRScalePreset::Balanced:
    case FSRScalePreset::Performance:
    case FSRScalePreset::UltraPerformance:
    {
        // Query the upscale ratio from the provider instead of using hard-coded values
        float queryRatio = 0.0f;
        ffx::ReturnCode retCode;

#if defined(_DX12)
        // Chain ffxCreateBackendDX12Desc to provide device for null-context query to reach external driver provider
        struct ffxCreateBackendDX12Desc backendDX12Desc = {};
        backendDX12Desc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backendDX12Desc.device = GetDevice()->GetImpl()->DX12Device();
#elif defined(_VK)
        struct ffxCreateBackendVKDesc backendVKDesc = {};
        backendVKDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
        
        VkDeviceContext deviceContext;
        deviceContext.vkDevice = GetDevice()->GetImpl()->VKDevice();
        deviceContext.vkPhysicalDevice = GetDevice()->GetImpl()->VKPhysicalDevice();
        deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;

        backendVKDesc.deviceContext = deviceContext;
#endif

        ffx::QueryDescUpscaleGetUpscaleRatioFromQualityMode ratioDesc{};
        ratioDesc.qualityMode = static_cast<uint32_t>(m_ScalePreset);
        ratioDesc.pOutUpscaleRatio = &queryRatio;

        if (m_FsrVersionIndex < m_FsrVersionIds.size() && m_overrideVersion)
        {
            ffx::CreateContextDescOverrideVersion versionOverride{};
            versionOverride.versionId = m_FsrVersionIds[m_FsrVersionIndex];
            {
#if defined(_DX12)
                retCode = ffx::Query(ratioDesc, versionOverride, backendDX12Desc);
#elif defined(_VK)
                retCode = ffx::Query(ratioDesc, versionOverride, backendVKDesc);
#endif
                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                    L"ffxQuery(nullptr,UpscaleGetUpscaleRatioFromQualityMode,%S,backend) returned %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), (uint32_t)retCode);
            }
        }
        else
        {
            {
#if defined(_DX12)
                retCode = ffx::Query(ratioDesc, backendDX12Desc);
#elif defined(_VK)
                retCode = ffx::Query(ratioDesc, backendVKDesc);
#endif
                CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                    L"ffxQuery(nullptr,UpscaleGetUpscaleRatioFromQualityMode,backend) returned %d", (uint32_t)retCode);
            }
        }        

        if (retCode == ffx::ReturnCode::Ok && queryRatio > 0.0f)
        {
            m_UpscaleRatio = queryRatio;
        }
        break;
    }
    case FSRScalePreset::CustomDRS:
    case FSRScalePreset::Custom:
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update whether we can update the custom scale slider
    m_UpscaleRatioEnabled = (m_ScalePreset == FSRScalePreset::Custom || m_ScalePreset == FSRScalePreset::CustomDRS);

    // Update mip bias
    float oldValue = m_MipBias;
    if (m_ScalePreset != FSRScalePreset::Custom && m_ScalePreset != FSRScalePreset::CustomDRS)
        m_MipBias = cMipBias[static_cast<uint32_t>(m_ScalePreset)];
    else
        m_MipBias = m_MipBias = CalculateMipBias(m_UpscaleRatio);
    UpdateMipBias(&oldValue);

    // Update resolution since rendering ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);

    GetFramework()->EnableFrameInterpolation(m_FrameInterpolation);

    GetFramework()->EnableSwapchainFrameGenerationCallback(m_UseCallback);
}

void FSRRenderModule::UpdateUpscaleRatio(const float*)
{
#if defined(_VK)
    void* ffxSwapChain = GetSwapChain()->GetImpl()->VKSwapChain();
#elif defined(_DX12)
    void* ffxSwapChain = GetSwapChain()->GetImpl()->DX12SwapChain();
#endif
    
    m_FrameGenerationConfig.frameGenerationEnabled = false;
    m_FrameGenerationConfig.swapChain = ffxSwapChain;
    m_FrameGenerationConfig.presentCallback = nullptr;
    m_FrameGenerationConfig.HUDLessColor = FfxApiResource({});
    auto retCode = ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);
    CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Configuring FSR FG before destroy failed: %d", (uint32_t)retCode);

    // Disable/Enable FSR since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSRRenderModule::UpdateMipBias(const float*)
{
    // Update the scene MipLODBias to use
    GetScene()->SetMipLODBias(m_MipBias);
}

void FSRRenderModule::FfxMsgCallback(uint32_t type, const wchar_t* message)
{
    if (type == FFX_API_MESSAGE_TYPE_ERROR)
    {
        CauldronError(L"FSR_API_DEBUG_ERROR: %ls", message);
    }
    else if (type == FFX_API_MESSAGE_TYPE_WARNING)
    {
        CauldronWarning(L"FSR_API_DEBUG_WARNING: %ls", message);
    }
}

ffxReturnCode_t FSRRenderModule::UiCompositionCallback(ffxCallbackDescFrameGenerationPresent* params)
{
    if (s_uiRenderMode != UICompositionMode::UiCallback)
        return FFX_API_RETURN_ERROR_PARAMETER;

    // Get a handle to the UIRenderModule for UI composition (only do this once as there's a search cost involved)
    if (!m_pUIRenderModule)
    {
        m_pUIRenderModule = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        CauldronAssert(ASSERT_CRITICAL, m_pUIRenderModule, L"Could not get a handle to the UIRenderModule.");
    }
    
    // Wrap everything in cauldron wrappers to allow backend agnostic execution of UI render.
    CommandList* pCmdList = CommandList::GetWrappedCmdListFromSDK(L"UI_CommandList", CommandQueue::Graphics, params->commandList);
    SetAllResourceViewHeaps(pCmdList); // Set the resource view heaps to the wrapped command list

    ResourceState rtResourceState = SDKWrapper::GetFrameworkState((FfxApiResourceState)params->outputSwapChainBuffer.state);
    ResourceState bbResourceState = SDKWrapper::GetFrameworkState((FfxApiResourceState)params->currentBackBuffer.state);

    TextureDesc rtDesc = SDKWrapper::GetFrameworkTextureDescription(params->outputSwapChainBuffer.description);
    TextureDesc bbDesc = SDKWrapper::GetFrameworkTextureDescription(params->currentBackBuffer.description);

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
    FfxApiResourceDescription rdesc = params->outputSwapChainBuffer.description;

    TextureDesc rtResourceDesc = TextureDesc::Tex2D(L"UI_RenderTarget",
                                                    SDKWrapper::GetFrameworkSurfaceFormat((FfxApiSurfaceFormat)rdesc.format),
                                                    rdesc.width,
                                                    rdesc.height,
                                                    rdesc.depth,
                                                    rdesc.mipCount,
                                                    ResourceFlags::AllowRenderTarget);
    m_pRTResourceView->BindTextureResource(pRTResource, rtResourceDesc, ResourceViewType::RTV, ViewDimension::Texture2D, 0, 1, 0);

    ResourceViewInfo viewInfo = m_pRTResourceView->GetViewInfo(0);
    m_pUIRenderModule->ExecuteAsync(pCmdList, &viewInfo);

    Barrier barrier = Barrier::Transition(pRTResource, ResourceState::RenderTargetResource, rtResourceState);
    ResourceBarrier(pCmdList, 1, &barrier);

    // Clean up wrapped resources for the frame
    delete pBBResource;
    delete pRTResource;
    delete pCmdList;

    return FFX_API_RETURN_OK;
}

void FSRRenderModule::UpdateFSRContext(bool enabled)
{
    if (enabled)
    {
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        static bool s_InvertedDepth = GetConfig()->InvertedDepth;
        // Backend creation (for both FFXAPI contexts, FG and Upscale)

        DeviceInternal* device = GetDevice()->GetImpl();
#if defined(_VK)
        ffx::CreateBackendVKDesc backendDesc{};
        backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
        VkDeviceContext deviceContext;
        deviceContext.vkDevice = GetDevice()->GetImpl()->VKDevice();
        deviceContext.vkPhysicalDevice = GetDevice()->GetImpl()->VKPhysicalDevice();
        deviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
        backendDesc.deviceContext = deviceContext;
#elif defined(_DX12)
        ffx::CreateBackendDX12Desc backendDesc{};
        backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backendDesc.device = device->DX12Device();
#endif

        if (m_UpscaleMethod == Upscaler_FSRAPI)
        {
            ffx::CreateContextDescUpscale createFsr{};
            ffx::ReturnCode retCode;
            ffxReturnCode_t retCode_t;

            createFsr.maxRenderSize  = {resInfo.RenderWidth, resInfo.RenderHeight};
            createFsr.maxUpscaleSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
            createFsr.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
            createFsr.flags |= FFX_UPSCALE_ENABLE_DEBUG_VISUALIZATION;

            if (m_ScalePreset == FSRScalePreset::CustomDRS)
            {
                createFsr.flags |= FFX_UPSCALE_ENABLE_DYNAMIC_RESOLUTION;
            }
            if (s_InvertedDepth)
            {
                createFsr.flags |= FFX_UPSCALE_ENABLE_DEPTH_INVERTED | FFX_UPSCALE_ENABLE_DEPTH_INFINITE;
            }
            createFsr.flags |= FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
            createFsr.flags |= m_colorSpace == FSRColorSpace::NonLinearColorSpace ? FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE : 0;
            createFsr.fpMessage = nullptr;

            if (m_GlobalDebugCheckerMode != FSRDebugCheckerMode::Disabled)
            {
                createFsr.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
            }

            // Create the FSR context
            {
                //Before creating any of FSR contexts, query VRAM size
                struct FfxApiEffectMemoryUsage gpuMemoryUsageUpscaler = {0};
                struct ffxQueryDescUpscaleGetGPUMemoryUsageV2 upscalerGetGPUMemoryUsageV2 = {0};
                upscalerGetGPUMemoryUsageV2.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2;
#if defined(_VK)
                VkDeviceContext vkDeviceContext = {};
                vkDeviceContext.vkDevice         = cauldron::GetDevice()->GetImpl()->VKDevice();
                vkDeviceContext.vkPhysicalDevice = cauldron::GetDevice()->GetImpl()->VKPhysicalDevice();
                vkDeviceContext.vkDeviceProcAddr = vkGetDeviceProcAddr;
                upscalerGetGPUMemoryUsageV2.device = reinterpret_cast<void*>(&vkDeviceContext);
#elif defined(_DX12)
                upscalerGetGPUMemoryUsageV2.device = GetDevice()->GetImpl()->DX12Device();
#endif
                upscalerGetGPUMemoryUsageV2.maxRenderSize = { resInfo.RenderWidth, resInfo.RenderHeight };
                upscalerGetGPUMemoryUsageV2.maxUpscaleSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
                upscalerGetGPUMemoryUsageV2.flags = createFsr.flags;
                upscalerGetGPUMemoryUsageV2.gpuMemoryUsageUpscaler = &gpuMemoryUsageUpscaler;

                uint32_t renderResolution[2] = { 0, 0 };
                ffx::QueryDescUpscaleGetRenderResolutionFromQualityMode renderResDesc;
                renderResDesc.qualityMode = static_cast<uint32_t>(m_ScalePreset);
                renderResDesc.displayHeight = resInfo.DisplayHeight;
				renderResDesc.displayWidth = resInfo.DisplayWidth;
				renderResDesc.pOutRenderWidth = &renderResolution[0];
				renderResDesc.pOutRenderHeight = &renderResolution[1];

                ffx::CreateContextDescUpscaleVersion headerVersion = {};
                headerVersion.version = FFX_UPSCALER_VERSION;

                // lifetime of this must last until after CreateContext call!
                struct ffxOverrideVersion versionOverride = {0};
                versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
                if (m_FsrVersionIndex < m_FsrVersionIds.size() && m_overrideVersion)
                {
                    versionOverride.versionId = m_FsrVersionIds[m_FsrVersionIndex];
                    upscalerGetGPUMemoryUsageV2.header.pNext = &versionOverride.header;
                    retCode_t = ffxQuery(nullptr, &upscalerGetGPUMemoryUsageV2.header);
                    CauldronAssert(ASSERT_WARNING, retCode_t == FFX_API_RETURN_OK,
                        L"ffxQuery(nullptr,UpscaleGetGPUMemoryUsageV2, %S) returned %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), retCode_t);
                    CAUDRON_LOG_INFO(L"Upscaler version %S Query GPUMemoryUsageV2 VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", m_FsrVersionNames[m_FsrVersionIndex].c_str(), gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f, gpuMemoryUsageUpscaler.aliasableUsageInBytes / 1048576.f);
                    
                    versionOverride.header.pNext = &backendDesc.header;
                    renderResDesc.header.pNext = &versionOverride.header;
                    // Query render resolution from display resolution via driver override path (null-context)
                    retCode_t = ffxQuery(nullptr, &renderResDesc.header);
                    CauldronAssert(ASSERT_WARNING, retCode_t == FFX_API_RETURN_OK,
                        L"ffxQuery(nullptr,QueryDescUpscaleGetRenderResolutionFromQualityMode, %S) returned %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), retCode_t);
					CAUDRON_LOG_INFO(L"Upscaler version %S QueryDescUpscaleGetRenderResolutionFromQualityMode qualityMode %d renderResolution %d x %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), static_cast<uint32_t>(m_ScalePreset), renderResolution[0], renderResolution[1]);
                    
                    retCode = ffx::CreateContext(m_UpscalingContext, nullptr, createFsr, backendDesc, headerVersion, versionOverride);
                }
                else
                {
                    retCode = ffx::Query(upscalerGetGPUMemoryUsageV2);
                    CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                        L"ffxQuery(nullptr,UpscaleGetGPUMemoryUsageV2) returned %d", (uint32_t)retCode);
                    CAUDRON_LOG_INFO(L"Default Upscaler Query GPUMemoryUsageV2 totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f, gpuMemoryUsageUpscaler.aliasableUsageInBytes / 1048576.f);

                    retCode_t = ffxQuery(nullptr, &renderResDesc.header);
                    CauldronAssert(ASSERT_WARNING, retCode_t == FFX_API_RETURN_OK,
                        L"ffxQuery(nullptr,QueryDescUpscaleGetRenderResolutionFromQualityMode, %S) returned %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), retCode_t);
					CAUDRON_LOG_INFO(L"Default Upscaler QueryDescUpscaleGetRenderResolutionFromQualityMode qualityMode %d renderResolution %d x %d", static_cast<uint32_t>(m_ScalePreset), renderResolution[0], renderResolution[1]);

                    retCode = ffx::CreateContext(m_UpscalingContext, nullptr, createFsr, backendDesc, headerVersion);
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

            CAUDRON_LOG_INFO(L"Upscaler Context versionid 0x%016llx, %S", m_currentUpscaleContextVersionId, m_currentUpscaleContextVersionName);

            for (uint32_t i = 0; i < m_FsrVersionIds.size(); i++)
            {
                if (m_FsrVersionIds[i] == m_currentUpscaleContextVersionId)
                {
                    m_FsrVersionIndex = i;
                }
            }

        }

        // Pre-context FG swapchain VRAM estimate. The query does not use m_SwapChainContext.
        {
            FfxApiResource uiColor = SDKWrapper::ffxGetResourceApi(m_pUiTexture[0]->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            const bool useUiTexture = s_uiRenderMode == UICompositionMode::UiTexture;
            const FfxApiDimensions2D uiResourceSize = {
                useUiTexture ? uiColor.description.width : 0,
                useUiTexture ? uiColor.description.height : 0};
            const uint32_t uiResourceFormat = useUiTexture ? uiColor.description.format : FFX_API_SURFACE_FORMAT_UNKNOWN;
            const uint32_t uiFlags = m_DoublebufferInSwapchain ? FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING : 0;

            FfxApiEffectMemoryUsage gpuMemoryUsageSwapChainV2 = {};
            ffx::ReturnCode retCode = ffx::ReturnCode::ErrorParameter;
#if defined(_VK)
            const VkSwapchainCreateInfoKHR* swapChainInfo = GetSwapChain()->GetImpl()->GetCreateInfo();
            ffx::QueryFrameGenerationSwapChainGetGPUMemoryUsageVKV2 swapChainGetGPUMemoryUsageV2{};
            swapChainGetGPUMemoryUsageV2.device = GetDevice()->GetImpl()->VKDevice();
            swapChainGetGPUMemoryUsageV2.physicalDevice = GetDevice()->GetImpl()->VKPhysicalDevice();
            swapChainGetGPUMemoryUsageV2.displaySize = { swapChainInfo->imageExtent.width, swapChainInfo->imageExtent.height };
            swapChainGetGPUMemoryUsageV2.backBufferFormat = ffxApiGetSurfaceFormatVK(swapChainInfo->imageFormat);
            swapChainGetGPUMemoryUsageV2.backBufferCount = swapChainInfo->minImageCount;
            swapChainGetGPUMemoryUsageV2.composeOnGameQueue = 0;
#elif defined(_DX12)
            IDXGISwapChain4* dxgiSwapchain = GetSwapChain()->GetImpl()->DX12SwapChain();
            DXGI_SWAP_CHAIN_DESC1 desc1;
            dxgiSwapchain->GetDesc1(&desc1);
            ffx::QueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2 swapChainGetGPUMemoryUsageV2{};
            swapChainGetGPUMemoryUsageV2.device = GetDevice()->GetImpl()->DX12Device();
            swapChainGetGPUMemoryUsageV2.displaySize = { desc1.Width, desc1.Height };
            swapChainGetGPUMemoryUsageV2.backBufferFormat = ffxApiGetSurfaceFormatDX12(desc1.Format);
            swapChainGetGPUMemoryUsageV2.backBufferCount = desc1.BufferCount;
#endif
            swapChainGetGPUMemoryUsageV2.uiResourceSize = uiResourceSize;
            swapChainGetGPUMemoryUsageV2.uiResourceFormat = uiResourceFormat;
            swapChainGetGPUMemoryUsageV2.flags = uiFlags;
            swapChainGetGPUMemoryUsageV2.gpuMemoryUsageFrameGenerationSwapchain = &gpuMemoryUsageSwapChainV2;

            retCode = ffx::Query(swapChainGetGPUMemoryUsageV2);
            CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                L"ffxQuery(nullptr,SwapChainGetGPUMemoryUsageV2) returned %d", (uint32_t)retCode);
            CAUDRON_LOG_INFO(L"Swapchain Query GPUMemoryUsageV2 VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageSwapChainV2.totalUsageInBytes / 1048576.f, gpuMemoryUsageSwapChainV2.aliasableUsageInBytes / 1048576.f);
        }

        // Create the FrameGen context
        if (m_FrameInterpolationAvailable)
        {
            // Acquire lock before creating FG context
            cauldron::FGContextScopeLock lock(true);
            ffx::CreateContextDescFrameGeneration createFg{};
            ffx::ReturnCode retCode;

            createFg.displaySize = { resInfo.DisplayWidth, resInfo.DisplayHeight };
            createFg.maxRenderSize = { resInfo.DisplayWidth, resInfo.DisplayHeight };
            if (s_InvertedDepth)
            {
                createFg.flags |= FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED | FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE;
            }
            createFg.flags |= FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;

            m_EnableAsyncCompute = m_PendingEnableAsyncCompute;
            if (m_EnableAsyncCompute)
            {
                createFg.flags |= FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT;
            }
            if (m_GlobalDebugCheckerMode != FSRDebugCheckerMode::Disabled)
            {
                createFg.flags |= FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING;
            }

            createFg.backBufferFormat = SDKWrapper::GetFfxSurfaceFormat(cauldron::GetSwapChain()->GetSwapChainFormat());

            m_FrameGenerationConfig.flags = 0u;
            m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugTearLines ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
            m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugResetIndicators ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS : 0;
            m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugPacingLines ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES : 0;
            m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
            m_FrameGenerationConfig.flags |= !m_SwapChainContext ? FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY : 0;

            //Before creating FG context, query VRAM size
            FfxApiEffectMemoryUsage gpuMemoryUsageFrameGenerationV2;
            ffx::QueryDescFrameGenerationGetGPUMemoryUsageV2 frameGenGetGPUMemoryUsageV2{};

#if defined(_VK)
            VkDeviceContext vkDeviceContext = {};
            vkDeviceContext.vkDevice = cauldron::GetDevice()->GetImpl()->VKDevice();
            vkDeviceContext.vkPhysicalDevice = cauldron::GetDevice()->GetImpl()->VKPhysicalDevice();
            frameGenGetGPUMemoryUsageV2.device = reinterpret_cast<void*>(&vkDeviceContext);
#elif defined(_DX12)
            frameGenGetGPUMemoryUsageV2.device = GetDevice()->GetImpl()->DX12Device();
#endif
            frameGenGetGPUMemoryUsageV2.maxRenderSize = createFg.maxRenderSize;
            frameGenGetGPUMemoryUsageV2.displaySize = createFg.displaySize;
            frameGenGetGPUMemoryUsageV2.createFlags = createFg.flags;
            frameGenGetGPUMemoryUsageV2.dispatchFlags = m_FrameGenerationConfig.flags;
            frameGenGetGPUMemoryUsageV2.backBufferFormat = createFg.backBufferFormat;
            frameGenGetGPUMemoryUsageV2.gpuMemoryUsageFrameGeneration = &gpuMemoryUsageFrameGenerationV2;
            
			ffx::CreateContextDescFrameGenerationVersion headerVersion{};
            headerVersion.version = FFX_FRAMEGENERATION_VERSION;
            if (s_uiRenderMode == UICompositionMode::PreUiBackbuffer)
            {
                ffx::CreateContextDescFrameGenerationHudless createFgHudless{};
                createFgHudless.hudlessBackBufferFormat = SDKWrapper::GetFfxSurfaceFormat(m_pHudLessTexture[0]->GetResource()->GetTextureResource()->GetFormat());
                frameGenGetGPUMemoryUsageV2.backBufferFormat = createFgHudless.hudlessBackBufferFormat;

                if (m_FgVersionIndex < m_FgVersionIds.size() && m_overrideVersion)
                {
                    struct ffxOverrideVersion versionOverride = { 0 };
                    versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
                    versionOverride.versionId = m_FgVersionIds[m_FgVersionIndex];
                    retCode = ffx::Query(frameGenGetGPUMemoryUsageV2, versionOverride);
                    CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                        L"ffx::Query(FrameGenerationGetGPUMemoryUsageV2, %S) returned %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), (uint32_t)retCode);
                    retCode = ffx::CreateContext(m_FrameGenContext, nullptr, createFg, backendDesc, headerVersion, createFgHudless, versionOverride);
                }
                else
                {
                    retCode = ffx::Query(frameGenGetGPUMemoryUsageV2);
                    CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                        L"ffx::Query(FrameGenerationGetGPUMemoryUsageV2) returned %d", (uint32_t)retCode);
                    retCode = ffx::CreateContext(m_FrameGenContext, nullptr, createFg, backendDesc, headerVersion, createFgHudless);
                }
            }
            else
            {
                frameGenGetGPUMemoryUsageV2.hudlessBackBufferFormat = FFX_API_SURFACE_FORMAT_UNKNOWN;

                if (m_FgVersionIndex < m_FgVersionIds.size() && m_overrideVersion)
                {
                    struct ffxOverrideVersion versionOverride = { 0 };
                    versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
                    versionOverride.versionId = m_FgVersionIds[m_FgVersionIndex];
                    retCode = ffx::Query(frameGenGetGPUMemoryUsageV2, versionOverride);
                    CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                        L"ffx::Query(FrameGenerationGetGPUMemoryUsageV2, %S) returned %d", m_FsrVersionNames[m_FsrVersionIndex].c_str(), (uint32_t)retCode);
                    retCode = ffx::CreateContext(m_FrameGenContext, nullptr, createFg, backendDesc, headerVersion, versionOverride);
                }
                else
                {
                    retCode = ffx::Query(frameGenGetGPUMemoryUsageV2);
                    CauldronAssert(ASSERT_WARNING, retCode == ffx::ReturnCode::Ok,
                        L"ffx::Query(FrameGenerationGetGPUMemoryUsageV2) returned %d", (uint32_t)retCode);
                    retCode = ffx::CreateContext(m_FrameGenContext, nullptr, createFg, backendDesc, headerVersion);
                }
                
            }

            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't create the ffxapi framegen context: %d", (uint32_t)retCode);

            CAUDRON_LOG_INFO(L"FrameGenerationGetGPUMemoryUsageV2 VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageFrameGenerationV2.totalUsageInBytes / 1048576.f, gpuMemoryUsageFrameGenerationV2.aliasableUsageInBytes / 1048576.f);

#if defined(_VK)
            void* ffxSwapChain = GetSwapChain()->GetImpl()->VKSwapChain();
#elif defined(_DX12)
            void* ffxSwapChain = GetSwapChain()->GetImpl()->DX12SwapChain();
#endif

            // Configure frame generation
            FfxApiResource hudLessResource = SDKWrapper::ffxGetResourceApi(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

            m_FrameGenerationConfig.frameGenerationEnabled  = false;
            m_FrameGenerationConfig.frameGenerationCallback = [](ffxDispatchDescFrameGeneration* params, void* pUserCtx) -> ffxReturnCode_t
            {
                return ffxDispatch(reinterpret_cast<ffxContext*>(pUserCtx), &params->header);
            };
            m_FrameGenerationConfig.frameGenerationCallbackUserContext = &m_FrameGenContext;
            if (s_uiRenderMode == UICompositionMode::UiCallback)
            {
                m_FrameGenerationConfig.presentCallback = [](ffxCallbackDescFrameGenerationPresent* params, void* self) -> auto { return reinterpret_cast<FSRRenderModule*>(self)->UiCompositionCallback(params); };
                m_FrameGenerationConfig.presentCallbackUserContext = this;
            }
            else
            {
                m_FrameGenerationConfig.presentCallback = nullptr;
                m_FrameGenerationConfig.presentCallbackUserContext = nullptr;
            }
            m_FrameGenerationConfig.swapChain               = ffxSwapChain;
            m_FrameGenerationConfig.HUDLessColor           = (s_uiRenderMode == UICompositionMode::PreUiBackbuffer) ? hudLessResource : FfxApiResource({});

            m_FrameGenerationConfig.frameID = m_FrameID;

            retCode = ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);
            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, 
                L"ffx::Configure(FrameGenContext,FrameGenerationConfig) returned %d", (uint32_t)retCode);

        }
    }
    else
    {
        ffx::ReturnCode retCode;
        if (m_FrameInterpolationAvailable)
        {
            // Acquire lock before destroying FG context
            cauldron::FGContextScopeLock lock(true);

#if defined(_VK)
            void* ffxSwapChain = GetSwapChain()->GetImpl()->VKSwapChain();
#elif defined(_DX12)
            void* ffxSwapChain = GetSwapChain()->GetImpl()->DX12SwapChain();
#endif

            // disable frame generation before destroying context
            // also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
            m_FrameGenerationConfig.frameGenerationEnabled = false;
            m_FrameGenerationConfig.swapChain = ffxSwapChain;
            m_FrameGenerationConfig.presentCallback = nullptr;
            m_FrameGenerationConfig.HUDLessColor = FfxApiResource({});
            retCode = ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);
            CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Configuring FSR FG before destroy failed: %d", (uint32_t)retCode);

#if defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceVK uiConfig{};
            uiConfig.uiResource = {};
            ffx::Configure(m_SwapChainContext, uiConfig);
#endif
            
            ffx::DestroyContext(m_FrameGenContext);
            m_FrameGenContext = nullptr;
        }

        if (m_UpscalingContext)
        {
            ffx::DestroyContext(m_UpscalingContext);
            m_UpscalingContext = nullptr;
        }
    }
    
}

void FSRRenderModule::SetUpscaleConstantBuffer(uint64_t key, float value)
{
    ffx::ConfigureDescUpscaleKeyValue m_upscalerKeyValueConfig{};
    m_upscalerKeyValueConfig.key = key;
    m_upscalerKeyValueConfig.ptr = &value;
    ffx::Configure(m_UpscalingContext, m_upscalerKeyValueConfig);
}

void FSRRenderModule::SetGlobalDebugCheckerMode(FSRDebugCheckerMode mode, bool recreate)
{
    ffx::ConfigureDescGlobalDebug m_GlobalDebugConfig{};
    if (mode == FSRDebugCheckerMode::Disabled)
    {
        m_GlobalDebugConfig.fpMessage = nullptr;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_WARNINGS;
    }
    else if (mode == FSRDebugCheckerMode::EnabledNoMessageCallbackSilence)
    {
        m_GlobalDebugConfig.fpMessage = nullptr;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_SILENCE;
    }
    else if (mode == FSRDebugCheckerMode::EnabledNoMessageCallbackErrors)
    {
        m_GlobalDebugConfig.fpMessage = nullptr;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_ERRORS;
    }
    else if (mode == FSRDebugCheckerMode::EnabledNoMessageCallbackWarnings)
    {
        m_GlobalDebugConfig.fpMessage = nullptr;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_WARNINGS;
    }

    else if (mode == FSRDebugCheckerMode::EnabledWithMessageCallbackSilence)
    {
        m_GlobalDebugConfig.fpMessage = &FSRRenderModule::FfxMsgCallback;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_SILENCE;
    }
    else if (mode == FSRDebugCheckerMode::EnabledWithMessageCallbackErrors)
    {
        m_GlobalDebugConfig.fpMessage = &FSRRenderModule::FfxMsgCallback;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_ERRORS;
    }
    else if (mode == FSRDebugCheckerMode::EnabledWithMessageCallbackWarnings)
    {
        m_GlobalDebugConfig.fpMessage = &FSRRenderModule::FfxMsgCallback;
        m_GlobalDebugConfig.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_WARNINGS;
    }

    if (m_UpscalingContext)
    {
        if (recreate)
        {
            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        }

        m_GlobalDebugConfig.effectId = FFX_API_EFFECT_ID_UPSCALE;
        ffx::ReturnCode retCode = ffx::Configure(m_UpscalingContext, m_GlobalDebugConfig);
        CauldronAssert(ASSERT_ERROR, retCode == ffx::ReturnCode::Ok, L"Couldn't configure global debug config on the ffxapi upscaling context: %d", (uint32_t)retCode);
    }
    if (m_FrameGenContext)
    {
        if (recreate)
        {
            // Ask main loop to re-initialize.
            m_NeedReInit = true;
        }

        m_GlobalDebugConfig.effectId = FFX_API_EFFECT_ID_FRAMEGENERATION;
        ffx::ReturnCode retCode = ffx::Configure(m_FrameGenContext, m_GlobalDebugConfig);
        CauldronAssert(ASSERT_ERROR, retCode == ffx::ReturnCode::Ok, L"Couldn't configure global debug config on the ffxapi frame generation context: %d", (uint32_t)retCode);
    }
}

ResolutionInfo FSRRenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    ResolutionInfo info{};
    info.RenderWidth        = static_cast<uint32_t>((float)displayWidth / m_UpscaleRatio * m_LetterboxRatioX);
    info.RenderHeight       = static_cast<uint32_t>((float)displayHeight / m_UpscaleRatio * m_LetterboxRatioY);
    info.UpscaleWidth       = static_cast<uint32_t>((float)displayWidth * m_LetterboxRatioX);
    info.UpscaleHeight      = static_cast<uint32_t>((float)displayHeight * m_LetterboxRatioY);
    info.DisplayWidth       = displayWidth;
    info.DisplayHeight      = displayHeight;
    info.PostLetterboxWidth  = static_cast<uint32_t>((float)displayWidth * m_LetterboxRatioX * m_GenerationRectRatioX);
    info.PostLetterboxHeight = static_cast<uint32_t>((float)displayHeight * m_LetterboxRatioY * m_GenerationRectRatioY);
    return info;
}

void FSRRenderModule::OnPreFrame()
{
    if (NeedsReInit())
    {
        GetDevice()->FlushAllCommandQueues();

#if defined(_VK)
        if (m_FrameInterpolationAvailable && m_SwapChainContext)
        {
            ffx::DispatchDescFrameGenerationSwapChainWaitForPresentsVK waitForPresentsDesc;
            ffx::ReturnCode errorCode = ffx::Dispatch(m_SwapChainContext, waitForPresentsDesc);
            CauldronAssert(ASSERT_CRITICAL, errorCode == ffx::ReturnCode::Ok, L"OnPreFrame FrameGenerationSwapChain WaitForPresents failed: %d", (uint32_t)errorCode);
        }
#endif

        // Need to recreate the FSR context
        EnableModule(false);
        s_uiRenderMode = s_uiRenderModeNextFrame;
        EnableFrameInterpolationSwapchain(m_EnableFrameInterpolationSwapchain);
        EnableModule(true);

        ClearReInit();
    }
}

void FSRRenderModule::OnResize(const ResolutionInfo&)
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
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    CameraComponent* pCamera = GetScene()->GetCurrentCamera();

    GPUScopedProfileCapture sampleMarker(pCmdList, L"Cauldron Internals");
    if (m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState() != ResourceState::NonPixelShaderResource)
    {
        Barrier barrier = Barrier::Transition(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(),
            m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState(),
            ResourceState::NonPixelShaderResource);
        ResourceBarrier(pCmdList, 1, &barrier);
    }

    if (m_pUiTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState() != ResourceState::ShaderResource)
    {
        std::vector<Barrier> barriers = { Barrier::Transition(m_pUiTexture[m_curUiTextureIndex]->GetResource(),
                                                             m_pUiTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState(),
                                                             ResourceState::ShaderResource) };
        ResourceBarrier(pCmdList, (uint32_t)barriers.size(), barriers.data());
    }

    GPUResource* pSwapchainBackbuffer = cauldron::GetSwapChain()->GetBackBufferRT()->GetCurrentResource();
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
        GPUScopedProfileCapture sampleCopyMarker(pCmdList, L"CopyToTemp");

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
        GPUScopedProfileCapture sampleMarkerUpscaler(pCmdList, L"FFX API FSR Upscaler");
        // FFXAPI
        // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
        ffx::DispatchDescUpscale dispatchUpscale{};
#if defined(_VK)
        dispatchUpscale.commandList = pCmdList->GetImpl()->VKCmdBuffer();
#else
        dispatchUpscale.commandList = pCmdList->GetImpl()->DX12CmdList();
#endif
        dispatchUpscale.color = SDKWrapper::ffxGetResourceApi(m_pTempTexture->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.depth = SDKWrapper::ffxGetResourceApi(m_pDepthTarget->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.motionVectors = SDKWrapper::ffxGetResourceApi(m_pMotionVectors->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.exposure = SDKWrapper::ffxGetResourceApi(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchUpscale.output = SDKWrapper::ffxGetResourceApi(m_pColorTarget->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        if (m_MaskMode != FSRMaskMode::Disabled)
        {
            dispatchUpscale.reactive = SDKWrapper::ffxGetResourceApi(m_pReactiveMask->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }
        else
        {
            dispatchUpscale.reactive = SDKWrapper::ffxGetResourceApi(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }

        if (m_UseMask)
        {
            dispatchUpscale.transparencyAndComposition =
                SDKWrapper::ffxGetResourceApi(m_pCompositionMask->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }
        else
        {
            dispatchUpscale.transparencyAndComposition = SDKWrapper::ffxGetResourceApi(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }

        // Jitter is calculated earlier in the frame using a callback from the camera update
        dispatchUpscale.jitterOffset.x = -m_JitterX;
        dispatchUpscale.jitterOffset.y = -m_JitterY;
        dispatchUpscale.motionVectorScale.x = resInfo.fRenderWidth();
        dispatchUpscale.motionVectorScale.y = resInfo.fRenderHeight();
        dispatchUpscale.reset = m_ResetUpscale || GetScene()->GetCurrentCamera()->WasCameraReset();
        dispatchUpscale.enableSharpening = m_RCASSharpen;
        dispatchUpscale.sharpness = m_Sharpness;

        // Cauldron keeps time in seconds, but FSR expects milliseconds
        dispatchUpscale.frameTimeDelta = static_cast<float>(deltaTime * 1000.f);

        dispatchUpscale.preExposure = 1.0f; // Cauldron never applies exposure to input image going into FSR upscaler, only in tonemapper post upscale.
        dispatchUpscale.renderSize.width = resInfo.RenderWidth;
        dispatchUpscale.renderSize.height = resInfo.RenderHeight;
        dispatchUpscale.upscaleSize.width = resInfo.UpscaleWidth;
        dispatchUpscale.upscaleSize.height = resInfo.UpscaleHeight;

        // Setup camera params as required
        dispatchUpscale.cameraFovAngleVertical = pCamera->GetFovY();
        dispatchUpscale.cameraFar  = pCamera->GetFarPlane();
        dispatchUpscale.cameraNear = pCamera->GetNearPlane();

        dispatchUpscale.flags = 0;
        dispatchUpscale.flags |= m_DrawUpscalerDebugView ? FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW : 0;
        dispatchUpscale.flags |= m_colorSpace == FSRColorSpace::sRGBColorSpace ? FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB : 0;
        dispatchUpscale.flags |= m_colorSpace == FSRColorSpace::PQColorSpace ? FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_PQ : 0;

        ffx::ReturnCode retCode = ffx::Dispatch(m_UpscalingContext, dispatchUpscale);
        CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Dispatching FSR upscaling failed: %d", (uint32_t)retCode);
    }

#pragma FFX_PRAGMA_WARNING_PUSH
#pragma FFX_PRAGMA_WARNING_DISABLE_DEPRECATIONS
        ffx::DispatchDescFrameGenerationPrepareV2 dispatchFgPrep{};
        ffx::ReturnCode retCode;

#if defined(_VK)
        dispatchFgPrep.commandList = pCmdList->GetImpl()->VKCmdBuffer();
#elif defined(_DX12)
        dispatchFgPrep.commandList = pCmdList->GetImpl()->DX12CmdList();
#endif
        dispatchFgPrep.depth         = SDKWrapper::ffxGetResourceApi(m_pDepthTarget->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchFgPrep.motionVectors = SDKWrapper::ffxGetResourceApi(m_pMotionVectors->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchFgPrep.flags         = 0;

        dispatchFgPrep.jitterOffset.x      = -m_JitterX;
        dispatchFgPrep.jitterOffset.y      = -m_JitterY;
        dispatchFgPrep.motionVectorScale.x  = resInfo.fRenderWidth();
        dispatchFgPrep.motionVectorScale.y = resInfo.fRenderHeight();

        // Cauldron keeps time in seconds, but FSR expects milliseconds
        dispatchFgPrep.frameTimeDelta = static_cast<float>(deltaTime * 1000.f);

        dispatchFgPrep.renderSize.width       = resInfo.RenderWidth;
        dispatchFgPrep.renderSize.height      = resInfo.RenderHeight;
        dispatchFgPrep.cameraFovAngleVertical = pCamera->GetFovY();

        dispatchFgPrep.cameraFar  = pCamera->GetFarPlane();
        dispatchFgPrep.cameraNear = pCamera->GetNearPlane();
        dispatchFgPrep.viewSpaceToMetersFactor = 0.f;
        dispatchFgPrep.frameID                 = m_FrameID;
        dispatchFgPrep.reset                   = m_ResetFrameInterpolation;

        // Update frame generation config
        FfxApiResource hudLessResource =
            SDKWrapper::ffxGetResourceApi(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), FFX_API_RESOURCE_STATE_COMPUTE_READ);

        m_FrameGenerationConfig.frameGenerationEnabled = m_FrameInterpolation;
        m_FrameGenerationConfig.flags = 0u;
        m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugTearLines ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
        m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugResetIndicators ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS : 0;
        m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugPacingLines ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES : 0;
        m_FrameGenerationConfig.flags |= m_DrawFrameGenerationDebugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
        if (!m_SwapChainContext)
            m_FrameGenerationConfig.flags |= FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY;
        dispatchFgPrep.flags = m_FrameGenerationConfig.flags;  // TODO: maybe these should be distinct flags?
        m_FrameGenerationConfig.HUDLessColor        = (s_uiRenderMode == UICompositionMode::PreUiBackbuffer) ? hudLessResource : FfxApiResource({});
        m_FrameGenerationConfig.allowAsyncWorkloads = m_AllowAsyncCompute && m_EnableAsyncCompute;

        // --- GenRect animation timer: smoothly animate generationRect to/from upscalerOutputRect ---
        float effectiveGenRectRatioX = m_GenerationRectRatioX;
        float effectiveGenRectRatioY = m_GenerationRectRatioY;
        if (m_AnimateGenerationRect)
        {
            constexpr float kAnimationSpeed = 2.0f; // seconds for one full ping-pong cycle
            const float dt = static_cast<float>(deltaTime);
            const float step = dt / kAnimationSpeed;

            if (m_GenRectAnimationForward)
            {
                m_GenRectAnimationT += step;
                if (m_GenRectAnimationT >= 1.0f)
                {
                    m_GenRectAnimationT = 1.0f;
                    m_GenRectAnimationForward = false;
                }
            }
            else
            {
                m_GenRectAnimationT -= step;
                if (m_GenRectAnimationT <= 0.0f)
                {
                    m_GenRectAnimationT = 0.0f;
                    m_GenRectAnimationForward = true;
                }
            }

            // At t=0 generationRect matches the upscalerOutputRect (effective ratio = 1.0).
            // At t=1 generationRect is fully shrunk to the slider-set target ratios.
            effectiveGenRectRatioX = 1.0f - m_GenRectAnimationT * (1.0f - m_GenerationRectRatioX);
            effectiveGenRectRatioY = 1.0f - m_GenRectAnimationT * (1.0f - m_GenerationRectRatioY);
        }
        
        // assume symmetric letterbox
        m_FrameGenerationConfig.generationRect.left = static_cast<int32_t>((resInfo.DisplayWidth - resInfo.DisplayWidth * m_LetterboxRatioX * effectiveGenRectRatioX) / 2);
        m_FrameGenerationConfig.generationRect.top = static_cast<int32_t>((resInfo.DisplayHeight - resInfo.DisplayHeight * m_LetterboxRatioY * effectiveGenRectRatioY) / 2);
        m_FrameGenerationConfig.generationRect.width = static_cast<int32_t>(resInfo.DisplayWidth * m_LetterboxRatioX * effectiveGenRectRatioX);
        m_FrameGenerationConfig.generationRect.height = static_cast<int32_t>(resInfo.DisplayHeight * m_LetterboxRatioY * effectiveGenRectRatioY);

        // Update UIRenderModule with letterbox info for black bar drawing
        if (!m_pUIRenderModule)
            m_pUIRenderModule = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        if (m_pUIRenderModule)
            m_pUIRenderModule->SetLetterbox(m_AppBlackOutsideGenerationRect,
                m_FrameGenerationConfig.generationRect.left,
                m_FrameGenerationConfig.generationRect.top,
                m_FrameGenerationConfig.generationRect.width,
                m_FrameGenerationConfig.generationRect.height);
        
        if (m_UseCallback)
        {
            m_FrameGenerationConfig.frameGenerationCallback = [](ffxDispatchDescFrameGeneration* params, void* pUserCtx) -> ffxReturnCode_t
            {
                return ffxDispatch(reinterpret_cast<ffxContext*>(pUserCtx), &params->header);
            };
            m_FrameGenerationConfig.frameGenerationCallbackUserContext = &m_FrameGenContext;
        }
        else
        {
            m_FrameGenerationConfig.frameGenerationCallback            = nullptr;
            m_FrameGenerationConfig.frameGenerationCallbackUserContext = nullptr;
        }
        m_FrameGenerationConfig.onlyPresentGenerated = m_PresentInterpolatedOnly;
        m_FrameGenerationConfig.frameID              = m_FrameID;

#if defined(_VK)
        void* ffxSwapChain = GetSwapChain()->GetImpl()->VKSwapChain();
        m_FrameGenerationConfig.swapChain = ffxSwapChain;
#elif defined(_DX12)
        void* ffxSwapChain;
        ffxSwapChain = GetSwapChain()->GetImpl()->DX12SwapChain();
        m_FrameGenerationConfig.swapChain = ffxSwapChain;
#endif

        {
            if (m_UseDistortionField)
            {
                ffx::ConfigureDescFrameGenerationRegisterDistortionFieldResource dfConfig{};
                dfConfig.distortionField =
                    SDKWrapper::ffxGetResourceApi(m_pDistortionField[m_curUiTextureIndex]->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
                retCode = ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig, dfConfig);
            }
            else
            {
                retCode = ffx::Configure(m_FrameGenContext, m_FrameGenerationConfig);
            }
        }
        CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Configuring FSR FG failed: %d", (uint32_t)retCode);

        //only dispatch FG Prepare if frame interpolation is enabled
        if (m_FrameInterpolation)
        {
            GPUScopedProfileCapture sampleMarkerFrameGen(pCmdList, L"FFX API FSR Frame Generation Prepare");
            // Acquire lock before dispatch FG Prepare
            cauldron::FGContextScopeLock lock(true);
            Vec3 vectorInfo = pCamera->GetCameraPos();
            memcpy(dispatchFgPrep.cameraPosition, &vectorInfo, 3 * sizeof(float));
            vectorInfo = pCamera->GetCameraUp();
            memcpy(dispatchFgPrep.cameraUp, &vectorInfo, 3 * sizeof(float));
            vectorInfo = pCamera->GetCameraRight();
            memcpy(dispatchFgPrep.cameraRight, &vectorInfo, 3 * sizeof(float));
            vectorInfo = -pCamera->GetDirection().getXYZ();
            memcpy(dispatchFgPrep.cameraForward, &vectorInfo, 3 * sizeof(float));

            retCode = ffx::Dispatch(m_FrameGenContext, dispatchFgPrep);
            CauldronAssert(ASSERT_WARNING, !!retCode, L"Dispatching FSR FGPrepareV2 failed: %d", (uint32_t)retCode);
        }

        if (m_SwapChainContext)
        {
            FfxApiResource uiColor =
                (s_uiRenderMode == UICompositionMode::UiTexture) ? SDKWrapper::ffxGetResourceApi(m_pUiTexture[m_curUiTextureIndex]->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ)
                : FfxApiResource({});

#if defined(_DX12)
            ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 uiConfig{};
#elif defined(_VK)
            ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceVK uiConfig{};
#endif
            uiConfig.uiResource = uiColor;
            uiConfig.flags = m_DoublebufferInSwapchain ? FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING : 0;
            ffx::Configure(m_SwapChainContext, uiConfig);
        }
    

    // Dispatch frame generation, if not using the callback
    if (m_FrameInterpolation && !m_UseCallback)
    {
        ffx::DispatchDescFrameGeneration dispatchFg{};
        pSwapchainBackbuffer = cauldron::GetSwapChain()->GetBackBufferRT()->GetCurrentResource();
            
        dispatchFg.numGeneratedFrames = 1;

        // assume symmetric letterbox
        dispatchFg.generationRect.left = static_cast<int32_t>((resInfo.DisplayWidth - resInfo.DisplayWidth * m_LetterboxRatioX * effectiveGenRectRatioX) / 2);
        dispatchFg.generationRect.top = static_cast<int32_t>((resInfo.DisplayHeight - resInfo.DisplayHeight * m_LetterboxRatioY * effectiveGenRectRatioY) / 2);
        dispatchFg.generationRect.width = static_cast<int32_t>(resInfo.DisplayWidth * m_LetterboxRatioX * effectiveGenRectRatioX);
        dispatchFg.generationRect.height = static_cast<int32_t>(resInfo.DisplayHeight * m_LetterboxRatioY * effectiveGenRectRatioY);
        if (m_SwapChainContext)
        {
            backbuffer = SDKWrapper::ffxGetResourceApi(pSwapchainBackbuffer, FFX_API_RESOURCE_STATE_PRESENT);
            dispatchFg.presentColor = backbuffer;

#if defined(_DX12)
            ffx::QueryDescFrameGenerationSwapChainInterpolationCommandListDX12 queryCmdList{};
#elif defined(_VK)
            ffx::QueryDescFrameGenerationSwapChainInterpolationCommandListVK queryCmdList{};
#endif
            queryCmdList.pOutCommandList = &dispatchFg.commandList;
            retCode = ffx::Query(m_SwapChainContext, queryCmdList);
            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                L"ffx::Query(SwapChainContext,InterpolationCommandList) returned %d", (uint32_t)retCode);

#if defined(_DX12)
            ffx::QueryDescFrameGenerationSwapChainInterpolationTextureDX12 queryFiTexture{};
#elif defined(_VK)
            ffx::QueryDescFrameGenerationSwapChainInterpolationTextureVK queryFiTexture{};
#endif
            queryFiTexture.pOutTexture = &dispatchFg.outputs[0];
            retCode = ffx::Query(m_SwapChainContext, queryFiTexture);
            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok,
                L"ffx::Query(SwapChainContext,InterpolationTexture) returned %d", (uint32_t)retCode);
        }
        else
        {
            backbuffer = SDKWrapper::ffxGetResourceApi(pSwapchainBackbuffer, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchFg.presentColor = backbuffer;

#if defined(_VK)
            dispatchFg.commandList = pCmdList->GetImpl()->VKCmdBuffer();
#elif defined(_DX12)
            dispatchFg.commandList = pCmdList->GetImpl()->DX12CmdList();
#endif

            dispatchFg.outputs[0] =
                SDKWrapper::ffxGetResourceApi(m_pInterpolationOutput->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }
            
        dispatchFg.frameID = m_FrameID;
        dispatchFg.reset = m_ResetFrameInterpolation;

        const cauldron::SwapChain* swapChain = cauldron::GetSwapChain();
        dispatchFg.backbufferTransferFunction = SDKWrapper::GetFfxApiBackbufferTransferFunction(swapChain->GetSwapChainDisplayMode());

        const HDRMetadata& hdrMetaData = swapChain->GetHDRMetaData();
        dispatchFg.minMaxLuminance[0] = hdrMetaData.MinLuminance;
        dispatchFg.minMaxLuminance[1] = hdrMetaData.MaxLuminance;
        {
            // Acquire lock if app dispatch frame generation directly
            cauldron::FGContextScopeLock lock(true);
            retCode = ffx::Dispatch(m_FrameGenContext, dispatchFg);
            CauldronAssert(ASSERT_CRITICAL, !!retCode, L"Dispatching Frame Generation failed: %d", (uint32_t)retCode);
        }
    }

    m_FrameID += uint64_t(1 + m_SimulatePresentSkip);
    m_SimulatePresentSkip = false;

    m_ResetUpscale = false;
    m_ResetFrameInterpolation = false;

    m_PreviousJitterX = m_JitterX;
    m_PreviousJitterY = m_JitterY;

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
}

void FSRRenderModule::PreTransCallback(double, CommandList* pCmdList)
{
    GPUScopedProfileCapture sampleMarker(pCmdList, L"Pre-Trans (FSR)");

    std::vector<Barrier> barriers;
    barriers.push_back(Barrier::Transition(m_pReactiveMask->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::RenderTargetResource));
    barriers.push_back(Barrier::Transition(m_pCompositionMask->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::RenderTargetResource));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // We need to clear the reactive and composition masks before any translucencies are rendered into them
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ResourceViewInfo rasterView0   = m_RasterViews[0]->GetResourceView();
    ResourceViewInfo rasterView1   = m_RasterViews[1]->GetResourceView();
    ClearRenderTarget(pCmdList, &rasterView0, clearColor);
    ClearRenderTarget(pCmdList, &rasterView1, clearColor);

    barriers.clear();
    barriers.push_back(Barrier::Transition(m_pReactiveMask->GetResource(), ResourceState::RenderTargetResource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
    barriers.push_back(Barrier::Transition(m_pCompositionMask->GetResource(), ResourceState::RenderTargetResource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // update index for UI doublebuffering
    UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
    m_curUiTextureIndex = (m_curUiTextureIndex + 1) & 1;
    uimod->SetUiSurfaceIndex(m_curUiTextureIndex);

    //update index for distortion texture doublebuffering
    m_pToneMappingRenderModule->SetDoubleBufferedTextureIndex(m_curUiTextureIndex);

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
}

void FSRRenderModule::PostTransCallback(double, CommandList* pCmdList)
{
    if ((m_MaskMode != FSRMaskMode::Auto) || (m_UpscaleMethod!= 1))
        return;

    GPUScopedProfileCapture sampleMarker(pCmdList, L"Gen Reactive Mask (FSR API)");

    ffx::DispatchDescUpscaleGenerateReactiveMask dispatchDesc{};
#if defined(_VK)
    dispatchDesc.commandList   = pCmdList->GetImpl()->VKCmdBuffer();
#elif defined(_DX12)
    dispatchDesc.commandList   = pCmdList->GetImpl()->DX12CmdList();
#endif
    dispatchDesc.colorOpaqueOnly = SDKWrapper::ffxGetResourceApi(m_pOpaqueTexture->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchDesc.colorPreUpscale = SDKWrapper::ffxGetResourceApi(m_pColorTarget->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchDesc.outReactive = SDKWrapper::ffxGetResourceApi(m_pReactiveMask->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    dispatchDesc.renderSize.width = resInfo.RenderWidth;
    dispatchDesc.renderSize.height = resInfo.RenderHeight;

    // The following are all hard-coded in the original FSR2 sample. Should these be exposed?
    dispatchDesc.scale = 1.f;
    dispatchDesc.cutoffThreshold = 0.2f;
    dispatchDesc.binaryValue = 0.9f;
    dispatchDesc.flags = FFX_UPSCALE_AUTOREACTIVEFLAGS_APPLY_TONEMAP |
                         FFX_UPSCALE_AUTOREACTIVEFLAGS_APPLY_THRESHOLD |
                         FFX_UPSCALE_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX;

    ffx::ReturnCode retCode = ffx::Dispatch(m_UpscalingContext, dispatchDesc);
    CauldronAssert(ASSERT_ERROR, retCode == ffx::ReturnCode::Ok, L"ffxDispatch(FSR_GENERATEREACTIVEMASK) failed with %d", (uint32_t)retCode);

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);
}

// Copy of ffxRestoreApplicationSwapChain from backend_interface, which is not built for this sample.
void RestoreApplicationSwapChain(bool recreateSwapchain)
{
#if defined(_VK)
    cauldron::SwapChainInternal* pSwapchain = cauldron::GetSwapChain()->GetImpl();
    if (pSwapchain->IsFrameInterpolation())
    {
        VkSwapchainKHR fiSwapchain = pSwapchain->VKSwapChain();
        pSwapchain->SetVKSwapChain(VK_NULL_HANDLE);
        cauldron::GetDevice()->GetImpl()->DestroySwapchainKHR(fiSwapchain, nullptr, true);
    }

    const VkSwapchainCreateInfoKHR* pCreateInfo = pSwapchain->GetCreateInfo();
    if (recreateSwapchain)
    {
        VkSwapchainKHR swapchain;
        VkResult res = cauldron::GetDevice()->GetImpl()->CreateSwapchainKHR(pCreateInfo, nullptr, &swapchain);
        if (res == VK_SUCCESS)
        {
            // Swapchain creation can fail when this function is called when closing the application. In that case, just exit silently
            pSwapchain->SetVKSwapChain(swapchain);
        }
    }
#elif defined(_DX12)
    // Acquire lock before destroying proxy swapchain
    cauldron::FGContextScopeLock lock(true);
    cauldron::SwapChain* pSwapchain  = cauldron::GetSwapChain();
    IDXGISwapChain4*     pSwapChain4 = pSwapchain->GetImpl()->DX12SwapChain();
    pSwapChain4->AddRef(); // Hold strong ref while clearing engine pointer.

    IDXGIFactory7*      factory   = nullptr;
    ID3D12CommandQueue* pCmdQueue = cauldron::GetDevice()->GetImpl()->DX12CmdQueue(cauldron::CommandQueue::Graphics);

    // Setup a new swapchain for HWND and set it to cauldron
    IDXGISwapChain1* pSwapChain1 = nullptr;
    if (SUCCEEDED(pSwapChain4->GetParent(IID_PPV_ARGS(&factory))))
    {
        // Engine no longer owns COM pointer; safe to recreate after Release().
        cauldron::GetSwapChain()->GetImpl()->SetDXGISwapChain(nullptr);

        // Capture data required to recreate a normal DXGI swap chain BEFORE releasing COM object.
        HWND windowHandle = pSwapchain->GetImpl()->DX12SwapChainDesc().OutputWindow;
        DXGI_SWAP_CHAIN_DESC1 desc1 = pSwapchain->GetImpl()->DX12SwapChainDesc1();
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC  fsDesc = pSwapchain->GetImpl()->DX12SwapChainFullScreenDesc();

        // Release final reference triggers proxy swap chain destructor when refCount reaches 0.
        ULONG refCount = pSwapChain4->Release();
        CauldronAssert(ASSERT_ERROR, refCount == 0, L"SwapChain should be destroyed here. But refcount is %u (expected 0).", refCount);

        // check if window is still valid or if app is shutting down bc window was closed
        if (recreateSwapchain && IsWindow(windowHandle))
        {
            // Recreate normal DXGI swap chain
            if (SUCCEEDED(factory->CreateSwapChainForHwnd(pCmdQueue, windowHandle, &desc1, &fsDesc, nullptr, &pSwapChain1)))
            {
                if (SUCCEEDED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain4))))
                {
                    cauldron::GetSwapChain()->GetImpl()->SetDXGISwapChain(pSwapChain4);
                    pSwapChain4->Release();
                }
                pSwapChain1->Release();
            }
            factory->MakeWindowAssociation(cauldron::GetFramework()->GetImpl()->GetHWND(), DXGI_MWA_NO_WINDOW_CHANGES);
        }

        factory->Release();
    }
    return;

#endif
}

void FSRRenderModule::EnableFrameInterpolationSwapchain(bool enabled)
{

#if defined(_VK)
    EnableFrameInterpolationSwapchainVK(enabled);
#endif

#if defined(_DX12)
    EnableFrameInterpolationSwapchainDX12(enabled);
#endif

}

#if defined(_VK)
void FSRRenderModule::EnableFrameInterpolationSwapchainVK(bool enabled)
{
    // only create the proxy swapchain if not already created and enabled
    if (enabled && (m_SwapChainContext == nullptr
                    || (m_overrideVersion && m_scNeedReInit)))
    {
        // Recreate swapchain after version override
        if (m_SwapChainContext && m_overrideVersion && m_scNeedReInit)
        {
            ffx::DestroyContext(m_SwapChainContext);
            m_SwapChainContext = nullptr;
            RestoreApplicationSwapChain(true);
            cauldron::GetDevice()->GetImpl()->SetSwapchainContext(nullptr, nullptr, nullptr);
            CauldronAssert(ASSERT_CRITICAL, cauldron::GetSwapChain()->GetImpl()->VKSwapChain() != VK_NULL_HANDLE,
                L"Could not restore the Vulkan swapchain before changing frame generation provider.");
        }

        m_scNeedReInit = false;

        const cauldron::FIQueue* pAsyncComputeQueue = cauldron::GetDevice()->GetImpl()->GetFIAsyncComputeQueue();
        const cauldron::FIQueue* pPresentQueue = cauldron::GetDevice()->GetImpl()->GetFIPresentQueue();
        const cauldron::FIQueue* pImageAcquireQueue = cauldron::GetDevice()->GetImpl()->GetFIImageAcquireQueue();
    
        // Create frameinterpolation swapchain
        cauldron::SwapChain* pSwapchain = cauldron::GetFramework()->GetSwapChain();
        VkSwapchainKHR       currentSwapchain = pSwapchain->GetImpl()->VKSwapChain();

        ffx::CreateContextDescFrameGenerationSwapChainVK createSwapChainDesc{};
        createSwapChainDesc.physicalDevice = cauldron::GetDevice()->GetImpl()->VKPhysicalDevice();
        createSwapChainDesc.device = cauldron::GetDevice()->GetImpl()->VKDevice();

        createSwapChainDesc.swapchain = &currentSwapchain;
        createSwapChainDesc.createInfo = *cauldron::GetSwapChain()->GetImpl()->GetCreateInfo();
        createSwapChainDesc.allocator = nullptr;
        createSwapChainDesc.gameQueue.queue = cauldron::GetDevice()->GetImpl()->VKCmdQueue(cauldron::CommandQueue::Graphics);
        createSwapChainDesc.gameQueue.familyIndex = cauldron::GetDevice()->GetImpl()->VKCmdQueueFamily(cauldron::CommandQueue::Graphics);
        createSwapChainDesc.gameQueue.submitFunc = nullptr;  // this queue is only used in vkQueuePresentKHR, hence doesn't need a callback

        createSwapChainDesc.asyncComputeQueue.queue = pAsyncComputeQueue->queue;
        createSwapChainDesc.asyncComputeQueue.familyIndex = pAsyncComputeQueue->family;
        createSwapChainDesc.asyncComputeQueue.submitFunc = nullptr;

        createSwapChainDesc.presentQueue.queue = pPresentQueue->queue;
        createSwapChainDesc.presentQueue.familyIndex = pPresentQueue->family;
        createSwapChainDesc.presentQueue.submitFunc = nullptr;

        createSwapChainDesc.imageAcquireQueue.queue = pImageAcquireQueue->queue;
        createSwapChainDesc.imageAcquireQueue.familyIndex = pImageAcquireQueue->family;
        createSwapChainDesc.imageAcquireQueue.submitFunc = nullptr;

        // make sure swapchain is not holding a ref to real swapchain
        cauldron::GetSwapChain()->GetImpl()->SetVKSwapChain(VK_NULL_HANDLE);

        // Deal with this when it's time to
        auto convertQueueInfo = [](VkQueueInfoFFXAPI queueInfo) {
            VkQueueInfoFFX info;
            info.queue = queueInfo.queue;
            info.familyIndex = queueInfo.familyIndex;
            info.submitFunc = queueInfo.submitFunc;
            return info;
            };

        VkFrameInterpolationInfoFFX frameInterpolationInfo = {};
        frameInterpolationInfo.device = createSwapChainDesc.device;
        frameInterpolationInfo.physicalDevice = createSwapChainDesc.physicalDevice;
        frameInterpolationInfo.pAllocator = createSwapChainDesc.allocator;
        frameInterpolationInfo.gameQueue = convertQueueInfo(createSwapChainDesc.gameQueue);
        frameInterpolationInfo.asyncComputeQueue = convertQueueInfo(createSwapChainDesc.asyncComputeQueue);
        frameInterpolationInfo.presentQueue = convertQueueInfo(createSwapChainDesc.presentQueue);
        frameInterpolationInfo.imageAcquireQueue = convertQueueInfo(createSwapChainDesc.imageAcquireQueue);

        ffx::CreateContextDescFrameGenerationSwapChainVersionVK headerVersion{};
        headerVersion.version = FFX_FRAMEGENERATION_SWAPCHAIN_VK_VERSION;
        ffx::ReturnCode retCode;
        {
            retCode = ffx::CreateContext(m_SwapChainContext, nullptr, createSwapChainDesc, headerVersion);
        }
        CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't create the frame interpolation swapchain context: %d", (uint32_t)retCode);

        ffx::QueryDescSwapchainReplacementFunctionsVK replacementFunctions{};
        retCode = ffx::Query(m_SwapChainContext, replacementFunctions);
        CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't query the frame interpolation swapchain replacement functions: %d", (uint32_t)retCode);
        
        cauldron::GetDevice()->GetImpl()->SetSwapchainContext(m_SwapChainContext, &frameInterpolationInfo, &replacementFunctions);

        // Set frameinterpolation swapchain to engine
        cauldron::GetSwapChain()->GetImpl()->SetVKSwapChain(currentSwapchain, true);

        // we need to re initialize HDR info since swapchain was re created
        cauldron::GetSwapChain()->SetHDRMetadataAndColorspace();
    }
    else if (!enabled && m_SwapChainContext != nullptr)
    {
        ffx::DestroyContext(m_SwapChainContext);
        m_SwapChainContext = nullptr;
        RestoreApplicationSwapChain(true);
        cauldron::GetDevice()->GetImpl()->SetSwapchainContext(nullptr, nullptr, nullptr);
        CauldronAssert(ASSERT_CRITICAL, cauldron::GetSwapChain()->GetImpl()->VKSwapChain() != VK_NULL_HANDLE,
            L"Could not restore the Vulkan swapchain after disabling frame generation.");
        cauldron::GetFramework()->EnableFrameGenerationSwapchain(false);
    }
}
#endif

#if defined(_DX12)
void FSRRenderModule::EnableFrameInterpolationSwapchainDX12(bool enabled)
{
    // only create the proxy swapchain if not already created and enabled
    if (enabled && (m_SwapChainContext == nullptr
                    || (m_overrideVersion && m_scNeedReInit)))
    {
        // Recreate swapchain after version override
        if (m_SwapChainContext && m_overrideVersion && m_scNeedReInit)
        {
            // This doesn't call proxy swapchain destructor. There is still a swapchain ref held by cauldron
            ffx::DestroyContext(m_SwapChainContext);
            m_SwapChainContext = nullptr;
            if (latencyWaitableObj)
            {
                CloseHandle(latencyWaitableObj);
                latencyWaitableObj = 0;
            }
            RestoreApplicationSwapChain(true);
        }

        m_scNeedReInit = false;

        ffx::ReturnCode retCode;
        IDXGISwapChain4* dxgiSwapchain = GetSwapChain()->GetImpl()->DX12SwapChain();
        dxgiSwapchain->AddRef();
        GetSwapChain()->GetImpl()->SetDXGISwapChain(nullptr);

        ffx::CreateContextDescFrameGenerationSwapChainForHwndDX12 createSwapChainDesc{};
        dxgiSwapchain->GetHwnd(&createSwapChainDesc.hwnd);
        DXGI_SWAP_CHAIN_DESC1 desc1;
        dxgiSwapchain->GetDesc1(&desc1);
        createSwapChainDesc.desc = &desc1;
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
        dxgiSwapchain->GetFullscreenDesc(&fullscreenDesc);
        createSwapChainDesc.fullscreenDesc = &fullscreenDesc;
        dxgiSwapchain->GetParent(IID_PPV_ARGS(&createSwapChainDesc.dxgiFactory));
        createSwapChainDesc.gameQueue = GetDevice()->GetImpl()->DX12CmdQueue(cauldron::CommandQueue::Graphics);

        if (getLatencyWaitableObject)
        {
            createSwapChainDesc.desc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        }

        dxgiSwapchain->Release();
        dxgiSwapchain = nullptr;
        createSwapChainDesc.swapchain = &dxgiSwapchain;

        ffx::CreateContextDescFrameGenerationSwapChainVersionDX12 headerVersion = {};
        headerVersion.version = FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;

        {
            // TODO: Get memory usage for SwapChainContext
            retCode = ffx::CreateContext(m_SwapChainContext, nullptr, createSwapChainDesc, headerVersion);
        }
        CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't create the ffxapi fg swapchain (dx12): %d", (uint32_t)retCode);
    
        createSwapChainDesc.dxgiFactory->Release();

        cauldron::GetSwapChain()->GetImpl()->SetDXGISwapChain(dxgiSwapchain);
        
        if (getLatencyWaitableObject)
        {
            latencyWaitableObj = dxgiSwapchain->GetFrameLatencyWaitableObject();
        }


        // In case the app is handling Alt-Enter manually we need to update the window association after creating a different swapchain
        IDXGIFactory7* factory = nullptr;
        if (SUCCEEDED(dxgiSwapchain->GetParent(IID_PPV_ARGS(&factory))))
        {
            factory->MakeWindowAssociation(cauldron::GetFramework()->GetImpl()->GetHWND(), DXGI_MWA_NO_WINDOW_CHANGES);
            factory->Release();
        }

        dxgiSwapchain->Release();

        // Lets do the same for HDR as well as it will need to be re initialized since swapchain was re created
        cauldron::GetSwapChain()->SetHDRMetadataAndColorspace();

        cauldron::GetFramework()->EnableFrameGenerationSwapchain(true);

    }
	else if (!enabled && m_SwapChainContext != nullptr)
    {
        // This doesn't call proxy swapchain destructor. There is still a swapchain ref held by cauldron
        ffx::DestroyContext(m_SwapChainContext);
		m_SwapChainContext = nullptr;
        if (latencyWaitableObj)
        {
            CloseHandle(latencyWaitableObj);
            latencyWaitableObj = 0;
        }
        RestoreApplicationSwapChain(true);
        cauldron::GetFramework()->EnableFrameGenerationSwapchain(false);
	}
}
#endif

