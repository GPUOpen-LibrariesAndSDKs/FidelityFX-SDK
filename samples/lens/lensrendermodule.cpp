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


#include "lensrendermodule.h"
#include "validation_remap.h"

#include "core/uimanager.h"
#include "render/device.h"
#include "render/dynamicresourcepool.h"
#include "render/profiler.h"
#include "render/uploadheap.h"

using namespace cauldron;

static const char* s_FloatingPointMathOptions[] = {"Use FP32", "Use FP16"};
FfxLensFloatPrecision GetFloatPrecision(int32_t fpMathIndex)
{
    std::string floatingPointMath = s_FloatingPointMathOptions[fpMathIndex];
    if (floatingPointMath == "Use FP16")
        return FFX_LENS_FLOAT_PRECISION_16BIT;
    else if (floatingPointMath == "Use FP32")
        return FFX_LENS_FLOAT_PRECISION_32BIT;
    else
        FFX_ASSERT_FAIL("Unhandled float precision value.");
    return FFX_LENS_FLOAT_PRECISION_COUNT;
}

static uint32_t CalcGrainSeed(double deltaTime, double seedUpdateRate)
{
    // Update seed for grain at fixed time intervals
    static uint32_t seedCtr = 0;
    static double   timeCtr = 0.0;
    timeCtr += deltaTime;
    if (timeCtr >= seedUpdateRate)
    {
        ++seedCtr;
        timeCtr = 0.0;
    }
    return seedCtr;
}

void LensRenderModule::Init(const json& initData)
{
    // Fetch needed resource
    m_pColorSrc = GetFramework()->GetColorTargetForCallback(GetName());

    switch (m_pColorSrc->GetFormat())
    {
    case ResourceFormat::RGBA16_FLOAT:
        m_InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        break;
        // For all other instances, just use floating point 32-bit format
    default:
        m_InitializationParameters.outputFormat = FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
        break;
    }

    // Create lens intermediate texture
    TextureDesc           desc    = m_pColorSrc->GetDesc();
    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
    desc.Width                    = resInfo.RenderWidth;
    desc.Height                   = resInfo.RenderHeight;
    desc.Name                     = L"Lens_Intermediate_Color";
    m_pColorIntermediate          = GetDynamicResourcePool()->CreateRenderTexture(
        &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
            desc.Width  = renderingWidth;
            desc.Height = renderingHeight;
        });

    // Register UI
    UISection uiSection;
    uiSection.SectionName = "Lens effects";
    uiSection.SectionType = UISectionType::Sample;

    std::vector<std::string> comboOptions;

    // Use the same callback for all option changes, which will always destroy/create the context
    std::function<void(void*)> optionChangeCallback = [this](void* pParams) {
        if (!m_ContextCreated)
            return;

        // Refresh
        UpdateLensContext(false);
        UpdateLensContext(true);
    };

    // Add math combo
    comboOptions.clear();
    comboOptions.assign(s_FloatingPointMathOptions, s_FloatingPointMathOptions + _countof(s_FloatingPointMathOptions));
    uiSection.AddCombo("Lens Math", (int32_t*)&m_LensMath, &comboOptions, optionChangeCallback);

    // Sliders for lens artistic constants
    m_grainScale = 0.01f;
    uiSection.AddFloatSlider("Grain scale",                 (float*)&m_grainScale, 0.01f, 20.0f);

    m_grainAmount = 0.7f;
    uiSection.AddFloatSlider("Grain amount",                (float*)&m_grainAmount, 0.0f, 20.0f);

    m_chromAb = 1.65f;
    uiSection.AddFloatSlider("Chromatic aberration intensity", (float*)&m_chromAb, 0.0f, 20.0f);

    m_vignette = 0.6f;
    uiSection.AddFloatSlider("Vignette intensity",             (float*)&m_vignette, 0.0f, 2.0f);

    // Register the Lens section
    GetUIManager()->RegisterUIElements(uiSection);

    // Initialize the FFX backend
    const size_t scratchBufferSize = ffxGetScratchMemorySize(FFX_LENS_CONTEXT_COUNT);
    void* scratchBuffer = calloc(scratchBufferSize, 1);
    FfxErrorCode errorCode = ffxGetInterface(&m_InitializationParameters.backendInterface, GetDevice(), scratchBuffer, scratchBufferSize, FFX_LENS_CONTEXT_COUNT);
    FFX_ASSERT(errorCode == FFX_OK);

    // Init Lens
    UpdateLensContext(true);

    // We are now ready for use
    SetModuleReady(true);
}

