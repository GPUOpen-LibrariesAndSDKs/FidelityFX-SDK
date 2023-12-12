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


#include "fsr3rendermodule.h"
#include "validation_remap.h"
#include "render/rendermodules/ui/uirendermodule.h"
#include "render/rasterview.h"
#include "render/dynamicresourcepool.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/rootsignature.h"
#include "render/swapchain.h"
#include "render/resourceviewallocator.h"
#include "core/scene.h"
#include "misc/assert.h"
#include "render/profiler.h"

#include "core/win/framework_win.h"

#ifdef FFX_API_DX12
#    include "render/dx12/swapchain_dx12.h"
#    include "render/dx12/device_dx12.h"
#    include "render/dx12/commandlist_dx12.h"
#    include "render/dx12/resourceview_dx12.h"
#endif

#include <functional>

using namespace std;
using namespace cauldron;

void FSR3RenderModule::Init(const json& initData)
{
    // Fetch needed resources
    m_pColorTarget = cauldron::GetFramework()->GetColorTargetForCallback(GetName());
    m_pDepthTarget = cauldron::GetFramework()->GetRenderTexture(L"DepthTarget");

    // Create render resolution opaque render target to use for auto-reactive mask generation
    TextureDesc desc = m_pColorTarget->GetDesc();
    const ResolutionInfo& resInfo = cauldron::GetFramework()->GetResolutionInfo();
    desc.Width = resInfo.RenderWidth;
    desc.Height = resInfo.RenderHeight;
    desc.Name = L"FSR3_OpaqueTexture";
    m_pOpaqueTexture = GetDynamicResourcePool()->CreateRenderTexture(&desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight)
        {
            desc.Width = renderingWidth;
            desc.Height = renderingHeight;
        });

    // Assumed resources, need to check they are there
    m_pMotionVectors = GetFramework()->GetRenderTexture(L"GBufferMotionVectorRT");
    m_pReactiveMask = GetFramework()->GetRenderTexture(L"ReactiveMask");
    m_pCompositionMask = GetFramework()->GetRenderTexture(L"TransCompMask");
   
    // Create raster views on the reactive mask and composition masks (for clearing and rendering)
    m_RasterViews.resize(2);
    m_RasterViews[0] = GetRasterViewAllocator()->RequestRasterView(m_pReactiveMask, ViewDimension::Texture2D);
    m_RasterViews[1] = GetRasterViewAllocator()->RequestRasterView(m_pCompositionMask, ViewDimension::Texture2D);

    // Set our render resolution function as that to use during resize to get render width/height from display width/height
    m_pUpdateFunc = [this](uint32_t displayWidth, uint32_t displayHeight) { return this->UpdateResolution(displayWidth, displayHeight); };

    //////////////////////////////////////////////////////////////////////////
    // Build UI

    // Build UI options, but don't register them yet. Registration/Deregistration will be controlled by enabling/disabling the render module
    m_UISection.SectionName = "Upscaling";  // We will piggy-back on existing upscaling section"

    // Setup scale preset options
    const char* preset[] = { "Native AA (1.0x)", "Quality (1.5x)", "Balanced (1.7x)", "Performance (2x)", "Ultra Performance (3x)", "Custom" };
    std::vector<std::string> presetComboOptions;
    for (int32_t i = 0; i < _countof(preset); ++i)
        presetComboOptions.push_back(preset[i]);
    std::function<void(void*)> presetCallback = [this](void* pParams) { this->UpdatePreset(static_cast<int32_t*>(pParams)); };
    m_UISection.AddCombo("Scale Preset", reinterpret_cast<int32_t*>(&m_ScalePreset), &presetComboOptions, presetCallback);

    // Setup mip bias
    std::function<void(void*)> mipBiasCallback = [this](void* pParams) { this->UpdateMipBias(static_cast<float*>(pParams)); };
    m_UISection.AddFloatSlider("Mip LOD Bias", &m_MipBias, -5.f, 0.f, mipBiasCallback);

    // Setup scale factor (disabled for all but custom)
    std::function<void(void*)> ratioCallback = [this](void* pParams) { this->UpdateUpscaleRatio(static_cast<float*>(pParams)); };
    m_UISection.AddFloatSlider("Custom Scale", &m_UpscaleRatio, 1.f, 3.f, ratioCallback, &m_UpscaleRatioEnabled);

    // Reactive mask
    const char* masks[] = { "Disabled", "Manual Reactive Mask Generation", "Autogen FSR3 Helper Function" };
    std::vector<std::string> maskComboOptions;
    for (int32_t i = 0; i < _countof(masks); ++i)
        maskComboOptions.push_back(masks[i]);
    m_UISection.AddCombo("Reactive Mask Mode", reinterpret_cast<int32_t*>(&m_MaskMode), &maskComboOptions);

    // Use mask
    m_UISection.AddCheckBox("Use Transparency and Composition Mask", &m_UseMask);

    // Sharpening
    std::function<void(void*)> sharpeningCallback = [this](void* pParams) { this->m_SharpnessEnabled = this->m_RCASSharpen; };
    m_UISection.AddCheckBox("RCAS Sharpening", &m_RCASSharpen, sharpeningCallback);
    m_UISection.AddFloatSlider("Sharpness", &m_Sharpness, 0.f, 1.f, nullptr, &m_RCASSharpen);

