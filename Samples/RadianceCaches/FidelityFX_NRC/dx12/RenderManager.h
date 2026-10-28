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

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include "ShaderGraph.h"
#include "utils/HighResTimer.h"
#include "utils/Json.h"

#include <FidelityFX/api/include/dx12/ffx_api_dx12.hpp>
#include <FidelityFX/radiancecache/include/ffx_radiancecache.hpp>

using namespace DirectX;

#include "shaders/common/FrameCtx.hlsl"

namespace fsr
{	
	// This would have to be solved eventually...
	struct RadianceCacheInput
	{
		float position[3];
		float normal[2];
		float viewDir[2];
		float diffuseAlbedo[3];
		float  roughness;
	};

	struct RadianceCacheOutput
	{
		float radiance[3];
	};

	struct RadianceCachePixelData
	{
		float weight[3];
	};
	
	struct RenderLayerPerfStats
	{
		std::string layerId;
		float timeMs;
	};

	struct RenderManagerStats
	{
		std::vector<RenderLayerPerfStats>					layerPerf;
		std::array<std::tuple<std::string, uint32_t, uint32_t>, 2>		bufferOccupancy;
	};

	struct RenderManagerConfig
	{	
		bool resetCache = false;

		struct
		{
			float learningRate = 0.002f;
			float weightSmoothing = 0.99f;
		} 
		cache;

		struct
		{
			float accumBlur = 0.7;
			float indirectRoughening = 0.5;
			bool lockNoise = false;
		} 
		renderer;

		struct
		{
			bool enable = true;
			bool materials = true;
			bool geometry = true;
			bool lights = true;
			bool camera = true;
		}
		animate;

		struct
		{
			bool demoMode = false;
			float splitPartition = 0.5;
		}
		display;
	};
	
	class RenderManager
	{
	public:
		RenderManager(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, const int width, const int height, const Json& json, const std::string& adapterName);

		void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, CD3DX12_CPU_DESCRIPTOR_HANDLE& outputTarget);
		void Destroy();
		const RenderManagerStats& GatherStats(ComPtr<ID3D12CommandQueue>& commandQueue);
		const RenderManagerConfig& GetConfig() const { return m_config; }
		void UpdateConfig(const RenderManagerConfig&);
		const bool IsWMMAEnabled() const { return m_isWmmaEnabled; }

	private:
		void CreateRootSignatures();
		void CreateQuad();
		void InitializeRadianceCache(ComPtr<ID3D12GraphicsCommandList>& commandList, const std::string& deviceName);
		void ReleaseRadianceCache();
		void PrepareFrameCtx();

	private:
		ComPtr<ID3D12Device>			m_device;
		ComPtr<ID3D12RootSignature>		m_pixelRootSig;
		ComPtr<ID3D12RootSignature>		m_rendererRootSig;
		ComPtr<ID3D12Resource>			m_triangleVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW		m_triangleVertexBufferView;
		ComPtr<ID3D12QueryHeap>			m_perfQueryHeap;
		ComPtr<ID3D12Resource>			m_perfQueryData;
		ComPtr<ID3D12Resource>			m_renderBuffer;
		ComPtr<ID3D12Resource>			m_accumTexture;

		RenderManagerStats				m_stats;

		ComPtr<ID3D12DescriptorHeap>	m_rendererDescHeap;
		ComPtr<ID3D12DescriptorHeap>	m_pixelDescHeap;

		std::unique_ptr<ShaderGraph>    m_shaderGraph;
		std::shared_ptr<LiveShader>	    m_renderShader;
		std::shared_ptr<LiveShader>	    m_displayShader;	
		std::shared_ptr<LiveShader>	    m_compositeShader;
		
		ffx::Context					m_cacheCtx = nullptr;
		ComPtr<ID3D12Resource>			m_cachePredictionQueries;
		ComPtr<ID3D12Resource>			m_cachePredictionRadiance;
		ComPtr<ID3D12Resource>			m_cacheTrainingQueries;
		ComPtr<ID3D12Resource>			m_cacheTrainingRadiance;
		ComPtr<ID3D12Resource>			m_cacheCounters;
		ComPtr<ID3D12Resource>			m_cacheRenderState;
		ComPtr<ID3D12Resource>			m_cacheCountersReadback;
		ffx::DispatchDescRadianceCache  m_cacheDesc;

		uint32_t						m_width;
		uint32_t						m_height;
		HighResTimer					m_wallTime;

		int								m_frameIdx;		
		hlsl::FrameCtx					m_frameCtx;

		uint32_t						m_maxInferenceSampleCount;
		uint32_t						m_maxTrainingSampleCount;
		float							m_trainingRatio;

		RenderManagerConfig				m_config;
		bool							m_isWmmaEnabled = false;

	};
}
