// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
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
#include <cauldron2/dx12/framework/core/win/framework_win.h>
typedef Sample FrameworkType;

using namespace cauldron;

static FrameworkInitParamsInternal s_WindowsParams;
//////////////////////////////////////////////////////////////////////////
// WinMain
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Create the sample and kick it off to the framework to run
    wchar_t sampleName[] = L"FidelityFX FSR FFXAPI v1.3";
    FrameworkInitParams initParams  = { };
    initParams.Name                 = sampleName;
    initParams.CmdLine              = lpCmdLine;
    initParams.AdditionalParams     = &s_WindowsParams;

    // Setup the windows info
    s_WindowsParams.InstanceHandle       = hInstance;
    s_WindowsParams.CmdShow              = nCmdShow;

    FrameworkType frameworkInstance(&initParams);
    return RunFramework(&frameworkInstance);
}