#if defined(FFX_API_DX12)
    // Frame interpolation
    m_UISection.AddCheckBox("Frame Interpolation", &m_FrameInterpolation, [this](void*) {
        this->m_OfUiEnabled = this->m_FrameInterpolation && this->s_enableSoftwareMotionEstimation;
        // Ask main loop to re-initialize.
        this->m_NeedReInit  = true;
    });

    m_UISection.AddCheckBox("Support Async Compute", &m_PendingEnableAsyncCompute, [this](void*) {
        // Ask main loop to re-initialize.
        this->m_NeedReInit = true;
    });

    m_UISection.AddCheckBox("Allow async compute", &m_AllowAsyncCompute, nullptr, &m_PendingEnableAsyncCompute);

    m_UISection.AddCheckBox("Use callback", &m_UseCallback, nullptr, &m_FrameInterpolation);
    m_UISection.AddCheckBox("Draw tear lines", &m_DrawDebugTearLines, nullptr, &m_FrameInterpolation);
    m_UISection.AddCheckBox("Draw debug view", &m_DrawDebugView, nullptr, &m_FrameInterpolation);

    std::vector<string>        uiRenderModeLabels   = {"No UI handling (not recommended)", "UiTexture", "UiCallback", "Pre-Ui Backbuffer"};
    std::function<void(void*)> uiRenderModeCallback = [this](void* pParams) {
        UIRenderModule* uimod = static_cast<UIRenderModule*>(cauldron::GetFramework()->GetRenderModule("UIRenderModule"));
        uimod->SetAsyncRender(s_uiRenderMode == 2);
        uimod->SetRenderToTexture(s_uiRenderMode == 1);
        uimod->SetCopyHudLessTexture(s_uiRenderMode == 3);

        this->m_NeedReInit = true;
    };
    m_UISection.AddCombo("UI Composition Mode", &s_uiRenderMode, &uiRenderModeLabels, uiRenderModeCallback, &m_FrameInterpolation);

#endif
    //////////////////////////////////////////////////////////////////////////
    // Register additional execution callbacks during the frame

    // Register a post-lighting callback to copy opaque texture
    ExecuteCallback callbackPreTrans = [this](double deltaTime, CommandList* pCmdList) {
        this->PreTransCallback(deltaTime, pCmdList);
    };
    ExecutionTuple callbackPreTransTuple = std::make_pair(L"FSR3RenderModule::PreTransCallback", std::make_pair(this, callbackPreTrans));
    GetFramework()->RegisterExecutionCallback(L"LightingRenderModule", false, callbackPreTransTuple);

    // Register a post-transparency callback to generate reactive mask
    ExecuteCallback callbackPostTrans = [this](double deltaTime, CommandList* pCmdList) {
        this->PostTransCallback(deltaTime, pCmdList);
    };
    ExecutionTuple callbackPostTransTuple = std::make_pair(L"FSR3RenderModule::PostTransCallback", std::make_pair(this, callbackPostTrans));
    GetFramework()->RegisterExecutionCallback(L"TranslucencyRenderModule", false, callbackPostTransTuple);

    m_curUiTextureIndex     = 0;
    // Get the proper UI color target
    switch (GetFramework()->GetSwapChain()->GetSwapChainDisplayMode())
    {
    case DisplayMode::DISPLAYMODE_LDR:
        m_pUiTexture[0] = GetFramework()->GetRenderTexture(L"UiTarget0_LDR8");
        m_pUiTexture[1] = GetFramework()->GetRenderTexture(L"UiTarget1_LDR8");
        break;
    case DisplayMode::DISPLAYMODE_HDR10_2084:
    case DisplayMode::DISPLAYMODE_FSHDR_2084:
        m_pUiTexture[0] = GetFramework()->GetRenderTexture(L"UiTarget0_HDR10");
        m_pUiTexture[1] = GetFramework()->GetRenderTexture(L"UiTarget1_HDR10");
        break;
    case DisplayMode::DISPLAYMODE_HDR10_SCRGB:
    case DisplayMode::DISPLAYMODE_FSHDR_SCRGB:
        m_pUiTexture[0] = GetFramework()->GetRenderTexture(L"UiTarget0_HDR16");
        m_pUiTexture[1] = GetFramework()->GetRenderTexture(L"UiTarget1_HDR16");
        break;
    }

    // Create FrameInterpolationSwapchain
    // Separate from FSR3 generation so it can be done when the engine creates the swapchain
    // should not be created and destroyed with FSR3, as it requires a switch to windowed mode
#ifdef FFX_API_CAULDRON
    // FSR3 not supported on cauldron backend, so proxy swapchain is not required
