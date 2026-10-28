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

#include "Win32.h"
#include "D3DPipeline.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <iostream>

#include <OpenSource/imgui/imgui.h>
#include <OpenSource/imgui/backends/imgui_impl_win32.h>

#include "utils/Log.h"
#include "ShaderGraph.h"
#include "utils/Json.h"
#include <OpenSource/nlohmann/json.hpp>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace fsr
{
	void Win32::InitialiseIMGUI(HWND hWnd)
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(hWnd);
	}

	void Win32::DestroyIMGUI()
	{
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	int Win32::Run(HINSTANCE hInstance, int nCmdShow)
	{	
		// Load the configuration file
		Json configJson("config.json");	

		Assert(configJson.Contains("window"));
		auto windowJson = configJson["window"];
		Assert(windowJson.Contains("dimensions") && (*windowJson)["dimensions"].is_array() && (*windowJson)["dimensions"].size() == 2);
		const int kStartupWidth = (*windowJson)["dimensions"][0];
		const int kStartupHeight = (*windowJson)["dimensions"][1];
		Log::Debug("Window dimensions: {} x {}", kStartupWidth, kStartupHeight);
		
		// Parse the command line parameters
		/*int argc;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		pSample->ParseCommandLineArgs(argv, argc);
		LocalFree(argv);*/

		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = L"fsr-radiance-cache";
		RegisterClassEx(&windowClass);

		RECT windowRect = { 0, 0, kStartupWidth, kStartupHeight };		
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		D3DPipeline d3dPipe("FSR Radiance Cache");

		// Create the window and store a handle to it.
		GetHwnd() = CreateWindow(
			windowClass.lpszClassName,
			L"",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,		// We have no parent window.
			nullptr,		// We aren't using menus.
			hInstance,
			&d3dPipe);

		InitialiseIMGUI(GetHwnd());

		// Option to display the renderer window to a custom monitor
		if (windowJson.Contains("startupMonitor"))
		{
			// Retrieve information about the monitors
			std::vector<MONITORINFO> monitorInfo;
			EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC unnamedParam2, LPRECT unnamedParam3, LPARAM lParam) -> BOOL
				{
					MONITORINFO info = { sizeof(MONITORINFO) };
					if (GetMonitorInfoA(hMonitor, &info))
					{
						reinterpret_cast<std::vector<MONITORINFO>*>(lParam)->push_back(info);
					}
					return TRUE;
				},
				LPARAM(&monitorInfo));

			// Move the window
			const int monitorIdx = (*windowJson)["startupMonitor"];
			if (monitorIdx >= monitorInfo.size())
			{
				Log::Warning("Warning: monitor index {} must be between 0 and {}.", monitorIdx, monitorInfo.size());
			}
			else
			{
				SetWindowPos(GetHwnd(), NULL, monitorInfo[monitorIdx].rcWork.left + 100, monitorInfo[monitorIdx].rcWork.top + 100, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
		}

		ShowWindow(GetHwnd(), nCmdShow);

		// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
		d3dPipe.OnCreate(GetHwnd(), configJson);

		// Main sample loop.
		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			// Process any messages in the queue.
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		d3dPipe.OnDestroy();

		// Return this part of the WM_QUIT message to Windows.
		return static_cast<char>(msg.wParam);
	}

	// Main message handler for the sample.
	LRESULT CALLBACK Win32::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) { return 1; }

		try
		{
			D3DPipeline* d3dPipe = reinterpret_cast<D3DPipeline*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			// Only process UI messages if the container object has been successfully created
			if (d3dPipe)
			{
				switch (message)
				{

				case WM_KEYDOWN:	d3dPipe->OnKey(wParam, false, true); return 0;
				case WM_KEYUP:		d3dPipe->OnKey(wParam, false, false); return 0;
				case WM_SYSKEYDOWN:	d3dPipe->OnKey(wParam, true, true); return 0;
				case WM_SYSKEYUP:	d3dPipe->OnKey(wParam, true, false); return 0;

				case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: d3dPipe->OnMouseButton(VK_LBUTTON, true); return 0;
				case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: d3dPipe->OnMouseButton(VK_RBUTTON, true); return 0;
				case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: d3dPipe->OnMouseButton(VK_MBUTTON, true); return 0;
				case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: d3dPipe->OnMouseButton(VK_XBUTTON1, true); return 0;

				case WM_LBUTTONUP: d3dPipe->OnMouseButton(VK_LBUTTON, false); return 0;
				case WM_RBUTTONUP: d3dPipe->OnMouseButton(VK_RBUTTON, false); return 0;
				case WM_MBUTTONUP: d3dPipe->OnMouseButton(VK_MBUTTON, false); return 0;
				case WM_XBUTTONUP: d3dPipe->OnMouseButton(VK_XBUTTON1, false); return 0;

				case WM_MOUSEMOVE: d3dPipe->OnMouseMove(int(GET_X_LPARAM(lParam)), int(GET_Y_LPARAM(lParam)), wParam); return 0;

				case WM_MOUSEWHEEL: d3dPipe->OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
					return 0;

				case WM_PAINT:
					//Timer frameTimer;
					d3dPipe->OnUpdate();
					d3dPipe->OnRender();

					//SetWindowText(hWnd, tfm::format("%.2f FPS", 1.0f / frameTimer.Get()).c_str());
					return 0;

				case WM_SIZE:
					d3dPipe->OnClientResize(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), wParam);
					return 0;

				case WM_SETFOCUS:
				case WM_KILLFOCUS:
					d3dPipe->OnFocusChange(hWnd, message == WM_SETFOCUS);
					return 0;
				}
			}

			// Critical window messages
			switch (message)
			{
			case WM_CREATE:
			{
				// Save the DXSample* passed in to CreateWindow.
				LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
			}
			return 0;

			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			case WM_CLOSE:
				DestroyWindow(hWnd);
				return 0;
			}
		}
		catch (const std::runtime_error& err)
		{
			Log::Error("Runtime error: {}", err.what());
		}
		catch (...)
		{
			Log::Error("Unhandled exception.");
		}

		// Handle any messages the switch statement didn't.
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// In your main function or initialization code:
	AllocConsole();

	// Get the console output handle
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// Enable Virtual Terminal Processing
	DWORD dwMode = 0;
	GetConsoleMode(hConsole, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hConsole, dwMode);
	
	// Redirect stdout, stdin, stderr to console
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
	
	// Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
	std::ios::sync_with_stdio(true);
	
	// Optional: Set console title
	SetConsoleTitle(L"Debug Console");
	
	using namespace fsr;	

	// Initialise any objects that have global scope. 
	//InitialiseGlobalObjects();
	
	Log::EnableLevel(kLogSystem, false);
	Log::EnableLevel(kLogDebug, true);
	int rValue = 0;

	// For some reason, MSVC's debugger won't catch certain handled exceptions (possibly something to do with CUDA?)
//#define DISABLE_EXCEPTION_HANDLING
#ifndef DISABLE_EXCEPTION_HANDLING
	try
	{
#endif
		rValue = Win32::Run(GetModuleHandle(NULL), SW_SHOW);

#ifndef DISABLE_EXCEPTION_HANDLING
	}
	catch (const std::runtime_error& err)
	{
		Log::Error("Runtime error: {}\n", err.what());
		rValue = -1;
	}
	catch (...)
	{
		Log::Error("Unhandled error");
		rValue = -1;
	}
#endif

	if(rValue != 0)
	{
		std::cout << "Press any key to continue...\n";
		std::string str; std::getline(std::cin, str);
	}

	FreeConsole();
	return rValue;
}
