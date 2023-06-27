// AMD Cauldron code
// 
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include "core/framework.h"

#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers
#include <windows.h>

namespace cauldron
{
    struct FrameworkInitParamsInternal
    {
        HINSTANCE InstanceHandle;
        int       CmdShow;
    };

    enum PresentationMode
    {
        PRESENTATIONMODE_WINDOWED,
        PRESENTATIONMODE_BORDERLESS_FULLSCREEN
    };

    class FrameworkInternal : public FrameworkImpl
    {
    public:
        FrameworkInternal(Framework* pFramework, const FrameworkInitParams* pInitParams);
        virtual ~FrameworkInternal() = default;

        virtual int32_t Init() override;
        virtual int32_t Run() override;

        virtual int32_t PreRun() override { return 0; }
        virtual void PostRun() {}
        virtual void Shutdown() {}

        const HWND GetHWND() const { return m_WindowHandle; }
        const PresentationMode GetPresentationMode() const { return m_PresentationMode; }

    private:
        FrameworkInternal() = delete;

        static LRESULT CALLBACK WindowProc(HWND wndHandle, UINT message, WPARAM wParam, LPARAM lParam);
        void ToggleFullscreen();
        void OnWindowMove();
        void OnResize(uint32_t width, uint32_t height);

        // Internal members
        HWND             m_WindowHandle = 0;
        HMONITOR         m_Monitor = 0;
        HINSTANCE        m_InstanceHandle;
        int              m_CmdShow = 0;
        RECT             m_WindowRect = {};
        UINT             m_WindowStyle = 0;
        PresentationMode m_PresentationMode = PRESENTATIONMODE_WINDOWED;
        bool             m_sendResizeEvent = false;
        bool             m_Minimized = false;
    };

} // namespace cauldron
