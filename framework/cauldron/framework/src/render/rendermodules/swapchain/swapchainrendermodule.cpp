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

#include "core/framework.h"
#include "render/rendermodules/swapchain/swapchainrendermodule.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/profiler.h"
#include "render/rasterview.h"
#include "render/rootsignature.h"
#include "render/swapchain.h"
#include "render/texture.h"

namespace cauldron
{
    void SwapChainRenderModule::Init(const json& initData)
    {
        // Setup the texture to be the swapchain render target input
        switch (GetFramework()->GetSwapChain()->GetSwapChainDisplayMode())
        {
            case DisplayMode::DISPLAYMODE_LDR:
                m_pTexture = GetFramework()->GetRenderTexture(L"LDR8Color");
                break;
            case DisplayMode::DISPLAYMODE_HDR10_2084:
            case DisplayMode::DISPLAYMODE_FSHDR_2084:
                m_pTexture = GetFramework()->GetRenderTexture(L"HDR10Color");
                break;
            case DisplayMode::DISPLAYMODE_HDR10_SCRGB:
            case DisplayMode::DISPLAYMODE_FSHDR_SCRGB:
                m_pTexture = GetFramework()->GetRenderTexture(L"HDR16Color");
                break;
        }

        // root signature
        RootSignatureDesc signatureDesc;
        signatureDesc.AddConstantBufferView(0, ShaderBindStage::Pixel, 1);
        signatureDesc.AddTextureSRVSet(0, ShaderBindStage::Pixel, 1);

        m_pRootSignature = RootSignature::CreateRootSignature(L"SampleRenderPass_RootSignature", signatureDesc);

        m_pRenderTarget = GetFramework()->GetSwapChain()->GetBackBufferRT();
        CauldronAssert(ASSERT_CRITICAL, m_pRenderTarget != nullptr, L"Couldn't get the swapchain render target when initializing SwapChainRenderModule.");

        CauldronAssert(ASSERT_ERROR, m_pRenderTarget->GetDesc().Width == m_pTexture->GetDesc().Width && m_pRenderTarget->GetDesc().Height == m_pTexture->GetDesc().Height, L"Final Render Resource and SwapChain width does not match.");

        // Get a raster view
        m_pRasterView = GetRasterViewAllocator()->RequestRasterView(m_pRenderTarget, ViewDimension::Texture2D);

        // Setup the pipeline object
        PipelineDesc psoDesc;
        psoDesc.SetRootSignature(m_pRootSignature);

        // Setup the shaders to build on the pipeline object
        psoDesc.AddShaderDesc(ShaderBuildDesc::Vertex(L"fullscreen.hlsl", L"FullscreenVS", ShaderModel::SM6_0, nullptr));
        psoDesc.AddShaderDesc(ShaderBuildDesc::Pixel(L"copytexture.hlsl", L"CopyTextureToSwapChainPS", ShaderModel::SM6_0, nullptr));

        // Setup remaining information and build
        psoDesc.AddPrimitiveTopology(PrimitiveTopologyType::Triangle);
        psoDesc.AddRasterFormats(m_pRenderTarget->GetFormat());

        m_pPipelineObj = PipelineObject::CreatePipelineObject(L"SwapChainCopyPass_PipelineObj", psoDesc);

        m_pParameters = ParameterSet::CreateParameterSet(m_pRootSignature);

        // Set our texture to the right parameter slot
        m_pParameters->SetTextureSRV(m_pTexture, ViewDimension::Texture2D, 0);

        // We are now ready for use
        SetModuleReady(true);
    }

    SwapChainRenderModule::~SwapChainRenderModule()
    {
        delete m_pRootSignature;
        delete m_pPipelineObj;
        delete m_pParameters;
    }

    void SwapChainRenderModule::Execute(double deltaTime, CommandList* pCmdList)
    {
        GPUScopedProfileCapture SwapchainMarker(pCmdList, L"SwapChain");

        // Cauldron resources need to be transitioned app-side to avoid confusion in states internally
        // Render modules expect resources coming in/going out to be in a shader read state
        Barrier barrier = Barrier::Transition(m_pRenderTarget->GetCurrentResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::RenderTargetResource);
        ResourceBarrier(pCmdList, 1, &barrier);

        BeginRaster(pCmdList, 1, &m_pRasterView);

        // Bind all the parameters
        m_pParameters->Bind(pCmdList, m_pPipelineObj);

        // Swap chain RM always done at display res
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        SetViewportScissorRect(pCmdList, 0, 0, resInfo.DisplayWidth, resInfo.DisplayHeight, 0.f, 1.f);

        // Set pipeline and draw
        SetPrimitiveTopology(pCmdList, PrimitiveTopology::TriangleList);
        SetPipelineState(pCmdList, m_pPipelineObj);

        DrawInstanced(pCmdList, 3);

        EndRaster(pCmdList);

        // Render modules expect resources coming in/going out to be in a shader read state
        barrier = Barrier::Transition(m_pRenderTarget->GetCurrentResource(), ResourceState::RenderTargetResource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
        ResourceBarrier(pCmdList, 1, &barrier);
    }

} // namespace cauldron