#elif defined(FFX_API_DX12)
    //if (false)
    {
        // TODO: should check if FI swapchain already exists!
        
        // take control over the swapchain: first get the swapchain and set to NULL in engine
        IDXGISwapChain4* dxgiSwapchain = cauldron::GetFramework()->GetSwapChain()->GetImpl()->DX12SwapChain();
        //m_InitializationParameters.fiDescription.swapchain = ffxGetSwapchainDX12(dxgiSwapchain);
        dxgiSwapchain->AddRef();

        dxgiSwapchain->GetDesc(&gameSwapChainDesc);
        dxgiSwapchain->GetDesc1(&gameSwapChainDesc1);
        dxgiSwapchain->GetFullscreenDesc(&gameFullscreenDesc);

        // Create frameinterpolation swapchain
        cauldron::SwapChain* pSwapchain   = GetFramework()->GetSwapChain();
        FfxSwapchain         ffxSwapChain = ffxGetSwapchainDX12(pSwapchain->GetImpl()->DX12SwapChain());
        // make sure swapchain is not holding a ref to real swapchain
        GetFramework()->GetSwapChain()->GetImpl()->SetDXGISwapChain(nullptr);

        ID3D12CommandQueue* pCmdQueue    = GetDevice()->GetImpl()->DX12CmdQueue(CommandQueue::Graphics);
        FfxCommandQueue     ffxGameQueue = ffxGetCommandQueueDX12(pCmdQueue);

        ffxReplaceSwapchainForFrameinterpolation(ffxGameQueue, ffxSwapChain);

        // Set frameinterpolation swapchain to engine
        IDXGISwapChain4* frameinterpolationSwapchain = ffxGetDX12SwapchainPtr(ffxSwapChain);
        GetFramework()->GetSwapChain()->GetImpl()->SetDXGISwapChain(frameinterpolationSwapchain);
        // Framework swapchain adds to the refcount, so we need to release the swapchain here
        frameinterpolationSwapchain->Release();

        // In case the app is handling Alt-Enter manually we need to update the window association after creating a different swapchain
        IDXGIFactory7* factory = nullptr;
        if (SUCCEEDED(frameinterpolationSwapchain->GetParent(IID_PPV_ARGS(&factory))))
        {
            factory->MakeWindowAssociation(GetFramework()->GetInternal()->GetHWND(), DXGI_MWA_NO_WINDOW_CHANGES);
            factory->Release();
        }

        // Lets do the same for HDR as well as it will need to be re initialized since swapchain was re created
        {
            GetFramework()->GetSwapChain()->SetHDRMetadataAndColorspace();
        }
    }
#else
    // FSR3 not supported on vulkan backend, so proxy swapchain is not required
#endif

    cauldron::SwapChain* swapchain   = cauldron::GetFramework()->GetSwapChain();
    TextureDesc          hudlessDesc = swapchain->GetBackBufferRT()->GetDesc();
    hudlessDesc.MipLevels            = 1;
    hudlessDesc.Name                 = L"FSR3_HudLessTexture0";
    hudlessDesc.Flags                = ResourceFlags::AllowUnorderedAccess | ResourceFlags::AllowRenderTarget;
    m_pHudLessTexture[0]             = GetDynamicResourcePool()->CreateRenderTexture(
        &hudlessDesc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
            desc.Width  = displayWidth;
            desc.Height = displayHeight;
        });
    hudlessDesc.Name     = L"FSR3_HudLessTexture1";
    m_pHudLessTexture[1] = GetDynamicResourcePool()->CreateRenderTexture(
        &hudlessDesc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
            desc.Width  = displayWidth;
            desc.Height = displayHeight;
        });

    //////////////////////////////////////////////////////////////////////////
    // Finish up init

    // Start disabled as this will be enabled externally
    SetModuleEnabled(false);

    // That's all we need for now
    SetModuleReady(true);
}

FSR3RenderModule::~FSR3RenderModule()
{
    // Protection
    if (ModuleEnabled())
        EnableModule(false);    // Destroy FSR context

            // ToDo: unset FrameinterpolationSwapchain
#ifdef FFX_API_CAULDRON
    // FSR3 not supported in cauldron backend, so proxy swapchain destruction is not required
#elif defined(FFX_API_DX12)
    //if (false)
    {
        cauldron::SwapChain* pSwapchain  = GetFramework()->GetSwapChain();
        IDXGISwapChain4*     pSwapChain4 = pSwapchain->GetImpl()->DX12SwapChain();
        pSwapChain4->AddRef();

        DXGI_SWAP_CHAIN_DESC1 desc;
        IDXGIFactory7*        factory   = nullptr;
        ID3D12CommandQueue*   pCmdQueue = GetDevice()->GetImpl()->DX12CmdQueue(CommandQueue::Graphics);
        pSwapChain4->GetDesc1(&desc);

        // Setup a new swapchain for HWND and set it to cauldron
        IDXGISwapChain1* pSwapChain1 = nullptr;
        if (SUCCEEDED(pSwapChain4->GetParent(IID_PPV_ARGS(&factory))))
        {
            GetFramework()->GetSwapChain()->GetImpl()->SetDXGISwapChain(nullptr);
            pSwapChain4->Release();

            if (SUCCEEDED(factory->CreateSwapChainForHwnd(
                    pCmdQueue, gameSwapChainDesc.OutputWindow, &gameSwapChainDesc1, &gameFullscreenDesc, nullptr, &pSwapChain1)))
            {
                if (SUCCEEDED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain4))))
                {
                    GetFramework()->GetSwapChain()->GetImpl()->SetDXGISwapChain(pSwapChain4);
                    pSwapChain4->Release();
                }
                pSwapChain1->Release();
            }
            factory->MakeWindowAssociation(GetFramework()->GetInternal()->GetHWND(), DXGI_MWA_NO_WINDOW_CHANGES);
            factory->Release();
        }
    }
#else
    // FSR3 not supported in native Vulkan backend, so proxy swapchain destruction is not required
#endif

    // Clear out raster views
    m_RasterViews.clear();
}

