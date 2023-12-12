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

#include "render/rendermodule.h"
#include "shaders/tonemapping/tonemappercommon.h"

namespace cauldron
{
    class ParameterSet;
    class PipelineObject;
    class RasterView;
    class ResourceView;
    class RootSignature;
    class RenderTarget;
    class Texture;
}  // namespace cauldron

class ToneMappingRenderModule : public cauldron::RenderModule
{
public:
    ToneMappingRenderModule();
    ToneMappingRenderModule(const wchar_t* pName);
    virtual ~ToneMappingRenderModule();

    virtual void Init(const json& initData) override;
    virtual void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

private:
    // No copy, No move
    NO_COPY(ToneMappingRenderModule)
    NO_MOVE(ToneMappingRenderModule)

private:
    // Constant data
    TonemapperCBData m_ConstantData;

    // common
    cauldron::RootSignature*    m_pRootSignature  = nullptr;
    const cauldron::RasterView* m_pRasterView     = nullptr;
    cauldron::PipelineObject*   m_pPipelineObj    = nullptr;
    cauldron::ParameterSet*     m_pParameters     = nullptr;

    const cauldron::Texture*    m_pTexture      = nullptr;
    const cauldron::Texture*    m_pRenderTarget   = nullptr;
};
