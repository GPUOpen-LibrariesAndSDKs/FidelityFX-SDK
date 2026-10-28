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

#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <filesystem>
#include <string>
#include "utils/Json.h"
#include "DXC.h"

namespace fsr
{	
	class LiveShader
	{
	public:
		enum StatusFlags : int { kUnloaded, kActive, kEnqueued };
		enum ShaderType : int { kUndefined, kCompute, kPixel };

		LiveShader(const std::string& filePath, DXC& dxcCtx, ComPtr<ID3D12Device> device, ComPtr<ID3D12RootSignature> rootSignature);

		//inline operator bool() const { return m_shaderState != kUnloaded;  }
		virtual void Compile();
		virtual void Destroy();

		inline bool IsDirty() { return m_isDirty && m_shaderState != kEnqueued; }
		void MakeDirty() { m_isDirty = true; } 
		inline bool IsCompiled() const { return m_shaderState != kUnloaded; }
		void OnUpdateAsyc();
		ComPtr<ID3D12PipelineState>& GetPipelineState(); 

	protected:
		virtual int CompileImpl() = 0;

	protected:
		ComPtr<ID3D12PipelineState>		m_activePipeState;
		ComPtr<ID3D12PipelineState>		m_enqueuedPipeState;

		ComPtr<ID3D12Device>			m_device;
		ComPtr<ID3D12RootSignature>		m_rootSignature;
		DXGI_FORMAT   					m_rtvFormat;

		std::atomic<int>				m_shaderState;
		std::string						m_filePath;
		bool							m_isDirty;
		bool							m_isCompiled;

		DXC&							m_dxcCtx;
	};

	class LivePixelShader : public LiveShader
	{
	public:
		LivePixelShader(const std::string& filePath, const DXGI_FORMAT rtvFormat, DXC& dxcCtx, ComPtr<ID3D12Device> device, ComPtr<ID3D12RootSignature> rootSignature);

		virtual int CompileImpl() override final;

	private:
		DXGI_FORMAT						m_rtvFormat;
	};

	class LiveComputeShader : public LiveShader
	{
	public:
		LiveComputeShader(const std::string& filePath, DXC& dxcCtx, ComPtr<ID3D12Device> device, ComPtr<ID3D12RootSignature> rootSignature);

		virtual int CompileImpl() override final;

	private:
	};

	struct ShaderFile
	{		
		std::unordered_map<std::string, std::weak_ptr<LiveShader>> m_deps;
		std::string m_filePath;
		std::filesystem::file_time_type m_prevWriteTime;
	};
	
	class ShaderGraph
	{
		struct CreateCtx
		{
			std::shared_ptr<LiveShader> shader;
			ComPtr<ID3D12RootSignature> rootSignature;
		};
	public:
		ShaderGraph(ComPtr<ID3D12Device> device, Json& json, const std::vector<std::string>& includeDirs = {});

		std::shared_ptr<LiveShader> CreateShader(const std::string& path, ComPtr<ID3D12RootSignature> rootSignature);
		void Finalise();
		void Destroy();

	private:
		void MonitorDependencies();
		void ParseShaderRecurse(const std::string& path, const std::string& rootPath, const int depth, CreateCtx& ctx);		
		bool ResolveShaderPath(const std::string& path, const std::string& rootPath, std::string& resolvedPath) const;

	private:
		enum Attrs : int { kThreadStopped, kThreadRunning, kThreadShutdown };

		std::thread m_thread;
		std::atomic<int> m_threadStatus;
		std::vector<std::string> m_fileIncludeDirs;
		std::vector<std::shared_ptr<LiveShader>> m_shaders;
		std::unordered_map<std::string, ShaderFile> m_shaderFiles;

		ComPtr<ID3D12Device>		m_device;
		DXC							m_dxcCtx;
	};
}
