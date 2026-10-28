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

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

// Declarations
#include "debugoverlayrendermodule.h"
// rr::DenoiserRenderModule
#include "denoiserrendermodule.h"
// rr::DebugOverlayConstants
#include "shaders/DebugOverlayData.h"
// rr::F32
// rr::U32
#include "shaders/Types.h"

//------------------------------------------------------------------------------
// Cauldron Includes
//------------------------------------------------------------------------------

// cauldron::GetDynamicBufferPool
// cauldron::GetFramework
// json
#include "Cauldron2/framework/core/framework.h"
// cauldron::BufferAddressInfo
#include "Cauldron2/framework/render/buffer.h"
// cauldron::DynamicBufferPool
#include "Cauldron2/framework/render/dynamicbufferpool.h"
// cauldron::ParameterSet
#include "Cauldron2/framework/render/parameterset.h"
// cauldron::PipelineDesc
// cauldron::ShaderBuildDesc
#include "Cauldron2/framework/render/pipelinedesc.h"
// cauldron::PipelineObject
#include "Cauldron2/framework/render/pipelineobject.h"
// cauldron::ResolutionInfo
// cauldron::ShaderBindStage
// cauldron::ShaderModel
// cauldron::ViewDimension
#include "Cauldron2/framework/render/renderdefines.h"
// cauldron::RootSignature
#include "Cauldron2/framework/render/rootsignature.h"
// cauldron::RootSignatureDesc
#include "Cauldron2/framework/render/rootsignaturedesc.h"
// cauldron::SamplerDesc
#include "Cauldron2/framework/render/sampler.h"
// cauldron::Texture
#include "Cauldron2/framework/render/texture.h"

//------------------------------------------------------------------------------
// Sample Definitions
//------------------------------------------------------------------------------

using namespace cauldron;

namespace rr
{
    [[nodiscard]]
    DebugOverlayRenderModule::DebugOverlayRenderModule()
        : RenderModule(L"DebugOverlayRenderModule")
    {
    }

    DebugOverlayRenderModule::~DebugOverlayRenderModule() = default;

    void DebugOverlayRenderModule::Init(const json& initData)
    {
        m_pColorTarget = GetFramework()->GetColorTargetForCallback(GetName());

        m_pDenoiserRenderModule = static_cast< DenoiserRenderModule* >(GetFramework()->GetRenderModule("DenoiserRenderModule"));
        if (not m_pDenoiserRenderModule)
            return;

        m_pDenoiserDebugView = GetFramework()->GetRenderTexture(L"DenoiserDebugViewTarget");

        SamplerDesc linearSampler = {};

        RootSignatureDesc debugOverlayRootSignatureDesc;
        debugOverlayRootSignatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1);
        debugOverlayRootSignatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1); // Debug view
        debugOverlayRootSignatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1); // Output color
        debugOverlayRootSignatureDesc.AddStaticSamplers(0, ShaderBindStage::Compute, 1, &linearSampler);
        m_pDebugOverlayRootSignature = RootSignature::CreateRootSignature(L"DebugOverlayRootSignature", debugOverlayRootSignatureDesc);
        if (not m_pDebugOverlayRootSignature)
            return;

        PipelineDesc debugOverlayPipelineDesc;
        debugOverlayPipelineDesc.SetRootSignature(m_pDebugOverlayRootSignature);
        ShaderBuildDesc debugOverlayDesc = ShaderBuildDesc::Compute(L"DebugOverlay.cs.hlsl", L"Main", ShaderModel::SM6_6, nullptr);
        debugOverlayDesc.AdditionalParams = L"-HV 2021"; // rr headers use templates/namespaces/float32_t (overrides the framework's -HV 2018)
        debugOverlayPipelineDesc.AddShaderDesc(debugOverlayDesc);
        m_pDebugOverlayPipeline = PipelineObject::CreatePipelineObject(L"DebugOverlayPipeline", debugOverlayPipelineDesc);
        if (not m_pDebugOverlayPipeline)
            return;

        m_pDebugOverlayParameterSet = ParameterSet::CreateParameterSet(m_pDebugOverlayRootSignature);
        m_pDebugOverlayParameterSet->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(DebugOverlayConstants), 0);
        m_pDebugOverlayParameterSet->SetTextureSRV(m_pDenoiserDebugView, ViewDimension::Texture2D, 0);
        m_pDebugOverlayParameterSet->SetTextureUAV(m_pColorTarget, ViewDimension::Texture2D, 0);

        SetModuleEnabled(true);
        SetModuleReady(true);
    }

    void DebugOverlayRenderModule::Execute(double deltaTime, CommandList* pCmdList)
    {
        if (m_pDenoiserRenderModule and m_pDenoiserRenderModule->IsDebugViewEnabled())
        {
            const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

            const DebugOverlayConstants constants =
            {
                .outputTargetSize = { static_cast< F32 >(resInfo.DisplayWidth), static_cast< F32 >(resInfo.DisplayHeight) },
            };

            BufferAddressInfo constantsBufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(constants), &constants);
            m_pDebugOverlayParameterSet->UpdateRootConstantBuffer(&constantsBufferInfo, 0);

            m_pDebugOverlayParameterSet->Bind(pCmdList, m_pDebugOverlayPipeline);
            SetPipelineState(pCmdList, m_pDebugOverlayPipeline);
            const U32 threadGroupCountX = (resInfo.DisplayWidth + GROUP_THREAD_COUNT_X - 1) / GROUP_THREAD_COUNT_X;
            const U32 threadGroupCountY = (resInfo.DisplayHeight + GROUP_THREAD_COUNT_Y - 1) / GROUP_THREAD_COUNT_Y;
            Dispatch(pCmdList, threadGroupCountX, threadGroupCountY, 1);
        }
    }
} // namespace rr
