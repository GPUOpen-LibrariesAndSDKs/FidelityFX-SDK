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


#include "fsrsample.h"
#include "fsr2rendermodule.h"
#include "fsr3upscalerendermodule.h"
#include "fsr3rendermodule.h"
#include "upscalerendermodule.h"
#include "fsr1rendermodule.h"
#include "upscalerendermodule.h"

#include "rendermoduleregistry.h"
#include "taa/taarendermodule.h"
#include "translucency/translucencyrendermodule.h"
#include "render/rendermodules/ui/uirendermodule.h"

#include "misc/fileio.h"
#include "render/device.h"
#include "misc/assert.h"

using namespace cauldron;

// Read in sample-specific configuration parameters.
// Cauldron defaults may also be overridden at this point
void FSRSample::ParseSampleConfig()
{
    json sampleConfig;
    CauldronAssert(ASSERT_CRITICAL, ParseJsonFile(L"configs/fsrconfig.json", sampleConfig), L"Could not parse JSON file %ls", L"fsrconfig.json");

    // Get the sample configuration
    json configData = sampleConfig["FidelityFX FSR"];

    // Let the framework parse all the "known" options for us
    ParseConfigData(configData);
}

void FSRSample::ParseSampleCmdLine(const wchar_t* cmdLine)
{
    // Process any command line parameters the sample looks for here
}

// Register sample's render modules so the factory can spawn them
void FSRSample::RegisterSampleModules()
{
    // Init all pre-registered render modules
    rendermodule::RegisterAvailableRenderModules();

    // Register sample render modules
    RenderModuleFactory::RegisterModule<FSR3RenderModule>("FSR3RenderModule");
    RenderModuleFactory::RegisterModule<FSR3UpscaleRenderModule>("FSR3UpscaleRenderModule");
    RenderModuleFactory::RegisterModule<FSR2RenderModule>("FSR2RenderModule");
    RenderModuleFactory::RegisterModule<FSR1RenderModule>("FSR1RenderModule");
    RenderModuleFactory::RegisterModule<UpscaleRenderModule>("UpscaleRenderModule");
}