LensRenderModule::~LensRenderModule()
{
    // Flush anything out of the pipes before destroying the context
    GetDevice()->FlushAllCommandQueues();

    // Tear down context
    UpdateLensContext(false);

    // Destroy the FidelityFX interface memory
    free(m_InitializationParameters.backendInterface.scratchBuffer);
}

void LensRenderModule::UpdateLensContext(bool enabled)
{
    if (!enabled)
    {
        // Flush anything out of the pipes before destroying the context
        GetDevice()->FlushAllCommandQueues();

        ffxLensContextDestroy(&m_LensContext);
        m_ContextCreated = false;
    }
    else
    {
        // Setup all the parameters for this Lens run
        m_InitializationParameters.flags |= m_LensMath == FFX_LENS_FLOAT_PRECISION_16BIT ? FFX_LENS_MATH_PACKED : FFX_LENS_MATH_NONPACKED;
        m_InitializationParameters.floatPrecision = GetFloatPrecision(m_LensMath);

        ffxLensContextCreate(&m_LensContext, &m_InitializationParameters);
        m_ContextCreated = true;
    }
}


void LensRenderModule::Execute(double deltaTime, CommandList* pCmdList)
{
    GPUScopedProfileCapture sampleMarker(pCmdList, L"Lens RM");

    const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

    FfxLensDispatchDescription dispatchParameters = {};
    dispatchParameters.commandList                = ffxGetCommandList(pCmdList);
    dispatchParameters.renderSize                 = {resInfo.RenderWidth, resInfo.RenderHeight};
    dispatchParameters.grainScale                 = m_grainScale;
    dispatchParameters.grainAmount                = m_grainAmount;
    dispatchParameters.grainSeed                  = CalcGrainSeed(deltaTime, m_seedUpdateRate);
    dispatchParameters.chromAb                    = m_chromAb;
    dispatchParameters.vignette                   = m_vignette;

    // Copy main color to intermediate buffer, then run lens on the intermediate, writing back into the main color buffer
    std::vector<Barrier> barriers;
    barriers.push_back(
        Barrier::Transition(m_pColorSrc->GetResource(), 
            ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
            ResourceState::CopySource));
    barriers.push_back(Barrier::Transition(m_pColorIntermediate->GetResource(), 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, 
        ResourceState::CopyDest));
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    TextureCopyDesc desc(m_pColorSrc->GetResource(), m_pColorIntermediate->GetResource());
    CopyTextureRegion(pCmdList, &desc);

    barriers[0] = Barrier::Transition(m_pColorIntermediate->GetResource(), 
        ResourceState::CopyDest, 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    barriers[1] = Barrier::Transition(m_pColorSrc->GetResource(), 
        ResourceState::CopySource, 
        ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
    ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

    // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
    dispatchParameters.resource = ffxGetResource(m_pColorIntermediate->GetResource(), L"Lens_Intermediate_Color", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchParameters.resourceOutput = ffxGetResource(m_pColorSrc->GetResource(), L"Lens_Output", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

    FfxErrorCode errorCode = ffxLensContextDispatch(&m_LensContext, &dispatchParameters);
    FFX_ASSERT(errorCode == FFX_OK);

    // FidelityFX contexts modify the set resource view heaps, so set the cauldron one back
    SetAllResourceViewHeaps(pCmdList);
}

void LensRenderModule::OnResize(const ResolutionInfo& resInfo)
{
    if (!ModuleEnabled())
        return;

    // Refresh
    UpdateLensContext(false);
    UpdateLensContext(true);
}