void FSR3RenderModule::EnableModule(bool enabled)
{
    // If disabling the render module, we need to disable the upscaler with the framework
    if (!enabled)
    {
        // Toggle this now so we avoid the context changes in OnResize
        SetModuleEnabled(enabled);

        // Destroy the FSR3 context
        UpdateFSR3Context(false);

        GetFramework()->EnableUpscaling(false);
#if FFX_API_DX12
        GetFramework()->EnableFrameInterpolation(false);
#endif

        UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
        uimod->SetAsyncRender(false);
        uimod->SetRenderToTexture(false);
        uimod->SetCopyHudLessTexture(false);

        if (ffxBackendInitialized_ == true)
        {
            for (auto i = 0; i < FSR3_BACKEND_COUNT; i++)
            {
                free(ffxFsr3Backends_[i].scratchBuffer);
                ffxFsr3Backends_[i].scratchBuffer = nullptr;
            }
            ffxBackendInitialized_ = false;
        }

        CameraComponent::SetJitterCallbackFunc(nullptr);

        // Deregister UI elements for inactive upscaler
        GetUIManager()->UnRegisterUIElements(m_UISection);
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

        // Setup Cauldron FidelityFX interface.
        if (!ffxBackendInitialized_)
        {
            FfxErrorCode errorCode = 0;

            int effectCounts[] = {1, 1, 2};
            for (auto i = 0; i < FSR3_BACKEND_COUNT; i++)
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySize(effectCounts[i]);
                void*        scratchBuffer     = calloc(scratchBufferSize, 1);
                memset(scratchBuffer, 0, scratchBufferSize);
                errorCode |= ffxGetInterface(&ffxFsr3Backends_[i], GetDevice(), scratchBuffer, scratchBufferSize, effectCounts[i]);
            }

            ffxBackendInitialized_ = (errorCode == FFX_OK);
            FFX_ASSERT(ffxBackendInitialized_);

            m_InitializationParameters.backendInterfaceSharedResources = ffxFsr3Backends_[FSR3_BACKEND_SHARED_RESOURCES];
            m_InitializationParameters.backendInterfaceUpscaling          = ffxFsr3Backends_[FSR3_BACKEND_UPSCALING];
            m_InitializationParameters.backendInterfaceFrameInterpolation = ffxFsr3Backends_[FSR3_BACKEND_FRAME_INTERPOLATION];
        }

        // Create the FSR3 context
        UpdateFSR3Context(true);

        // Set the jitter callback to use
        CameraJitterCallback jitterCallback = [this](Vec2& values) {
            // Increment jitter index for frame
            ++m_JitterIndex;

            // Update FSR3 jitter for built in TAA
            const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
            const int32_t jitterPhaseCount = ffxFsr3GetJitterPhaseCount(resInfo.RenderWidth, resInfo.DisplayWidth);
            ffxFsr3GetJitterOffset(&m_JitterX, &m_JitterY, m_JitterIndex, jitterPhaseCount);

            values = Vec2(-2.f * m_JitterX / resInfo.RenderWidth, 2.f * m_JitterY / resInfo.RenderHeight);
        };
        CameraComponent::SetJitterCallbackFunc(jitterCallback);

        // ... and register UI elements for active upscaler
        GetUIManager()->RegisterUIElements(m_UISection);
    }
}

void FSR3RenderModule::UpdatePreset(const int32_t* pOldPreset)
{
    switch (m_ScalePreset)
    {
    case FSR3ScalePreset::NativeAA:
        m_UpscaleRatio = 1.0f;
        break;
    case FSR3ScalePreset::Quality:
        m_UpscaleRatio = 1.5f;
        break;
    case FSR3ScalePreset::Balanced:
        m_UpscaleRatio = 1.7f;
        break;
    case FSR3ScalePreset::Performance:
        m_UpscaleRatio = 2.0f;
        break;
    case FSR3ScalePreset::UltraPerformance:
        m_UpscaleRatio = 3.0f;
        break;
    case FSR3ScalePreset::Custom:
    default:
        // Leave the upscale ratio at whatever it was
        break;
    }

    // Update whether we can update the custom scale slider
    m_UpscaleRatioEnabled = (m_ScalePreset == FSR3ScalePreset::Custom);

    // Update mip bias
    float oldValue = m_MipBias;
    if (m_ScalePreset != FSR3ScalePreset::Custom)
        m_MipBias = cMipBias[static_cast<uint32_t>(m_ScalePreset)];
    else
        m_MipBias = std::log2f(1.f / m_UpscaleRatio) - 1.f + std::numeric_limits<float>::epsilon();
    UpdateMipBias(&oldValue);

    // Update resolution since rendering ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
#if FFX_API_DX12
    GetFramework()->EnableFrameInterpolation(true);
#endif
}

void FSR3RenderModule::UpdateUpscaleRatio(const float* pOldRatio)
{
    // Disable/Enable FSR3 since resolution ratios have changed
    GetFramework()->EnableUpscaling(true, m_pUpdateFunc);
}

void FSR3RenderModule::UpdateMipBias(const float* pOldBias)
{
    // Update the scene MipLODBias to use
    GetScene()->SetMipLODBias(m_MipBias);
}

void FSR3RenderModule::FfxMsgCallback(FfxMsgType type, const wchar_t* message)
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

