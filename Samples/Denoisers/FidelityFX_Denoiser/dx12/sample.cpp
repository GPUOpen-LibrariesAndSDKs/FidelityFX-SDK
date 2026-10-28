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

#include "sample.h"
#include "pathtracingrendermodule.h"
#include "denoiserrendermodule.h"
#include "fsrapirendermodule.h"
#include "debugoverlayrendermodule.h"

#include <cauldron2/framework/misc/fileio.h>
#include <cauldron2/rendermodules/rendermoduleregistry.h>
#include <cauldron2/framework/render/rendermodule.h>
#include <cauldron2/framework/core/inputmanager.h>
#include <cauldron2/framework/core/components/animationcomponent.h>

using namespace cauldron;

namespace rr
{
    [[nodiscard]]
    Sample::Sample(const FrameworkInitParams* pInitParams)
        : Framework(pInitParams)
    {
    }

    Sample::~Sample() = default;

    // Read in sample-specific configuration parameters.
    // Cauldron defaults may also be overridden at this point
    void Sample::ParseSampleConfig()
    {
        json sampleConfig;
        CauldronAssert(
            ASSERT_CRITICAL, ParseJsonFile(L"configs\\denoiserconfig.json", sampleConfig), L"Could not parse JSON file %ls", L"configs\\denoiserconfig.json");

        // Get the sample configuration
        json configData = sampleConfig["FSR Ray Regeneration"];

        // Let the framework parse all the "known" options for us
        ParseConfigData(configData);
        // Read in any unknown options

        // Framework config parameters can optionally also be overridden here e.g.
        // m_Config.GPUValidationEnabled = true; // Force GPU Validation

        // If this sample has defined a rendermodule config, we also need to parse the config file for its rendermodule
        #ifdef RenderModuleConfig
        json rmConfig;
        CauldronAssert(ASSERT_CRITICAL, ParseJsonFile(RenderModuleConfig, rmConfig), L"Could not parse JSON file %ls", RenderModuleConfig);

        // Get the render module configuration
        json rmConfigData = rmConfig[RenderModuleConfig];

        // Let the framework parse all the "known" options for us
        ParseConfigData(rmConfigData);
        #endif // RenderModuleConfig
    }

    void Sample::ParseSampleCmdLine(const wchar_t* cmdLine)
    {
        (void)cmdLine;
    }

    // Register sample's render modules so the factory can spawn them
    void Sample::RegisterSampleModules()
    {
        // Init all pre-registered render modules
        rendermodule::RegisterAvailableRenderModules();

        // Register sample render module
        RenderModuleFactory::RegisterModule< PathTracingRenderModule >("PathTracingRenderModule");
        RenderModuleFactory::RegisterModule< DenoiserRenderModule >("DenoiserRenderModule");
        RenderModuleFactory::RegisterModule< FSRRenderModule >("FSRApiRenderModule");
        RenderModuleFactory::RegisterModule< DebugOverlayRenderModule >("DebugOverlayRenderModule");
    }

    // Sample initialization point
    void Sample::DoSampleInit()
    {
    }

    // Do any app-specific (global) updates here
    // This is called prior to components/render module updates
    void Sample::DoSampleUpdates(double deltaTime)
    {

        if (GetInputManager()->GetInputState().GetKeyUpState(Key_Space))
        {
            if (AnimationComponentMgr::Get())
            {
                if (AnimationComponentMgr::Get()->IsPaused())
                {
                    AnimationComponentMgr::Get()->UnPause();
                }
                else
                {
                    AnimationComponentMgr::Get()->Pause();
                }
            }
        }

        const InputState& inputState = GetInputManager()->GetInputState();
        FSRApiRenderModule* pFSR = dynamic_cast<FSRApiRenderModule*>(GetFramework()->GetRenderModule("FSRApiRenderModule"));
        DenoiserRenderModule* pDenoiser = dynamic_cast<DenoiserRenderModule*>(GetFramework()->GetRenderModule("DenoiserRenderModule"));

        if (pFSR)
        {
            // Upscale method hotkeys
            constexpr int32_t kUpscalerNative = 0;
            constexpr int32_t kUpscalerFSRAPI = 1;

            // Scale preset hotkeys
            constexpr int32_t kScalePresetNativeAA = 0;
            constexpr int32_t kScalePresetQuality = 1;
            constexpr int32_t kScalePresetBalanced = 2;
            constexpr int32_t kScalePresetPerformance = 3;
            constexpr int32_t kScalePresetUltraPerformance = 4;
            constexpr int32_t kScalePresetCount = 5;

            // Track FSR state
            static bool fsrEnabled = true;
            static int32_t currentPreset = kScalePresetPerformance; // Persistent preset state

            // F5: Toggle FSR method on/off
             // When enabling: set to FSR method with Native AA preset
             // When disabling: set to Native method
            if (inputState.GetKeyUpState(Key_F5))
            {
                fsrEnabled = not fsrEnabled;
                if (fsrEnabled)
                {
                    // Enable FSR method first, then set to Native AA preset
                    pFSR->SetUpscaleMethodHotkey(kUpscalerFSRAPI);
                    currentPreset = kScalePresetNativeAA;
                    pFSR->SetScalePresetHotkey(currentPreset);
                }
                else
                {
                    // Disable FSR, switch to Native method
                    pFSR->SetUpscaleMethodHotkey(kUpscalerNative);
                }
            }

            // F6: Cycle through scale presets
            // If Native method is active: enable FSR method and jump to Quality preset
            // If FSR method is active: cycle through all presets (Native AA -> Quality -> Balanced -> Performance -> Ultra Performance -> Native AA)
            if (inputState.GetKeyUpState(Key_F6))
            {
                if (not fsrEnabled)
                {
                    // Currently using Native method, enable FSR first then jump to Quality preset
                    pFSR->SetUpscaleMethodHotkey(kUpscalerFSRAPI);
                    fsrEnabled = true;
                    currentPreset = kScalePresetQuality;
                    pFSR->SetScalePresetHotkey(currentPreset);
                }
                else
                {
                    // FSR is already enabled, cycle to next preset
                    currentPreset = (currentPreset + 1) % kScalePresetCount;
                    pFSR->SetScalePresetHotkey(currentPreset);
                }
            }
        }

        if (pDenoiser)
        {
            // F8: Toggle dominant light visibility denoising
            if (inputState.GetKeyUpState(Key_F8))
            {
                pDenoiser->ToggleDominantLightVisibilityHotkey();
            }

            // F9: Cycle through view modes
            if (inputState.GetKeyUpState(Key_F9))
            {
                pDenoiser->CycleViewModeHotkey();
            }

            // F10: Cycle through RGBA channel combinations
            if (inputState.GetKeyUpState(Key_F10))
            {
                pDenoiser->CycleRGBAChannelsHotkey();
            }
        }
    }

    // Handle any changes that need to occur due to applicate resize
    // NOTE: Cauldron with auto-resize internal resources
    void Sample::DoSampleResize(const ResolutionInfo& resInfo)
    {}

    void Sample::DoSampleShutdown()
    {}
} // namespace rr
