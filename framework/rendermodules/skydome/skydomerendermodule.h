// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "render/rendermodule.h"
#include "shaders/skydomecommon.h"
#include "core/components/lightcomponent.h"

#include <memory>
#include <vector>

namespace cauldron
{
    // forward declaration
    class ParameterSet;
    class PipelineObject;
    class RasterView;
    class RenderTarget;
    class ResourceView;
    class RootSignature;
    class Texture;
}  // namespace cauldron

/**
* @class SkyDomeRenderModule
*
* The sky dome render module is responsible for rendering the set ibl map to background or to generate a procedural sky.
*
* @ingroup CauldronRender
*/
class SkyDomeRenderModule : public cauldron::RenderModule
{
public:

    /**
    * @brief   Construction.
    */
    SkyDomeRenderModule() : RenderModule(L"SkyDomeRenderModule") {}

    /**
    * @brief   Destruction.
    */
    virtual ~SkyDomeRenderModule();

    /**
    * @brief   Initialization function. Sets up resource pointers, pipeline objects, root signatures, and parameter sets.
    */
    void Init(const json& initData) override;

    /**
    * @brief   Renders the corresponding skydome (lookup or generated) if enabled.
    */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

private:
    // No copy, No move
    NO_COPY(SkyDomeRenderModule)
    NO_MOVE(SkyDomeRenderModule)

private:
    void InitSkyDome();
    void InitProcedural();
    void InitSunlight();

    void UpdateSunDirection();

    // Callback for texture loading so we can mark ourselves "ready"
    void TextureLoadComplete(const std::vector<const cauldron::Texture*>& textureList, void*);

    // Skydome
    SkydomeCBData  m_ConstantData;
    const cauldron::Texture* m_pSkyTexture = nullptr;

    // procedural skydome
    ProceduralCBData    m_ProceduralConstantData;
    // sunlight
    cauldron::LightComponentData m_SunlightCompData = {};
    cauldron::LightComponent*    m_pSunlightComponent = nullptr;
    cauldron::Entity*            m_pSunlight          = nullptr;

    // common
    bool                        m_IsProcedural   = false;
    cauldron::RootSignature*    m_pRootSignature = nullptr;
    cauldron::PipelineObject*   m_pPipelineObj   = nullptr;
    cauldron::ParameterSet*     m_pParameters    = nullptr;

    const cauldron::Texture*    m_pRenderTarget  = nullptr;
};
