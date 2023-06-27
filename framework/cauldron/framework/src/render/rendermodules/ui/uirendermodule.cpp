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

#include "render/rendermodules/ui/uirendermodule.h"
#include "core/framework.h"
#include "core/uimanager.h"
#include "core/inputmanager.h"

#include "render/color_conversion.h"
#include "render/dynamicresourcepool.h"
#include "render/parameterset.h"
#include "render/pipelineobject.h"
#include "render/profiler.h"
#include "render/rasterview.h"
#include "render/rootsignature.h"
#include "render/swapchain.h"
#include "render/texture.h"
#include "shaders/shadercommon.h"

#include "imgui/imgui.h"

namespace cauldron
{
    constexpr uint32_t g_NumThreadX = 8;
    constexpr uint32_t g_NumThreadY = 8;

    void UIRenderModule::Init(const json& initData)
    {
        //////////////////////////////////////////////////////////////////////////
        // Register UI elements for magnifier
        static constexpr float s_MAGNIFICATION_AMOUNT_MIN = 1.0f;
        static constexpr float s_MAGNIFICATION_AMOUNT_MAX = 32.0f;
        static constexpr float s_MAGNIFIER_RADIUS_MIN = 0.01f;
        static constexpr float s_MAGNIFIER_RADIUS_MAX = 0.85f;

        UISection uiSection;
        uiSection.SectionName = "Magnifier";
        uiSection.AddCheckBox("Show Magnifier (M or Middle Mouse Button)", &m_MagnifierEnabled);
        uiSection.AddCheckBox("Lock Position (L)", &m_LockMagnifierPosition, nullptr, &m_MagnifierEnabled);
        uiSection.AddFloatSlider("Screen Size", &m_MagnifierCBData.MagnifierScreenRadius, s_MAGNIFIER_RADIUS_MIN, s_MAGNIFIER_RADIUS_MAX, nullptr, &m_MagnifierEnabled);
        uiSection.AddFloatSlider("Magnification", &m_MagnifierCBData.MagnificationAmount, s_MAGNIFICATION_AMOUNT_MIN, s_MAGNIFICATION_AMOUNT_MAX, nullptr, &m_MagnifierEnabled);
        GetUIManager()->RegisterUIElements(uiSection);

        //////////////////////////////////////////////////////////////////////////
        // Render Target

        m_pRenderTarget = GetFramework()->GetColorTargetForCallback(GetName());
        CauldronAssert(ASSERT_CRITICAL, m_pRenderTarget, L"Couldn't get the swapchain render target when initializing UIRenderModule.");
        m_pUIRasterView = GetRasterViewAllocator()->RequestRasterView(m_pRenderTarget, ViewDimension::Texture2D);

        // Temp intermediate RT for magnifier render pass
        TextureDesc           desc    = m_pRenderTarget->GetDesc();
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        desc.Width                    = resInfo.DisplayWidth;
        desc.Height                   = resInfo.DisplayHeight;
        desc.Name                     = L"Magnifier_Intermediate_Color";
        m_pRenderTargetTemp           = GetDynamicResourcePool()->CreateRenderTexture(
            &desc, [](TextureDesc& desc, uint32_t displayWidth, uint32_t displayHeight, uint32_t renderingWidth, uint32_t renderingHeight) {
                desc.Width  = displayWidth;
                desc.Height = displayHeight;
            });

        //////////////////////////////////////////////////////////////////////////
        // Create sampler - both magnifier and UI can use a point sampler
        SamplerDesc samplerDesc;
        samplerDesc.Filter = FilterFunc::MinMagMipPoint;
        samplerDesc.AddressU = AddressMode::Border;
        samplerDesc.AddressV = AddressMode::Border;
        samplerDesc.AddressW = AddressMode::Border;
        std::vector<SamplerDesc> samplers;
        samplers.push_back(samplerDesc);

        //////////////////////////////////////////////////////////////////////////
        // Root signatures

        // UI
        RootSignatureDesc uiSignatureDesc;
        uiSignatureDesc.AddConstantBufferView(0, ShaderBindStage::Vertex, 1);
        uiSignatureDesc.AddConstantBufferView(1, ShaderBindStage::Pixel, 1);
        uiSignatureDesc.AddTextureSRVSet(0, ShaderBindStage::Pixel, 1);
        uiSignatureDesc.AddStaticSamplers(0, ShaderBindStage::Pixel, (uint32_t)samplers.size(), samplers.data());

        m_pUIRootSignature = RootSignature::CreateRootSignature(L"UIRenderModule_UIRootSignature", uiSignatureDesc);

        // Magnifier
        RootSignatureDesc magSignatureDesc;
        magSignatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1); // Upscaler
        magSignatureDesc.AddConstantBufferView(1, ShaderBindStage::Compute, 1); // MagnifierCB
        magSignatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);       // ColorTarget
        magSignatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);       // ColorTargetTemp

        m_pMagnifierRootSignature = RootSignature::CreateRootSignature(L"UIRenderModule_MagnifierRootSignature", magSignatureDesc);

        //////////////////////////////////////////////////////////////////////////
        // Setup the pipeline objects (one each for LDR/HDR modes)

        //----------------------------------------------------
        // UI
        PipelineDesc uiPsoDesc;
        uiPsoDesc.SetRootSignature(m_pUIRootSignature);

        // Setup the shaders to build on the pipeline object
        DefineList defineList;
        defineList.insert(std::make_pair(L"NUM_THREAD_X", std::to_wstring(g_NumThreadX)));
        defineList.insert(std::make_pair(L"NUM_THREAD_Y", std::to_wstring(g_NumThreadY)));
        uiPsoDesc.AddShaderDesc(ShaderBuildDesc::Vertex(L"ui.hlsl", L"uiVS", ShaderModel::SM6_0, &defineList));
        uiPsoDesc.AddShaderDesc(ShaderBuildDesc::Pixel(L"ui.hlsl", L"uiPS", ShaderModel::SM6_0, &defineList));

        // Setup vertex elements
        std::vector<InputLayoutDesc> vertexAttributes;
        vertexAttributes.push_back(InputLayoutDesc(VertexAttributeType::Position, ResourceFormat::RG32_FLOAT, 0, 0));
        vertexAttributes.push_back(InputLayoutDesc(VertexAttributeType::Texcoord0, ResourceFormat::RG32_FLOAT, 0, 8));
        vertexAttributes.push_back(InputLayoutDesc(VertexAttributeType::Color0, ResourceFormat::RGBA8_UNORM, 0, 16));
        uiPsoDesc.AddInputLayout(vertexAttributes);

        // Setup blend and depth states
        BlendDesc blendDesc = { true, Blend::SrcAlpha, Blend::InvSrcAlpha, BlendOp::Add, Blend::One, Blend::InvSrcAlpha, BlendOp::Add, static_cast<uint32_t>(ColorWriteMask::All) };
        std::vector<BlendDesc> blends;
        blends.push_back(blendDesc);
        uiPsoDesc.AddBlendStates(blends, false, false);
        DepthDesc depthDesc;
        uiPsoDesc.AddDepthState(&depthDesc);  // Use defaults

        RasterDesc rasterDesc;
        rasterDesc.CullingMode = CullMode::None;
        uiPsoDesc.AddRasterStateDescription(&rasterDesc);

        // Setup remaining information and build
        uiPsoDesc.AddPrimitiveTopology(PrimitiveTopologyType::Triangle);

        uiPsoDesc.AddRasterFormats(m_pRenderTarget->GetFormat());
        m_pUIPipelineObj = PipelineObject::CreatePipelineObject(L"UI_PipelineObj", uiPsoDesc);

        //----------------------------------------------------
        // Magnifier
        PipelineDesc magPsoDesc;
        magPsoDesc.SetRootSignature(m_pMagnifierRootSignature);

        // Setup the shaders to build on the pipeline object
        magPsoDesc.AddShaderDesc(ShaderBuildDesc::Compute(L"ui.hlsl", L"MagnifierCS", ShaderModel::SM6_6, &defineList));

        m_pMagnifierPipelineObj = PipelineObject::CreatePipelineObject(L"Magnifier_PipelineObj", magPsoDesc);

        //////////////////////////////////////////////////////////////////////////
        // Setup parameter sets

        // UI
        m_pUIParameters = ParameterSet::CreateParameterSet(m_pUIRootSignature);
        m_pUIParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(UIVertexBufferConstants), 0);
        m_pUIParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(HDRCBData), 1);

        // Magnifier
        m_pMagnifierParameters = ParameterSet::CreateParameterSet(m_pMagnifierRootSignature);
        m_pMagnifierParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(UpscalerInformation), 0);
        m_pMagnifierParameters->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(MagnifierCBData), 1);
        m_pMagnifierParameters->SetTextureSRV(m_pRenderTarget, ViewDimension::Texture2D, 0);
        m_pMagnifierParameters->SetTextureUAV(m_pRenderTargetTemp, ViewDimension::Texture2D, 0);
    }

    UIRenderModule::~UIRenderModule()
    {
        delete m_pUIRootSignature;
        delete m_pMagnifierRootSignature;

        delete m_pUIPipelineObj;
        delete m_pMagnifierPipelineObj;

        delete m_pUIParameters;
        delete m_pMagnifierParameters;
    }

    void UIRenderModule::Execute(double deltaTime, CommandList* pCmdList)
    {
        GPUScopedProfileCapture UIMarker(pCmdList, L"UI");

        // If upscaler is enabled, we NEED to have hit the upscaler by now!
        CauldronAssert(ASSERT_WARNING,
                       GetFramework()->GetUpscalingState() != UpscalerState::PreUpscale,
                       L"Upscale state is still PreUpscale when reaching UIRendermodule. This should not be the case.");

        // Do input updates for magnifier
        const InputState& inputState = GetInputManager()->GetInputState();
        if (inputState.GetKeyUpState(Key_M) || inputState.GetMouseButtonUpState(Mouse_MButton))
            m_MagnifierEnabled = !m_MagnifierEnabled;

        if (inputState.GetKeyUpState(Key_L))
            m_LockMagnifierPosition = !m_LockMagnifierPosition;

        // And fetch all the data ImGUI pushed this frame for UI rendering
        ImDrawData* pUIDrawData = ImGui::GetDrawData();

        // If there's nothing to do, return
        if (!m_MagnifierEnabled && !pUIDrawData)
            return;

        uint32_t rtWidth, rtHeight;
        float    widthScale, heightScale;
        GetFramework()->GetUpscaledRenderInfo(rtWidth, rtHeight, widthScale, heightScale);
        UpscalerInformation upscaleConst = {Vec4((float)rtWidth, (float)rtHeight, widthScale, heightScale)};


        // Check if enabled
        if (m_MagnifierEnabled)
        {
            // Setup constant buffer data and update magnifier to stay centered on screen
            UpdateMagnifierParams();
            BufferAddressInfo bufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(MagnifierCBData), &m_MagnifierCBData);
            BufferAddressInfo upscaleInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(UpscalerInformation), &upscaleConst);

            // Render modules expect resources coming in/going out to be in a shader read state
            {
                Barrier barrier;
                barrier = Barrier::Transition(
                    m_pRenderTargetTemp->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::UnorderedAccess);
                ResourceBarrier(pCmdList, 1, &barrier);
            }

            // Bind everything
            m_pMagnifierParameters->UpdateRootConstantBuffer(&upscaleInfo, 0);
            m_pMagnifierParameters->UpdateRootConstantBuffer(&bufferInfo, 1);
            m_pMagnifierParameters->Bind(pCmdList, m_pMagnifierPipelineObj);

            // Set pipeline and draw
            SetPipelineState(pCmdList, m_pMagnifierPipelineObj);

            const uint32_t numGroupX = DivideRoundingUp(m_pRenderTarget->GetDesc().Width, g_NumThreadX);
            const uint32_t numGroupY = DivideRoundingUp(m_pRenderTarget->GetDesc().Height, g_NumThreadY);
            Dispatch(pCmdList, numGroupX, numGroupY, 1);

            // Copy intermediate target into original render target
            {
                std::vector<Barrier> barriers;

                barriers.push_back(Barrier::Transition(m_pRenderTargetTemp->GetResource(), ResourceState::UnorderedAccess, ResourceState::CopySource));
                barriers.push_back(Barrier::Transition(
                    m_pRenderTarget->GetResource(), ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource, ResourceState::CopyDest));
                ResourceBarrier(pCmdList, static_cast<uint32_t>(barriers.size()), barriers.data());

                TextureCopyDesc desc(m_pRenderTargetTemp->GetResource(), m_pRenderTarget->GetResource());
                CopyTextureRegion(pCmdList, &desc);

                barriers[0] = Barrier::Transition(
                    m_pRenderTargetTemp->GetResource(), ResourceState::CopySource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
                ResourceBarrier(pCmdList, 1, barriers.data());
            }

            // If we aren't drawing UI data, then end raster and transition the render target back to read
            if (!pUIDrawData)
            {
                {
                    Barrier barrier;
                    barrier = Barrier::Transition(m_pRenderTarget->GetResource(),
                                                  ResourceState::CopyDest,
                                                  ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
                    ResourceBarrier(pCmdList, 1, &barrier);
                }
            }
        }

        if (pUIDrawData)
        {
            // UI RM always done at display resolution
            const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
            SetViewportScissorRect(pCmdList, 0, 0, resInfo.DisplayWidth, resInfo.DisplayHeight, 0.f, 1.f);
            SetPrimitiveTopology(pCmdList, PrimitiveTopology::TriangleList);

            // Pick the right pipeline / raster set / render target
            PipelineObject* pPipelineObj = m_pUIPipelineObj;
            const RasterView* pRasterView = m_pUIRasterView;
            const Texture* pRenderTarget = m_pRenderTarget;

            // If we didn't render magnifier, transition the resources from a different state
            {
                // Render modules expect resources coming in/going out to be in a shader read state
                Barrier rtBarrier = Barrier::Transition(pRenderTarget->GetResource(),
                    m_MagnifierEnabled ? ResourceState::CopyDest : ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource,
                    ResourceState::RenderTargetResource);
                ResourceBarrier(pCmdList, 1, &rtBarrier);
            }

            // Set the render targets
            BeginRaster(pCmdList, 1, &pRasterView);

            // Get buffers for vertices and indices
            char* pVertices = nullptr;
            BufferAddressInfo vertBufferInfo = GetDynamicBufferPool()->AllocVertexBuffer(pUIDrawData->TotalVtxCount, sizeof(ImDrawVert), reinterpret_cast<void**>(&pVertices));

            char* pIndices = nullptr;
            BufferAddressInfo idxBufferInfo = GetDynamicBufferPool()->AllocIndexBuffer(pUIDrawData->TotalIdxCount, sizeof(ImDrawIdx), reinterpret_cast<void**>(&pIndices));

            // Copy data in
            ImDrawVert* pVtx = (ImDrawVert*)pVertices;
            ImDrawIdx* pIdx = (ImDrawIdx*)pIndices;
            for (int32_t n = 0; n < pUIDrawData->CmdListsCount; n++)
            {
                const ImDrawList* pIMCmdList = pUIDrawData->CmdLists[n];
                memcpy(pVtx, pIMCmdList->VtxBuffer.Data, pIMCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(pIdx, pIMCmdList->IdxBuffer.Data, pIMCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                pVtx += pIMCmdList->VtxBuffer.Size;
                pIdx += pIMCmdList->IdxBuffer.Size;
            }

            // Setup constant buffer data
            float left = 0.0f;
            float right = ImGui::GetIO().DisplaySize.x;
            float bottom = ImGui::GetIO().DisplaySize.y;
            float top = 0.0f;
            Mat4 mvp(Vec4(2.f / (right - left), 0.f, 0.f, 0.f),
                     Vec4(0.f, 2.f / (top - bottom), 0.f, 0.f),
                     Vec4(0.f, 0.f, 0.5f, 0.f),
                     Vec4((right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.f));

            BufferAddressInfo bufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(Mat4), &mvp);

            m_HDRCBData.MonitorDisplayMode  = GetFramework()->GetSwapChain()->GetSwapChainDisplayMode();
            m_HDRCBData.DisplayMaxLuminance = GetFramework()->GetSwapChain()->GetHDRMetaData().MaxLuminance;

            // Scene dependent
            ColorSpace inputColorSpace = ColorSpace_REC709;

            // Display mode dependent
            // Both FSHDR_2084 and HDR10_2084 take rec2020 value
            // Difference is FSHDR needs to be gamut mapped using monitor primaries and then gamut converted to rec2020
            ColorSpace outputColorSpace = ColorSpace_REC2020;
            SetupGamutMapperMatrices(inputColorSpace, outputColorSpace, &m_HDRCBData.ContentToMonitorRecMatrix);

            BufferAddressInfo hdrbufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(HDRCBData), &m_HDRCBData);

            // Update constant buffer
            m_pUIParameters->UpdateRootConstantBuffer(&bufferInfo, 0);
            m_pUIParameters->UpdateRootConstantBuffer(&hdrbufferInfo, 1);

            // Bind all parameters
            m_pUIParameters->Bind(pCmdList, pPipelineObj);

            // Set pipeline and draw
            SetPipelineState(pCmdList, pPipelineObj);

            // Render command lists
            uint32_t vtxOffset = 0;
            uint32_t idxOffset = 0;
            for (int32_t n = 0; n < pUIDrawData->CmdListsCount; n++)
            {
                // Set vertex info
                SetVertexBuffers(pCmdList, 0, 1, &vertBufferInfo);

                const ImDrawList* pIMCmdList = pUIDrawData->CmdLists[n];
                for (int32_t cmdIndex = 0; cmdIndex < pIMCmdList->CmdBuffer.Size; cmdIndex++)
                {
                    // Set index info
                    SetIndexBuffer(pCmdList, &idxBufferInfo);

                    const ImDrawCmd* pDrawCmd = &pIMCmdList->CmdBuffer[cmdIndex];
                    if (pDrawCmd->UserCallback)
                        pDrawCmd->UserCallback(pIMCmdList, pDrawCmd);

                    else
                    {
                        // Project scissor/clipping rectangles into framebuffer space
                        ImVec2 clipOffset = pUIDrawData->DisplayPos;
                        ImVec2 clipMin(pDrawCmd->ClipRect.x - clipOffset.x, pDrawCmd->ClipRect.y - clipOffset.y);
                        ImVec2 clipMax(pDrawCmd->ClipRect.z - clipOffset.x, pDrawCmd->ClipRect.w - clipOffset.y);
                        if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                            continue;
                        Rect scissorRect = {static_cast<uint32_t>(clipMin.x),
                                            static_cast<uint32_t>(clipMin.y),
                                            static_cast<uint32_t>(clipMax.x),
                                            static_cast<uint32_t>(clipMax.y)};
                        SetScissorRects(pCmdList, 1, &scissorRect);
                        DrawIndexedInstanced(pCmdList, pDrawCmd->ElemCount, 1, idxOffset, vtxOffset);
                    }

                    idxOffset += pDrawCmd->ElemCount;
                }
                vtxOffset += pIMCmdList->VtxBuffer.Size;
            }

            // Done rendering to render target
            EndRaster(pCmdList);

            // Render modules expect resources coming in/going out to be in a shader read state
            Barrier rtBarrier = Barrier::Transition(pRenderTarget->GetResource(), ResourceState::RenderTargetResource, ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource);
            ResourceBarrier(pCmdList, 1, &rtBarrier);
        }
    }

    void UIRenderModule::SetFontResourceTexture(const Texture* pFontTexture)
    {
        // Bind the font texture to the parameter set
        m_pUIParameters->SetTextureSRV(pFontTexture, ViewDimension::Texture2D, 0);

        // We are now ready for use
        SetModuleReady(true);
    }

    void UIRenderModule::UpdateMagnifierParams()
    {
        // If we are locked, use locked position for mouse pos
        if (m_LockMagnifierPosition)
        {
            m_MagnifierCBData.MousePosX = m_LockedMagnifierPositionX;
            m_MagnifierCBData.MousePosY = m_LockedMagnifierPositionY;
        }
        // Otherwise poll input state, and update the last lock position
        else
        {
            const InputState& inputState = GetInputManager()->GetInputState();
            m_MagnifierCBData.MousePosX = static_cast<int32_t>(inputState.Mouse.AxisState[Mouse_XAxis]);
            m_MagnifierCBData.MousePosY = static_cast<int32_t>(inputState.Mouse.AxisState[Mouse_YAxis]);
            m_LockedMagnifierPositionX = m_MagnifierCBData.MousePosX;
            m_LockedMagnifierPositionY = m_MagnifierCBData.MousePosY;
        }

        // UI RM always uses display resolution
        const ResolutionInfo& resInfo    = GetFramework()->GetResolutionInfo();
        m_MagnifierCBData.ImageWidth     = resInfo.DisplayWidth;
        m_MagnifierCBData.ImageHeight    = resInfo.DisplayHeight;
        m_MagnifierCBData.BorderColorRGB = m_LockMagnifierPosition ? Vec4(0.72f, 0.002f, 0.0f, 1.f) : Vec4(0.002f, 0.72f, 0.0f, 1.f);

        const int32_t imageSize[2] = { static_cast<int>(m_MagnifierCBData.ImageWidth), static_cast<int>(m_MagnifierCBData.ImageHeight) };
        const int32_t& width = imageSize[0];
        const int32_t& height = imageSize[1];

        const int32_t radiusInPixelsMagifier = static_cast<int>(m_MagnifierCBData.MagnifierScreenRadius * height);
        const int32_t radiusInPixelsMagifiedArea = static_cast<int>(m_MagnifierCBData.MagnifierScreenRadius * height / m_MagnifierCBData.MagnificationAmount);

        const bool bCirclesAreOverlapping = radiusInPixelsMagifiedArea + radiusInPixelsMagifier > std::sqrt(m_MagnifierCBData.MagnifierOffsetX * m_MagnifierCBData.MagnifierOffsetX + m_MagnifierCBData.MagnifierOffsetY * m_MagnifierCBData.MagnifierOffsetY);

        if (bCirclesAreOverlapping) // Don't let the two circles overlap
        {
            m_MagnifierCBData.MagnifierOffsetX = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
            m_MagnifierCBData.MagnifierOffsetY = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
        }

        // Try to move the magnified area to be fully on screen, if possible
        const int32_t* pMousePos[2] = { &m_MagnifierCBData.MousePosX, &m_MagnifierCBData.MousePosY };
        int32_t* pMagnifierOffset[2] = { &m_MagnifierCBData.MagnifierOffsetX, &m_MagnifierCBData.MagnifierOffsetY };
        for (int32_t i = 0; i < 2; ++i)
        {
            const bool bMagnifierOutOfScreenRegion = *pMousePos[i] + *pMagnifierOffset[i] + radiusInPixelsMagifier > imageSize[i] ||
                *pMousePos[i] + *pMagnifierOffset[i] - radiusInPixelsMagifier < 0;
            if (bMagnifierOutOfScreenRegion)
            {
                if (!(*pMousePos[i] - *pMagnifierOffset[i] + radiusInPixelsMagifier > imageSize[i]
                    || *pMousePos[i] - *pMagnifierOffset[i] - radiusInPixelsMagifier < 0))
                {
                    // Flip offset if possible
                    *pMagnifierOffset[i] = -*pMagnifierOffset[i];
                }
                else
                {
                    // Otherwise clamp
                    if (*pMousePos[i] + *pMagnifierOffset[i] + radiusInPixelsMagifier > imageSize[i])
                        *pMagnifierOffset[i] = imageSize[i] - *pMousePos[i] - radiusInPixelsMagifier;
                    if (*pMousePos[i] + *pMagnifierOffset[i] - radiusInPixelsMagifier < 0)
                        *pMagnifierOffset[i] = -*pMousePos[i] + radiusInPixelsMagifier;
                }
            }
        }
    }

} // namespace cauldron