// copied from ffx_cauldron.cpp, needed for UI callback
static ResourceFormat CauldronGetSurfaceFormatFfx(FfxSurfaceFormat format)
{
    switch (format)
    {
    case FFX_SURFACE_FORMAT_UNKNOWN:
        return ResourceFormat::Unknown;
    case FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS:
        return ResourceFormat::RGBA32_TYPELESS;
    case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT:
        return ResourceFormat::RGBA32_FLOAT;
    case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT:
        return ResourceFormat::RGBA16_FLOAT;
    case FFX_SURFACE_FORMAT_R32G32_FLOAT:
        return ResourceFormat::RG32_FLOAT;
    case FFX_SURFACE_FORMAT_R32_UINT:
        return ResourceFormat::R32_UINT;
    case FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS:
        return ResourceFormat::RGBA8_TYPELESS;
    case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM:
        return ResourceFormat::RGBA8_UNORM;
    case FFX_SURFACE_FORMAT_R8G8B8A8_SNORM:
        return ResourceFormat::RGBA8_SNORM;
    case FFX_SURFACE_FORMAT_R8G8B8A8_SRGB:
        return ResourceFormat::RGBA8_SRGB;
    case FFX_SURFACE_FORMAT_R11G11B10_FLOAT:
        return ResourceFormat::RG11B10_FLOAT;
    case FFX_SURFACE_FORMAT_R16G16_FLOAT:
        return ResourceFormat::RG16_FLOAT;
    case FFX_SURFACE_FORMAT_R16G16_UINT:
        return ResourceFormat::RG16_UINT;
    case FFX_SURFACE_FORMAT_R16G16_SINT:
        return ResourceFormat::RG16_SINT;
    case FFX_SURFACE_FORMAT_R16_FLOAT:
        return ResourceFormat::R16_FLOAT;
    case FFX_SURFACE_FORMAT_R16_UINT:
        return ResourceFormat::R16_UINT;
    case FFX_SURFACE_FORMAT_R16_UNORM:
        return ResourceFormat::R16_UNORM;
    case FFX_SURFACE_FORMAT_R16_SNORM:
        return ResourceFormat::R16_SNORM;
    case FFX_SURFACE_FORMAT_R8_UNORM:
        return ResourceFormat::R8_UNORM;
    case FFX_SURFACE_FORMAT_R8_UINT:
        return ResourceFormat::R8_UINT;
    case FFX_SURFACE_FORMAT_R8G8_UNORM:
        return ResourceFormat::RG8_UNORM;
    case FFX_SURFACE_FORMAT_R32_FLOAT:
        return ResourceFormat::R32_FLOAT;
    case FFX_SURFACE_FORMAT_R10G10B10A2_UNORM:
        return ResourceFormat::RGB10A2_UNORM;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported format requested. Please implement.");
        return ResourceFormat::Unknown;
    }
}

// copied from ffx_cauldron.cpp, needed for UI callback
static ResourceFlags CauldronGetResourceFlagsFfx(FfxResourceUsage flags)
{
    ResourceFlags cauldronResourceFlags = ResourceFlags::None;

    if (flags & FFX_RESOURCE_USAGE_RENDERTARGET)
        cauldronResourceFlags |= ResourceFlags::AllowRenderTarget;

    if (flags & FFX_RESOURCE_USAGE_DEPTHTARGET)
        cauldronResourceFlags |= ResourceFlags::AllowDepthStencil;

    if (flags & FFX_RESOURCE_USAGE_UAV)
        cauldronResourceFlags |= ResourceFlags::AllowUnorderedAccess;

    if (flags & FFX_RESOURCE_USAGE_INDIRECT)
        cauldronResourceFlags |= ResourceFlags::AllowIndirect;

    return cauldronResourceFlags;
}

