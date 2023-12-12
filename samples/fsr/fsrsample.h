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


#pragma once

#include "core/framework.h"

class FSR3RenderModule;
class FSR3UpscaleRenderModule;
class FSR2RenderModule;
class FSR1RenderModule;
class UpscaleRenderModule;
class TAARenderModule;
class TranslucencyRenderModule;
class cauldron::RenderModule;

/// @defgroup FfxFsrSample FidelityFX Super Resolution Sample
/// Sample documentation for FidelityFX Super Resolution
///
/// @ingroup SDKEffects

/// @defgroup FSRS FSRSample
/// FSRSample Reference Documentation
///
/// @ingroup FfxFsrSample
/// @{

/**
 * @class FSRSample
 *
 * FSRSample takes care of:
 *      - creating UI section that enable users to select upscaling options
 *      - creating GPU resources
 *      - dispatch workloads for simple bilinear and bicubic upsampling
 */
class FSRSample : public cauldron::Framework
{
public:
    FSRSample(const cauldron::FrameworkInitParams* pInitParams) : cauldron::Framework(pInitParams) {}
    virtual ~FSRSample() = default;

    // Overrides
    virtual void ParseSampleConfig() override;
    virtual void ParseSampleCmdLine(const wchar_t* cmdLine) override;
    virtual void RegisterSampleModules() override;

    virtual int32_t DoSampleInit() override;
    virtual void DoSampleUpdates(double deltaTime) override;
    //virtual void DoSampleResize(const cauldron::ResolutionInfo& resInfo) override  {}
    virtual void DoSampleShutdown() override;

private:

    enum class UpscaleMethod : uint32_t
    {
        Native = 0,
        Point,
        Bilinear,
        Bicubic,
        FSR1,
        FSR2,
        FSR3UPSCALEONLY,
        FSR3,

        Count
    };

    void SwitchUpscaler(UpscaleMethod newUpscaler);

    UpscaleMethod m_Method = UpscaleMethod::Native;
    UpscaleMethod m_UIMethod = UpscaleMethod::Native;
    cauldron::RenderModule* m_pCurrentUpscaler = nullptr;

    FSR3RenderModule*         m_pFSR3RenderModule = nullptr;
    FSR3UpscaleRenderModule*  m_pFSR3UpscaleRenderModule = nullptr;
    FSR2RenderModule*         m_pFSR2RenderModule = nullptr;
    FSR1RenderModule*         m_pFSR1RenderModule = nullptr;
    UpscaleRenderModule*      m_pUpscaleRenderModule = nullptr;
    TAARenderModule*          m_pTAARenderModule = nullptr;
    TranslucencyRenderModule* m_pTransRenderModule = nullptr;

/// @}
};

typedef FSRSample FrameworkType;
