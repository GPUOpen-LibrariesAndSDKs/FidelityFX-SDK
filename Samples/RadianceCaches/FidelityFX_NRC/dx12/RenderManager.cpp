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

#include "RenderManager.h"

#include <windows.h>
#include <windowsx.h>
#include <string>

#include <OpenSource/imgui/backends/imgui_impl_dx12.h>
#include <OpenSource/imgui/backends/imgui_impl_win32.h>

#include "utils/Log.h"
#include "ShaderGraph.h"
#include "utils/Json.h"
#include "utils/StringUtils.h"
#include "utils/FilesystemUtils.h"
#include <OpenSource/nlohmann/json.hpp>

#include <FidelityFX/api/include/dx12/ffx_api_dx12.hpp>

namespace fsr
{
	// Creates an buffer with unordered access and creates a view for it on the specified heap
	template<typename Type>
	void CreateRadianceCacheBuffer(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList>& commandList, const size_t numElements, const wchar_t* idStr, ComPtr<ID3D12Resource>& d3dResource, CD3DX12_CPU_DESCRIPTOR_HANDLE& heap, FfxApiResource* ffxResource)
	{
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_RESOURCE_DESC resourceDesc = {};
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = numElements * sizeof(Type);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;		

		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numElements;
		if (std::is_aggregate<Type>::value)
		{
			uavDesc.Buffer.StructureByteStride = sizeof(Type);
		}
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		if (std::is_same_v<Type, uint32_t>) { uavDesc.Format = DXGI_FORMAT_R32_UINT; }
		else
		{
			Assert(std::is_aggregate<Type>::value);
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		}

		// Create the buffer
		ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&d3dResource)));

		if(idStr) { d3dResource->SetName(idStr); }

		//CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(d3dResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		//commandList->ResourceBarrier(1, &resBarrier);

		// Create the UAV
		static const UINT heapInc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);		
		device->CreateUnorderedAccessView(d3dResource.Get(), nullptr, &uavDesc, heap);
		heap.Offset(1, heapInc);

		if(ffxResource)
		{
			// Create an FFX handle to the resource
			*ffxResource = ffxApiGetResourceDX12(d3dResource.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ, 0);
			// FIXME: This shouldn't need to be set here. 
			ffxResource->description.stride = sizeof(Type);
		}
	}

	inline void TransitionBarrier(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12Resource>& d3dResource, const D3D12_RESOURCE_STATES beforeState, const D3D12_RESOURCE_STATES afterState)
	{
		CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(d3dResource.Get(), beforeState, afterState);
		commandList->ResourceBarrier(1, &resBarrier);
	}

	inline void UAVBarrier(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12Resource>& d3dResource)
	{
		CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::UAV(d3dResource.Get());
		commandList->ResourceBarrier(1, &resBarrier);
	}

	template<typename Type>
	void CreateReadbackBuffer(ComPtr<ID3D12Device> device, const size_t numElements, ComPtr<ID3D12Resource>& d3dResource)
	{
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_READBACK;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = numElements * sizeof(Type);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&d3dResource));
	}
	
	RenderManager::RenderManager(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, const int width, const int height, const Json& json, const std::string& deviceName) :
		m_device(device),
		m_frameIdx(0),
		m_width(width),
		m_height(height),
		m_trainingRatio(0.03f)
	{
		CreateRootSignatures();
		CreateQuad();
		InitializeRadianceCache(commandList, deviceName);

		// Create the accumulator texture
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		// Create the texture
		ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_SHARED, &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&m_accumTexture)));
		ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_SHARED, &textureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_renderBuffer)));
		m_accumTexture->SetName(L"accumTexture");
		m_renderBuffer->SetName(L"renderBuffer");

		// Create descriptor heaps
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		srvHeapDesc.NumDescriptors = 8;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_rendererDescHeap)));

		srvHeapDesc.NumDescriptors = 1;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pixelDescHeap)));

		// Renderer UAV
		CD3DX12_CPU_DESCRIPTOR_HANDLE heap;
		const UINT heapInc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		heap = m_rendererDescHeap->GetCPUDescriptorHandleForHeapStart();

		Log::Write(Pad(50, "Allocating buffers...") + "\b");

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = textureDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		device->CreateUnorderedAccessView(m_accumTexture.Get(), nullptr, &uavDesc, heap);
		heap.Offset(1, heapInc);
		device->CreateUnorderedAccessView(m_renderBuffer.Get(), nullptr, &uavDesc, heap);
		heap.Offset(1, heapInc);

		m_cacheDesc.commandList = commandList.Get();

		// Create the buffers that will store the data the cache will use
		CreateRadianceCacheBuffer<RadianceCacheInput>(m_device, commandList, m_maxInferenceSampleCount, L"cachePredictionQueries", m_cachePredictionQueries, heap, &m_cacheDesc.predictionInputs);
		CreateRadianceCacheBuffer<RadianceCacheOutput>(m_device, commandList, m_maxInferenceSampleCount, L"cachePredictionRadiance", m_cachePredictionRadiance, heap, &m_cacheDesc.predictionOutputs);
		CreateRadianceCacheBuffer<RadianceCacheInput>(m_device, commandList, m_maxTrainingSampleCount, L"cacheTrainingQueries", m_cacheTrainingQueries, heap, &m_cacheDesc.trainInputs);
		CreateRadianceCacheBuffer<RadianceCacheOutput>(m_device, commandList, m_maxTrainingSampleCount, L"cacheTrainingRadiance", m_cacheTrainingRadiance, heap, &m_cacheDesc.trainTargets);
		CreateRadianceCacheBuffer<uint32_t>(m_device, commandList, 2, L"cacheCounters", m_cacheCounters, heap, &m_cacheDesc.sampleCounters);
		CreateRadianceCacheBuffer<RadianceCachePixelData>(m_device, commandList, m_maxInferenceSampleCount, L"cacheRenderState", m_cacheRenderState, heap, nullptr);

		CreateReadbackBuffer<uint32_t>(m_device, 2, m_cacheCountersReadback);

		if (m_cacheCtx)
		{
			auto rv = ffx::Dispatch(m_cacheCtx, m_cacheDesc);
			// FIXME: Make sure API returns correct error code
			//AssertMsg(rv == ffx::ReturnCode::Ok, "FSR dispatch failed with code {}", int(rv));
			Log::Success("Okay!");
		}

		// Texture SRV
		heap = m_pixelDescHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(m_accumTexture.Get(), &srvDesc, heap);

		// Create the performance query heap 
		D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queryHeapDesc.Count = 5;
		device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&m_perfQueryHeap));

		// Create readback buffer for query results
		heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * queryHeapDesc.Count);
		device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_perfQueryData));

		// Create the perf stats		
		m_stats.layerPerf.emplace_back("Path tracer", 0.f);
		m_stats.layerPerf.emplace_back("Radiance cache", 0.f);
		m_stats.layerPerf.emplace_back("Composite", 0.f);
		m_stats.layerPerf.emplace_back("Display", 0.f);
		m_stats.bufferOccupancy[0] = { "Inference", 0, m_maxInferenceSampleCount };
		m_stats.bufferOccupancy[1] = { "Train", 0, m_maxTrainingSampleCount };

		// Resolve the path to the shader root directory
		const std::string shaderRootRelativePath = (*json)["/paths/shaderDirectory"_json_pointer];
		const std::string shaderRootAbsolutePath = MakeAbsolutePath(GetModuleDirectory() + "\\" + shaderRootRelativePath);
		AssertMsg(DirectoryExists(shaderRootAbsolutePath), "Invalid shader root directory '{}'", shaderRootAbsolutePath);
		Log::Debug("Shader root directory: {}", shaderRootAbsolutePath);

		const std::string sceneRelativePath = (*json)["/paths/sceneDirectory"_json_pointer];
		const std::string metaPath = shaderRootAbsolutePath + "\\" + sceneRelativePath + "\\meta.json";
		const std::string parentDir = GetParentDirectory(metaPath);
		Log::Write(Pad(50, ' ', "Loading scene '{}'...", NormalizePath(sceneRelativePath + "\\meta.json")) + "\b");
		Json metaJson(metaPath);
		Json filesNode = metaJson["files"];
		Assert(filesNode->contains("renderer") && filesNode->contains("display") && filesNode->contains("composite"));
		Log::Success("Okay!");

		// Initalise the shader graph
		Json dxcNode = metaJson["dxc"];
		m_shaderGraph.reset(new ShaderGraph(m_device, dxcNode, { shaderRootAbsolutePath + "\\common", parentDir }));

		m_renderShader = m_shaderGraph->CreateShader((*filesNode)["renderer"], m_rendererRootSig);
		m_compositeShader = m_shaderGraph->CreateShader((*filesNode)["composite"], m_rendererRootSig);
		m_displayShader = m_shaderGraph->CreateShader((*filesNode)["display"], m_pixelRootSig);

		// Finalizes and compiles the shaders
		m_shaderGraph->Finalise();

		// Initialise the frame context
		m_frameCtx.kViewportRes = XMUINT2(m_width, m_height);
		m_frameCtx.kMaxTrainSamples = m_maxTrainingSampleCount;
		m_frameCtx.kTrainingRatio = m_trainingRatio;
		m_frameCtx.kTime = 0;
	}

	void RenderManager::Destroy()
	{
		ReleaseRadianceCache();

		m_shaderGraph->Destroy();
		ReleaseResource(m_pixelRootSig);
		ReleaseResource(m_rendererRootSig);
		ReleaseResource(m_triangleVertexBuffer);
		ReleaseResource(m_perfQueryHeap);
		ReleaseResource(m_perfQueryData);
		ReleaseResource(m_accumTexture);
		ReleaseResource(m_renderBuffer);
		ReleaseResource(m_pixelDescHeap);
		ReleaseResource(m_rendererDescHeap);
	}

	void RenderManager::UpdateConfig(const RenderManagerConfig& newConfig)
	{
		Assert(newConfig.cache.weightSmoothing >= 0 && newConfig.cache.weightSmoothing <= 1);
		Assert(newConfig.cache.learningRate > 0);

		m_config = newConfig;
	}	

	void RenderManager::InitializeRadianceCache(ComPtr<ID3D12GraphicsCommandList>& commandList, const std::string& deviceName)
	{
		Log::Write(Pad(50, ' ', "Initializing... (this may take a while) ") + "\b");

		m_maxInferenceSampleCount = m_width * m_height;
		m_maxTrainingSampleCount = 16384 * 2;
		m_trainingRatio = float(m_maxTrainingSampleCount) / float(m_maxInferenceSampleCount);

		ffx::CreateBackendDX12Desc dx12BackendDesc = {};
		dx12BackendDesc.device = m_device.Get();

		ffx::CreateContextDescOverrideVersion versionOverride = {};

		ffx::CreateContextDescRadianceCache ctxDesc;
		ctxDesc.version = FFX_RADIANCECACHE_MAKE_VERSION(0, 9, 0);
		ctxDesc.maxInferenceSampleCount = m_maxInferenceSampleCount;
		ctxDesc.maxTrainingSampleCount = m_maxTrainingSampleCount;	
		ctxDesc.flags |= FFX_RADIANCE_CACHE_CONTEXT_TRY_FORCE_WMMA;

		// Test to see whether WMMA is supported by creating the context with the FFX_RADIANCE_CACHE_CONTEXT_TRY_FORCE_WMMA flag.
		// If it fails, we can try again with the fallback.
		ffx::ReturnCode ffxRv = ffx::CreateContext(m_cacheCtx, nullptr, ctxDesc, dx12BackendDesc, versionOverride);
		if(ffx::ReturnCode::Ok != ffxRv)
		{
			// If we're forced to use the using the reference backend, check whether this device meets the lane count required by the shaders.
			D3D12_FEATURE_DATA_D3D12_OPTIONS1 devOps = {};
			if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &devOps, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1))))
			{
				AssertFmt(devOps.WaveLaneCountMin <= 32 && devOps.WaveLaneCountMax >= 32,
						  "Device '{}' does not support the lane count of 32 required by the reference backend.", deviceName);
			}

			ctxDesc.flags &= ~FFX_RADIANCE_CACHE_CONTEXT_TRY_FORCE_WMMA;
			m_isWmmaEnabled = false;
			Log::Warning("Warning: WMMA is not supported on device '{}'. Using the reference backend.", deviceName);

			// Try to create the context again...
			AssertMsg(ffx::ReturnCode::Ok == ffx::CreateContext(m_cacheCtx, nullptr, ctxDesc, dx12BackendDesc, versionOverride), 
					 "FSR dispatch failed with code {}", int(ffxRv));
		}
		else
		{
			m_isWmmaEnabled = true;
		}

		Log::Success("Okay!");
	}

	void RenderManager::ReleaseRadianceCache()
	{
		ReleaseResource(m_cachePredictionQueries);
		ReleaseResource(m_cachePredictionRadiance);
		ReleaseResource(m_cacheTrainingQueries);
		ReleaseResource(m_cacheTrainingRadiance);
		ReleaseResource(m_cacheCounters);
		ReleaseResource(m_cacheRenderState);
		ReleaseResource(m_cacheCountersReadback);

		ffx::DestroyContext(m_cacheCtx);
	}

	void RenderManager::PrepareFrameCtx()
	{
		// Timings
		if (m_config.animate.enable) { m_frameCtx.kTime += m_wallTime.Get(); }
		m_wallTime.Reset();
		m_frameCtx.kFrameIdx = m_frameIdx;

		// Determine the position of the split-screen partition
		const float partition = m_config.display.demoMode ? (0.5f + 0.25f * std::sin(m_frameCtx.kTime)) : m_config.display.splitPartition;
		m_frameCtx.kSplitScreenPartitionX = int(m_width * partition);

		// Render settings
		m_frameCtx.kRenderFlags = 0;
		m_frameCtx.kRenderFlags |= uint32_t(m_config.animate.camera) * RENDER_ANIMATE_CAMERA;
		m_frameCtx.kRenderFlags |= uint32_t(m_config.animate.geometry) * RENDER_ANIMATE_GEOMETRY;
		m_frameCtx.kRenderFlags |= uint32_t(m_config.animate.lights) * RENDER_ANIMATE_LIGHTS;
		m_frameCtx.kRenderFlags |= uint32_t(m_config.animate.materials) * RENDER_ANIMATE_MATERIALS;
		m_frameCtx.kRenderFlags |= uint32_t(m_config.renderer.lockNoise) * RENDER_LOCK_NOISE;
		m_frameCtx.kAccumMotionBlur = m_config.renderer.accumBlur;
		m_frameCtx.kIndirectRoughening = m_config.renderer.indirectRoughening;
	}

	void RenderManager::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, CD3DX12_CPU_DESCRIPTOR_HANDLE& outputTarget)
	{
		PrepareFrameCtx();

		int queryIdx = 0;
		commandList->EndQuery(m_perfQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIdx++);

		ID3D12DescriptorHeap* pHeaps[] = { m_rendererDescHeap.Get(), m_pixelDescHeap.Get() };

		TransitionBarrier(commandList, m_accumTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->SetComputeRootSignature(m_rendererRootSig.Get());
		commandList->SetComputeRoot32BitConstants(1, sizeof(hlsl::FrameCtx) / sizeof(UINT), &m_frameCtx, 0);
		commandList->SetDescriptorHeaps(1, &pHeaps[0]);
		commandList->SetComputeRootDescriptorTable(0, m_rendererDescHeap->GetGPUDescriptorHandleForHeapStart());		
		
		// Render
		commandList->SetPipelineState(m_renderShader->GetPipelineState().Get());
		commandList->Dispatch((m_width + 7) / 8, (m_height + 7) / 8, 1);
		UAVBarrier(commandList, m_renderBuffer);
		commandList->EndQuery(m_perfQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIdx++);

		// Read back stats
		{
			ScopedTransitionBarrier barrier(commandList, m_cacheCounters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
			commandList->CopyResource(m_cacheCountersReadback.Get(), m_cacheCounters.Get());
		}

		// Radiance cache
		{
			ScopedTransitionBarrier barrier(commandList, { m_cacheTrainingQueries, m_cachePredictionRadiance, m_cacheTrainingRadiance, m_cacheCounters }, 
											D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			m_cacheDesc.commandList = commandList.Get();
			m_cacheDesc.overrides.learningRate = m_config.cache.learningRate;
			m_cacheDesc.overrides.weightSmoothing = m_config.cache.weightSmoothing;

			m_cacheDesc.flags = FFX_RADIANCE_CACHE_DISPATCH_INFERENCE | 
								   FFX_RADIANCE_CACHE_DISPATCH_TRAINING | 
								   FFX_RADIANCE_CACHE_OVERRIDE_LEARNING_RATE | 
								   FFX_RADIANCE_CACHE_OVERRIDE_WEIGHT_SMOOTHING | 
								   FFX_RADIANCE_CACHE_CLEAR_ALL_COUNTERS;
			if(m_config.resetCache) 
			{
				m_cacheDesc.flags |= FFX_RADIANCE_CACHE_RESET;
				m_config.resetCache = false;
			}

			auto rv = ffx::Dispatch(m_cacheCtx, m_cacheDesc);
			AssertMsg(rv == ffx::ReturnCode::Ok, "FSR dispatch failed with code 0x{:x}", uint32_t(rv));
			commandList->EndQuery(m_perfQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIdx++);
		}

		// Composite
		commandList->SetComputeRootSignature(m_rendererRootSig.Get());
		commandList->SetComputeRoot32BitConstants(1, sizeof(hlsl::FrameCtx) / sizeof(UINT), &m_frameCtx, 0);
		commandList->SetDescriptorHeaps(1, &pHeaps[0]);
		commandList->SetComputeRootDescriptorTable(0, m_rendererDescHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetPipelineState(m_compositeShader->GetPipelineState().Get());
		commandList->Dispatch((m_width + 7) / 8, (m_height + 7) / 8, 1);
		commandList->EndQuery(m_perfQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIdx++);
		
		UAVBarrier(commandList, m_accumTexture);
		TransitionBarrier(commandList, m_accumTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		commandList->SetGraphicsRootSignature(m_pixelRootSig.Get());
		commandList->SetGraphicsRoot32BitConstants(1, sizeof(hlsl::FrameCtx) / sizeof(UINT), &m_frameCtx, 0);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &m_triangleVertexBufferView);
		commandList->SetDescriptorHeaps(1, &pHeaps[1]);
		commandList->SetGraphicsRootDescriptorTable(0, m_pixelDescHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(1, &outputTarget, FALSE, nullptr);
		commandList->SetPipelineState(m_displayShader->GetPipelineState().Get());
		commandList->DrawInstanced(6, 1, 0, 0);
		commandList->EndQuery(m_perfQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIdx++);
		
		// Resolve perf query data to buffer
		commandList->ResolveQueryData(m_perfQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, queryIdx, m_perfQueryData.Get(), 0);

		//Log::Write("Dispatched display draw call.");
		//std::this_thread::sleep_for(std::chrono::milliseconds(500));

		++m_frameIdx;
	}

	inline D3D_ROOT_SIGNATURE_VERSION GetRootSigHighestVersion(ID3D12Device* device)
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			return D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		return featureData.HighestVersion;
	}
	
	void RenderManager::CreateRootSignatures()
	{
		// Standard texture sampler
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		auto Create = [&](CD3DX12_DESCRIPTOR_RANGE1* ranges, const int rangeSize, ComPtr<ID3D12RootSignature>& rootSignature, const wchar_t* idStr)
			{
				CD3DX12_ROOT_PARAMETER1 rootParameters[2];
				rootParameters[0].InitAsDescriptorTable(rangeSize, ranges, D3D12_SHADER_VISIBILITY_ALL);
				rootParameters[1].InitAsConstants(sizeof(hlsl::FrameCtx) / sizeof(UINT), 0);

				CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
				rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

				ComPtr<ID3DBlob> signature;
				ComPtr<ID3DBlob> error;
				ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, GetRootSigHighestVersion(m_device.Get()), &signature, &error));
				ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

				if (idStr)
				{
					rootSignature->SetName(idStr);
				};
			};

		CD3DX12_DESCRIPTOR_RANGE1 pixelRanges[1];
		pixelRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		Create(pixelRanges, 1, m_pixelRootSig, L"Pixel Root Signature");

		CD3DX12_DESCRIPTOR_RANGE1 computeRanges[1];
		const UINT numOfRenderAndCompositeShaderInputs = 8;
		computeRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numOfRenderAndCompositeShaderInputs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		Create(computeRanges, 1, m_rendererRootSig, L"Render Root Signature");
	}	

	void RenderManager::CreateQuad()
	{
		// Define the geometry for a triangle.
		VertexUV triangleVertices[] =
		{
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
			{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
			{ { 1.0f, -1.0f, 0.0f }, { 1.f, 0.0f } },
			{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.f } },
			{ { 1.0f, 1.0f, 0.0f }, { 1.f, 1.f } },
			{ { 1.0f, -1.0f, 0.0f }, { 1.f, 0.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_triangleVertexBuffer)));

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        
		ThrowIfFailed(m_triangleVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_triangleVertexBuffer->Unmap(0, nullptr);

		m_triangleVertexBufferView.BufferLocation = m_triangleVertexBuffer->GetGPUVirtualAddress();
		m_triangleVertexBufferView.StrideInBytes = sizeof(VertexUV);
		m_triangleVertexBufferView.SizeInBytes = vertexBufferSize;
	}

	const RenderManagerStats& RenderManager::GatherStats(ComPtr<ID3D12CommandQueue>& commandQueue)
	{
		// Perf stats
		UINT64* timestamps;
		UINT64 frequency;
		m_perfQueryData->Map(0, nullptr, reinterpret_cast<void**>(&timestamps));
		commandQueue->GetTimestampFrequency(&frequency);

		for (int idx = 0; idx < m_stats.layerPerf.size(); ++idx)
		{
			m_stats.layerPerf[idx].timeMs = float(1e3 * (timestamps[idx + 1] - timestamps[idx]) / double(frequency));
		}

		m_perfQueryData->Unmap(0, nullptr);

		// Occupancy
		uint32_t* mappedData;
		m_cacheCountersReadback->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		//std::memcpy(m_stats.m_bufferOccupancy.data(), mappedData, sizeof(uint32_t) * 3);
		for (int idx = 0; idx < m_stats.bufferOccupancy.size(); ++idx)
		{
			std::get<1>(m_stats.bufferOccupancy[idx]) = mappedData[idx];
		}

		m_cacheCountersReadback->Unmap(0, nullptr);

		return m_stats;
	}
}