// copied from ffx_cauldron.cpp, MODIFIED!, needed for UI callback
static ResourceState CauldronGetStateFfx(FfxResourceStates state)
{
    switch (state)
    {
    case FFX_RESOURCE_STATE_UNORDERED_ACCESS:
        return ResourceState::UnorderedAccess;
    case FFX_RESOURCE_STATE_COMPUTE_READ:
        return ResourceState::NonPixelShaderResource;
    case FFX_RESOURCE_STATE_PIXEL_READ:
        return ResourceState::PixelShaderResource;
    case FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ:
        return ResourceState(ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    case FFX_RESOURCE_STATE_COPY_SRC:
        return ResourceState::CopySource;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return ResourceState::CopyDest;
    case FFX_RESOURCE_STATE_GENERIC_READ:
        return (ResourceState)((uint32_t)ResourceState::NonPixelShaderResource | (uint32_t)ResourceState::PixelShaderResource);
    case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return ResourceState::IndirectArgument;
    case FFX_RESOURCE_STATE_PRESENT:
        return ResourceState::Present;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported resource state requested. Please implement.");
        return ResourceState::CommonResource;
    }
}

static ResourceView* AllocCPURenderView()
{
    ResourceView* retVal;
    GetResourceViewAllocator()->AllocateCPURenderViews(&retVal);
    return retVal;
}
#if FFX_API_DX12
FfxErrorCode FSR3RenderModule::UiCompositionCallback(const FfxPresentCallbackDescription* params)
{
    FSR3RenderModule* fsr3mod = static_cast<FSR3RenderModule*>(GetFramework()->GetRenderModule("FSR3RenderModule"));
    UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));

    if (fsr3mod->s_uiRenderMode != 2)
        return FFX_ERROR_INVALID_ARGUMENT;
    
    // Reverse of validation_remap functions.

    // wrap everything in cauldron wrappers to allow backend agnostic execution of UI render.
    ID3D12GraphicsCommandList2* pDxCmdList = reinterpret_cast<ID3D12GraphicsCommandList2*>(params->commandList);
    CommandListInternal         cmdList = {L"UI_CommandList", pDxCmdList, nullptr, CommandQueue::Graphics};
    ID3D12Resource*             pRtResource = reinterpret_cast<ID3D12Resource*>(params->outputSwapChainBuffer.resource);
    ID3D12Resource*             pBbResource = reinterpret_cast<ID3D12Resource*>(params->currentBackBuffer.resource);
    pRtResource->AddRef();
    GPUResourceInternal rtResource(pRtResource, L"UI_RenderTarget", CauldronGetStateFfx(params->outputSwapChainBuffer.state), false);
    pBbResource->AddRef();
    GPUResourceInternal bbResource(pBbResource, L"BackBuffer", CauldronGetStateFfx(params->currentBackBuffer.state), false);
    
    std::vector<Barrier> barriers;
    cauldron::ResourceState rtResourceState = CauldronGetStateFfx(params->outputSwapChainBuffer.state);
    barriers                                = {Barrier::Transition(&rtResource, rtResourceState, ResourceState::CopyDest),
                                               Barrier::Transition(&bbResource, CauldronGetStateFfx(params->currentBackBuffer.state), ResourceState::CopySource)};
    ResourceBarrier(&cmdList, (uint32_t)barriers.size(), barriers.data());
        
    pDxCmdList->CopyResource(pRtResource, pBbResource);

    barriers[0].SourceState = barriers[0].DestState;
    barriers[0].DestState   = ResourceState::RenderTargetResource;
    swap(barriers[1].SourceState, barriers[1].DestState);
    ResourceBarrier(&cmdList, (uint32_t)barriers.size(), barriers.data());

    // FIXME: this should be a member variable, but this is a static method. Need context param on this callback.
    static ResourceView* pRtResourceView = AllocCPURenderView();

    // Create and set RTV, required for async UI render.
    FfxResourceDescription rdesc = params->outputSwapChainBuffer.description;

    TextureDesc rtResourceDesc = TextureDesc::Tex2D(L"UI_RenderTarget",
                                                    CauldronGetSurfaceFormatFfx(rdesc.format),
                                                    rdesc.width,
                                                    rdesc.height,
                                                    rdesc.depth,
                                                    rdesc.mipCount,
                                                    ResourceFlags::AllowRenderTarget);
    pRtResourceView->BindTextureResource(&rtResource, rtResourceDesc, ResourceViewType::RTV, ViewDimension::Texture2D, 0, 1, 0);
    pDxCmdList->OMSetRenderTargets(1, &pRtResourceView->GetViewInfo().GetImpl()->hCPUHandle, false, nullptr);

    SetAllResourceViewHeaps(&cmdList);

    uimod->ExecuteAsync(&cmdList);

    ResourceBarrier(&cmdList, 1, &Barrier::Transition(&rtResource, ResourceState::RenderTargetResource, rtResourceState));

    return FFX_OK;
}
#endif

void FSR3RenderModule::UpdateFSR3Context(bool enabled)
{
    if (enabled)
    {
        const ResolutionInfo& resInfo                           = GetFramework()->GetResolutionInfo();
        m_InitializationParameters.maxRenderSize.width          = resInfo.RenderWidth;
        m_InitializationParameters.maxRenderSize.height         = resInfo.RenderHeight;
        m_InitializationParameters.upscaleOutputSize.width      = resInfo.DisplayWidth;
        m_InitializationParameters.upscaleOutputSize.height     = resInfo.DisplayHeight;
        m_InitializationParameters.displaySize.width            = resInfo.DisplayWidth;
        m_InitializationParameters.displaySize.height           = resInfo.DisplayHeight;
        m_InitializationParameters.flags                        = FFX_FSR3_ENABLE_AUTO_EXPOSURE;
        static bool s_InvertedDepth                             = GetConfig()->InvertedDepth;
        if (s_InvertedDepth)
            m_InitializationParameters.flags |= FFX_FSR3_ENABLE_DEPTH_INVERTED | FFX_FSR3_ENABLE_DEPTH_INFINITE;
        m_InitializationParameters.flags |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;

#if defined(FFX_API_DX12)
        m_EnableAsyncCompute = m_PendingEnableAsyncCompute;
        if (m_EnableAsyncCompute)
            m_InitializationParameters.flags |= FFX_FSR3_ENABLE_ASYNC_WORKLOAD_SUPPORT;
#endif

        // Do eror checking in debug
#if defined(_DEBUG)
        m_InitializationParameters.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
        m_InitializationParameters.fpMessage = &FSR3RenderModule::FfxMsgCallback;
#endif  // #if defined(_DEBUG)

        // Create the FSR3 context
        {

            m_InitializationParameters.backBufferFormat = GetFfxSurfaceFormat(GetFramework()->GetSwapChain()->GetSwapChainFormat());

            // create the context.
            FfxErrorCode errorCode = ffxFsr3ContextCreate(&m_FSR3Context, &m_InitializationParameters);
#if defined(FFX_API_DX12)
            FFX_ASSERT(errorCode == FFX_OK);

            cauldron::SwapChain* pSwapchain   = GetFramework()->GetSwapChain();
            FfxSwapchain         ffxSwapChain = ffxGetSwapchainDX12(pSwapchain->GetImpl()->DX12SwapChain());

            // configure frame generation
            FfxResourceDescription hudLessDesc = GetFfxResourceDescription(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), (FfxResourceUsage)0);
            FfxResource hudLessResource = ffxGetResourceDX12(m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetImpl()->DX12Resource(), hudLessDesc, L"FSR3_HudLessBackbuffer", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

            m_FrameGenerationConfig.frameGenerationEnabled  = false;
            m_FrameGenerationConfig.frameGenerationCallback = ffxFsr3DispatchFrameGeneration;
            m_FrameGenerationConfig.presentCallback         = (s_uiRenderMode == 2) ? UiCompositionCallback : nullptr;
            m_FrameGenerationConfig.swapChain               = ffxSwapChain;
            m_FrameGenerationConfig.HUDLessColor            = (s_uiRenderMode == 3) ? hudLessResource : FfxResource({});

            errorCode = ffxFsr3ConfigureFrameGeneration(&m_FSR3Context, &m_FrameGenerationConfig);
#endif
            FFX_ASSERT(errorCode == FFX_OK);
        }
    }
    else
    {
#if defined(FFX_API_DX12)
        cauldron::SwapChain* pSwapchain   = GetFramework()->GetSwapChain();
        FfxSwapchain         ffxSwapChain = ffxGetSwapchainDX12(pSwapchain->GetImpl()->DX12SwapChain());

        // disable frame generation before destroying context
        // also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
        m_FrameGenerationConfig.frameGenerationEnabled = false;
        m_FrameGenerationConfig.swapChain              = ffxSwapChain;
        m_FrameGenerationConfig.presentCallback        = nullptr;
        m_FrameGenerationConfig.HUDLessColor           = FfxResource({});
        ffxFsr3ConfigureFrameGeneration(&m_FSR3Context, &m_FrameGenerationConfig);

        ffxRegisterFrameinterpolationUiResource(ffxSwapChain, FfxResource({}));
#endif

        // Destroy the FSR3 context
        ffxFsr3ContextDestroy(&m_FSR3Context);
    }
}