// Sample initialization point
int32_t FSRSample::DoSampleInit()
{
    // Store pointers to various render modules
    m_pFSR2RenderModule = static_cast<FSR2RenderModule*>(GetFramework()->GetRenderModule("FSR2RenderModule"));
    m_pFSR3UpscaleRenderModule = static_cast<FSR3UpscaleRenderModule*>(GetFramework()->GetRenderModule("FSR3UpscaleRenderModule"));
    m_pFSR3RenderModule = static_cast<FSR3RenderModule*>(GetFramework()->GetRenderModule("FSR3RenderModule"));

    m_pFSR1RenderModule = static_cast<FSR1RenderModule*>(GetFramework()->GetRenderModule("FSR1RenderModule"));
    m_pUpscaleRenderModule = static_cast<UpscaleRenderModule*>(GetFramework()->GetRenderModule("UpscaleRenderModule"));
    m_pTAARenderModule = static_cast<TAARenderModule*>(GetFramework()->GetRenderModule("TAARenderModule"));
    m_pTransRenderModule = static_cast<TranslucencyRenderModule*>(GetFramework()->GetRenderModule("TranslucencyRenderModule"));

    m_pTAARenderModule->EnableModule(false);

    CauldronAssert(ASSERT_CRITICAL, m_pFSR3RenderModule, L"FidelityFX FSR Sample: Error: Could not find FSR3 render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pFSR3UpscaleRenderModule, L"FidelityFX FSR Sample: Error: Could not find FSR3Upscale render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pFSR2RenderModule, L"FidelityFX FSR Sample: Error: Could not find FSR2 render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pFSR1RenderModule, L"FidelityFX FSR Sample: Error: Could not find FSR1 render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pUpscaleRenderModule, L"FidelityFX FSR Sample: Error: Could not find upscale render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pTAARenderModule, L"FidelityFX FSR Sample: Error: Could not find TAA render module.");
    CauldronAssert(ASSERT_CRITICAL, m_pTransRenderModule, L"FidelityFX FSR Sample: Error: Could not find Translucency render module.");

    // Register additional exports for translucency pass
    const Texture* pReactiveMask = GetFramework()->GetRenderTexture(L"ReactiveMask");
    const Texture* pCompositionMask = GetFramework()->GetRenderTexture(L"TransCompMask");
    BlendDesc reactiveCompositionBlend = { true, Blend::InvDstColor, Blend::One, BlendOp::Add, Blend::One, Blend::Zero, BlendOp::Add, static_cast<uint32_t>(ColorWriteMask::Red) };

    OptionalTransparencyOptions transOptions;
    transOptions.OptionalTargets.push_back(std::make_pair(pReactiveMask, reactiveCompositionBlend));
    transOptions.OptionalTargets.push_back(std::make_pair(pCompositionMask, reactiveCompositionBlend));
    transOptions.OptionalAdditionalOutputs = L"float ReactiveTarget : SV_TARGET1; float CompositionTarget : SV_TARGET2;";
    transOptions.OptionalAdditionalExports = L"float hasAnimatedTexture = 0.f; output.ReactiveTarget = ReactiveMask; output.CompositionTarget = max(Alpha, hasAnimatedTexture);";

    // Add additional exports for FSR to translucency pass
    m_pTransRenderModule->AddOptionalTransparencyOptions(transOptions);

    // Set all other UI sections to collapse by default
    for (auto& section : GetUIManager()->GetGeneralLayout())
        if (section.SectionName != "FPS Limiter")
            section.defaultOpen = false;

    // Register upscale method picker picker
    UISection uiSection;
    uiSection.SectionName = "Upscaling";
    uiSection.SectionType = UISectionType::Sample;

#ifdef FFX_API_DX12
    // Setup upscale method options
    const char*              upscalers[] = {"Native", "Point", "Bilinear", "Bicubic", "FSR1", "FSR2", "FSR3Upscale", "FSR3"};
    std::vector<std::string> comboOptions;
    comboOptions.assign(upscalers, upscalers + _countof(upscalers));

    // Add the section header
    uiSection.AddCombo("Method", reinterpret_cast<int32_t*>(&m_UIMethod), &comboOptions);
    GetUIManager()->RegisterUIElements(uiSection);

    // Setup FSR3 as the default upscaler
    m_UIMethod = UpscaleMethod::FSR3;
#else
    // Setup upscale method options
    const char*              upscalers[] = {"Native", "Point", "Bilinear", "Bicubic", "FSR1", "FSR2"};
    std::vector<std::string> comboOptions;
    comboOptions.assign(upscalers, upscalers + _countof(upscalers));

    // Add the section header
    uiSection.AddCombo("Method", reinterpret_cast<int32_t*>(&m_UIMethod), &comboOptions);
    GetUIManager()->RegisterUIElements(uiSection);

    // Setup the default upscaler (FSR2 for now)
    m_UIMethod = UpscaleMethod::FSR2;
#endif
    SwitchUpscaler(m_UIMethod);
    #
    return 0;
}

void FSRSample::SwitchUpscaler(UpscaleMethod newUpscaler)
{
    // Flush everything out of the pipe before disabling/enabling things
    GetDevice()->FlushAllCommandQueues();

    UpscaleMethod oldUpscaler = m_Method;
    m_Method = newUpscaler;

    // If we are switching methods, handle it now
    RenderModule* pOldUpscaler = m_pCurrentUpscaler;
    
    // Disable the old upscaler
    if (pOldUpscaler)
        pOldUpscaler->EnableModule(false);

    // Render UI to separate target for FSR3
    UIRenderModule* uimod = static_cast<UIRenderModule*>(GetFramework()->GetRenderModule("UIRenderModule"));
    uimod->SetAsyncRender(m_Method == UpscaleMethod::FSR3);

    switch (m_Method)
    {
    case UpscaleMethod::Native:
        m_pCurrentUpscaler = nullptr;
        break;
    case UpscaleMethod::Point:
    case UpscaleMethod::Bilinear:
    case UpscaleMethod::Bicubic:
        // If updating from one default upscalers ... set the new filter parameter
        m_pUpscaleRenderModule->SetFilter(static_cast<UpscaleRM::UpscaleMethod>(m_Method));
        m_pCurrentUpscaler = m_pUpscaleRenderModule;
        break;
    case UpscaleMethod::FSR1:
        m_pCurrentUpscaler = m_pFSR1RenderModule;
        break;
    case UpscaleMethod::FSR2:
        m_pCurrentUpscaler = m_pFSR2RenderModule;
        break;
    case UpscaleMethod::FSR3UPSCALEONLY:
        m_pCurrentUpscaler = m_pFSR3UpscaleRenderModule;
        break;
    case UpscaleMethod::FSR3:
        m_pCurrentUpscaler = m_pFSR3RenderModule;
        m_pFSR3RenderModule->m_NeedReInit = false;
        break;
    default:
        CauldronCritical(L"Unsupported upscaler requested.");
        break;
    }

    // Enable the new one
    if (m_pCurrentUpscaler)
        m_pCurrentUpscaler->EnableModule(true);
}

void FSRSample::DoSampleUpdates(double deltaTime)
{
    // Upscaler changes need to be done before the rest of the frame starts executing
    // as it relies on the upscale method being set for the frame and whatnot
    if (m_UIMethod != m_Method || m_pFSR3RenderModule->m_NeedReInit)
    {
        m_Method = m_UIMethod;
        SwitchUpscaler(m_UIMethod);
    }

}

void FSRSample::DoSampleShutdown()
{
    if (m_pCurrentUpscaler)
        m_pCurrentUpscaler->EnableModule(false);
}
