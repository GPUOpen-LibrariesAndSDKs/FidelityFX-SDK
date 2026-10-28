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

#pragma once

#include "D3DBase.h"

//#include "modules/ModuleManager.cuh"
//#include "ui/UIModuleManager.h"

#include "ShaderGraph.h"
#include "RenderManager.h"
#include "utils/HighResTimer.h"

#include <vector>
#include <memory>

using namespace DirectX;

namespace fsr
{
	class ShaderGraph;
	class LiveShader;	

	static const char* shaderstrs =
		" struct PSInput \n" \
		" { \n" \
		"  float4 position : SV_POSITION; \n" \
		"  float4 color : COLOR; \n" \
		" } \n" \
		" PSInput VSMain(float3 position : POSITION, float4 color : COLOR) \n" \
		" { \n" \
		"  PSInput result;\n" \
		"  result.position = float4(position, 1.0f);\n" \
		"  result.color = color;\n"	\
		"  return result; \n" \
		" } \n" \
		" float4 PSMain(PSInput input) : SV_TARGETk \n" \
		" { \n" \
		"  return input.color;\n" \
		" } \n";

	class D3DPipeline
	{
	public:
		D3DPipeline(std::string name);

		void OnCreate(HWND hWnd, const Json& json);
		void OnRender();
		void OnDestroy();
		void OnUpdate();

		void OnClientResize(HWND hWnd, UINT width, UINT height, WPARAM wParam);
		void OnFocusChange(HWND hWnd, bool isSet);

		void OnKey(const WPARAM code, const bool isSysKey, const bool isDown);
		void OnMouseButton(const int button, const bool isDown);
		void OnMouseMove(const int mouseX, const int mouseY, const WPARAM flags);
		void OnMouseWheel(const float degrees);

		UINT GetClientWidth() const { return m_clientWidth; }
		UINT GetClientHeight() const { return m_clientHeight; }
		const CHAR* GetTitle() const { return "Probegen"; }

	private:

		void CreateDevice();
		void DestroyDevice();

		void CreateRenderTargets();
		void DestroyRenderTargets();

		void CreateSwapChain();
		void DestroySwapChain();

		void CreateAssets();
		void DestroyAssets();

		void CreateSynchronisationObjects();
		void DestroySynchronisationObjects();

		void CreateImguiObjects(ComPtr<ID3D12RootSignature>& rootSignature, ComPtr<ID3D12Device>& device, const int numConcurrentFrames);
		void PopulateImguiCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, const int frameIdx);
		void DestroyImguiObjects();

		void UpdateAssetDimensions();

		static const UINT				kFrameCount = 2;
		static const UINT				kTexturePixelSize = 16;    // The number of bytes used to represent a pixel in the texture.
		std::string						shadersSrc = shaderstrs;

		std::unique_ptr<RenderManager>	m_renderManager;
		RenderManagerConfig				m_renderConfig;

		RenderManagerStats				m_stats;
		std::unique_ptr<ShaderGraph>    m_shaderGraph;
		std::shared_ptr<LiveShader>	    m_liveShader;

		// Pipeline objects.
		D3D12_VIEWPORT					m_viewport;
		ComPtr<IDXGIFactory4>			m_factory;
		ComPtr<IDXGISwapChain3>			m_swapChain;
		ComPtr<ID3D12Device>			m_device;
		ComPtr<ID3D12Resource>			m_renderTargets[kFrameCount];
		ComPtr<ID3D12CommandAllocator>	m_commandAllocators[kFrameCount];
		ComPtr<ID3D12CommandQueue>		m_commandQueue;
		ComPtr<ID3D12RootSignature>		m_rootSignature;
		ComPtr<ID3D12DescriptorHeap>	m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap>	m_srvHeap;
		ComPtr<ID3D12DescriptorHeap>	m_imguiSrvHeap;
		ComPtr<ID3D12PipelineState>		m_pipelineState;
		ComPtr<ID3D12PipelineState>		m_trianglePipelineState;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;
		ComPtr<ID3D12InfoQueue1>		m_infoQueue;
		D3D12_RECT						m_scissorRect;
		UINT							m_rtvDescriptorSize;
		LUID							m_dx12deviceluid;
		std::string						m_deviceName;

		// App resources.
		//ComPtr<ID3D12Resource>			m_vertexBuffer;
		//D3D12_VERTEX_BUFFER_VIEW		m_vertexBufferView;


		// Synchronization objects.
		UINT							m_backBufferIdx;
		HANDLE							m_fenceEvent;
		ComPtr<ID3D12Fence>				m_fence;
		UINT64							m_fenceValues[kFrameCount];
		DWORD							m_callbackCookie;

		HWND							m_hWnd;

		UINT							m_quadTexWidth;
		UINT							m_quadTexHeight;

		UINT							m_clientWidth;
		UINT							m_clientHeight; 
		
		struct
		{
			int								idx;
			HighResTimer					timer;
			float							sumDeltaT;
			float							sumN;
		} 
		m_frame;

	private:
		void							CreatePipeline();
		void							InitCuda();
		void							LoadAssets();
		void							PopulateCommandList();
		void							MoveToNextFrame();
		void							WaitForGpu();
		void							CreateRootSignature();
		void							OnRenderGUI();

		void							GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);
	};
}