ResolutionInfo FSR3RenderModule::UpdateResolution(uint32_t displayWidth, uint32_t displayHeight)
{
    return { static_cast<uint32_t>((float)displayWidth / m_UpscaleRatio),
             static_cast<uint32_t>((float)displayHeight / m_UpscaleRatio),
             displayWidth, displayHeight };
}

void FSR3RenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    // Need to recreate the FSR3 context on resource resize
    UpdateFSR3Context(false);   // Destroy
    UpdateFSR3Context(true);    // Re-create

    // Reset jitter index
    m_JitterIndex = 0;
}

void FSR3RenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    if (m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState() != ResourceState::NonPixelShaderResource)
    {
        Barrier barrier = Barrier::Transition(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(),
                                              m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState(),
                                              ResourceState::NonPixelShaderResource);
        ResourceBarrier(pCmdList, 1, &barrier);
    }

    if (m_pUiTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState() != cauldron::ResourceState::ShaderResource)
    {
        std::vector<Barrier> barriers = {Barrier::Transition(m_pUiTexture[m_curUiTextureIndex]->GetResource(),
                                                             m_pUiTexture[m_curUiTextureIndex]->GetResource()->GetCurrentResourceState(),
                                                             ResourceState::ShaderResource)};
        ResourceBarrier(pCmdList, (uint32_t)barriers.size(), barriers.data());
    }

    GPUScopedProfileCapture sampleMarker(pCmdList, L"FFX FSR3 Upscaler");
    const ResolutionInfo&   resInfo = GetFramework()->GetResolutionInfo();
    CameraComponent*        pCamera = GetScene()->GetCurrentCamera();

#if FFX_API_DX12
    GPUResource*           swapchainBackbuffer = GetFramework()->GetSwapChain()->GetBackBufferRT()->GetCurrentResource();
    FfxResourceDescription swapChainDesc       = GetFfxResourceDescriptionDX12(swapchainBackbuffer->GetImpl()->DX12Resource());
    FfxResource backbuffer = ffxGetResourceDX12(swapchainBackbuffer->GetImpl()->DX12Resource(), swapChainDesc, L"SwapchainSurface", FFX_RESOURCE_STATE_PRESENT);
    
    IDXGISwapChain4* pSwapchain          = GetFramework()->GetSwapChain()->GetImpl()->DX12SwapChain();
    //FfxResource      interpolationOutput = ffxGetFrameinterpolationTextureDX12((FfxSwapchain)pSwapchain);

    // Upscale the scene first
    {
        // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
        FfxFsr3DispatchUpscaleDescription dispatchParameters = {};
        dispatchParameters.commandList                       = ffxGetCommandList(pCmdList);
        dispatchParameters.color         = ffxGetResource(m_pColorTarget->GetResource(), L"FSR3_Input_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchParameters.depth         = ffxGetResource(m_pDepthTarget->GetResource(), L"FSR3_InputDepth", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchParameters.motionVectors = ffxGetResource(m_pMotionVectors->GetResource(), L"FSR3_InputMotionVectors", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchParameters.exposure      = ffxGetResource(nullptr, L"FSR3_InputExposure", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchParameters.upscaleOutput = dispatchParameters.color;

        if (m_MaskMode != FSR3MaskMode::Disabled)
        {
            dispatchParameters.reactive = ffxGetResource(m_pReactiveMask->GetResource(), L"FSR3_InputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }
        else
        {
            dispatchParameters.reactive = ffxGetResource(nullptr, L"FSR3_EmptyInputReactiveMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }

        if (m_UseMask)
        {
            dispatchParameters.transparencyAndComposition =
                ffxGetResource(m_pCompositionMask->GetResource(), L"FSR3_TransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }
        else
        {
            dispatchParameters.transparencyAndComposition =
                ffxGetResource(nullptr, L"FSR3_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }

        // Jitter is calculated earlier in the frame using a callback from the camera update
        dispatchParameters.jitterOffset.x      = -m_JitterX;
        dispatchParameters.jitterOffset.y      = -m_JitterY;
        dispatchParameters.motionVectorScale.x = resInfo.fRenderWidth();
        dispatchParameters.motionVectorScale.y = resInfo.fRenderHeight();
        dispatchParameters.reset               = false;
        dispatchParameters.enableSharpening    = m_RCASSharpen;
        dispatchParameters.sharpness           = m_Sharpness;

        // Cauldron keeps time in seconds, but FSR expects miliseconds
        dispatchParameters.frameTimeDelta = (float)deltaTime * 1000.f;

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

        // Update frame generation config
        FfxResourceDescription hudLessDesc = GetFfxResourceDescription(m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), (FfxResourceUsage)0);
        FfxResource hudLessResource = ffxGetResourceDX12(m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetImpl()->DX12Resource(),
                                                         hudLessDesc,
                                                         L"FSR3_HudLessBackbuffer",
                                                         FFX_RESOURCE_STATE_COMPUTE_READ);

        m_FrameGenerationConfig.frameGenerationEnabled = m_FrameInterpolation;
        m_FrameGenerationConfig.flags                  = 0;
        m_FrameGenerationConfig.flags |= m_DrawDebugTearLines ? FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
        m_FrameGenerationConfig.flags |= m_DrawDebugView ? FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
        m_FrameGenerationConfig.HUDLessColor = (s_uiRenderMode == 3) ? hudLessResource : FfxResource({});
        m_FrameGenerationConfig.allowAsyncWorkloads = m_AllowAsyncCompute && m_EnableAsyncCompute;
        m_FrameGenerationConfig.frameGenerationCallback = m_UseCallback ? ffxFsr3DispatchFrameGeneration : nullptr;

        ffxFsr3ConfigureFrameGeneration(&m_FSR3Context, &m_FrameGenerationConfig);

        FfxErrorCode errorCode = ffxFsr3ContextDispatchUpscale(&m_FSR3Context, &dispatchParameters);
        FFX_ASSERT(errorCode == FFX_OK);



        FfxResource uiColor = (s_uiRenderMode == 1)
                                  ? ffxGetResource(m_pUiTexture[m_curUiTextureIndex]->GetResource(), L"FSR3_UiTexture", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ)
                                  : FfxResource({});
        cauldron::SwapChain* pSwapchain   = GetFramework()->GetSwapChain();
        FfxSwapchain ffxSwapChain = ffxGetSwapchainDX12(pSwapchain->GetImpl()->DX12SwapChain());
        ffxRegisterFrameinterpolationUiResource(ffxSwapChain, uiColor);

#if defined(FFX_API_DX12)
        // Dispatch frame generation, if not using the callback
        if (m_FrameInterpolation && !m_UseCallback)
        {
            FfxFrameGenerationDispatchDescription fgDesc{};

#if defined(FFX_API_DX12)
            IDXGISwapChain4* dxgiSwapchain = GetFramework()->GetSwapChain()->GetImpl()->DX12SwapChain();
            ffxGetInterpolationCommandlist(ffxGetSwapchainDX12(dxgiSwapchain), fgDesc.commandList);
#endif

            fgDesc.presentColor          = backbuffer;
            fgDesc.numInterpolatedFrames = 1;
            fgDesc.outputs[0]            = ffxGetFrameinterpolationTextureDX12(ffxGetSwapchainDX12(dxgiSwapchain));
            ffxFsr3DispatchFrameGeneration(&fgDesc);
        }
#endif
    }

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);

    // We are now done with upscaling
    GetFramework()->SetUpscalingState(UpscalerState::PostUpscale);
#endif
}

void FSR3RenderModule::PreTransCallback(double deltaTime, cauldron::CommandList* pCmdList)
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

    if (m_MaskMode != FSR3MaskMode::Auto)
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
    m_curUiTextureIndex = (++m_curUiTextureIndex) & 1;
    uimod->SetUiSurfaceIndex(m_curUiTextureIndex);
}

void FSR3RenderModule::PostTransCallback(double deltaTime, cauldron::CommandList* pCmdList)
{
    if (m_MaskMode != FSR3MaskMode::Auto)
        return;

    GPUScopedProfileCapture sampleMarker(pCmdList, L"Gen Reactive Mask (FSR3)");

    FfxFsr3GenerateReactiveDescription generateReactiveParameters = {};
    generateReactiveParameters.commandList = ffxGetCommandList(pCmdList);
    generateReactiveParameters.colorOpaqueOnly = ffxGetResource(m_pOpaqueTexture->GetResource(), L"FSR3_Input_Opaque_Color", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    generateReactiveParameters.colorPreUpscale = ffxGetResource(m_pColorTarget->GetResource(), L"FSR3_Input_PreUpscaleColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    generateReactiveParameters.outReactive = ffxGetResource(m_pReactiveMask->GetResource(), L"FSR3_InputReactiveMask", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

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
    FFX_ASSERT(errorCode == FFX_OK);

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);
}


