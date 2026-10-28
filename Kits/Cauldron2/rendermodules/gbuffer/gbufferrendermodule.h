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

#include "../../framework/shaders/surfacerendercommon.h"

#include "../../framework/core/contentmanager.h"
#include "../../framework/render/rendermodule.h"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace cauldron
{
    class ParameterSet;
    class PipelineObject;
    class RenderTarget;
    class ResourceView;
    struct RootSignatureDesc;
    class RootSignature;
    class Texture;
    class Sampler;
    class RasterView;
    class Surface;
}  // namespace cauldron

/**
* @class GBufferRenderModule
*
* The GBuffer render module is responsible for rendering the gbuffer for all loaded scene entities.
* If initialized to do so, will also output motion vectors for the frame.
*
* @ingroup CauldronRender
*/
class GBufferRenderModule : public cauldron::RenderModule, public cauldron::ContentListener
{
public:

    /**
    * @brief   Construction.
    */
    GBufferRenderModule() : RenderModule(L"GBufferRenderModule"), ContentListener() {}

    /**
    * @brief   Destruction.
    */
    virtual ~GBufferRenderModule();

    /**
    * @brief   Initialization function. Sets up target pointers and other global data.
    */
    void Init(const json& initData) override;

    /**
    * @brief   Renders all active geometric entities in the <c><i>Scene</i></c>.
    */
    void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

    /**
    * @brief   Callback invoked when new content is loaded so we can create additional pipelines if needed.
    */
    void OnNewContentLoaded(cauldron::ContentBlock* pContentBlock) override;

    /**
    * @brief   Callback invoked when content is unloaded. Permits us to clean things up if needed.
    */
    void OnContentUnloaded(cauldron::ContentBlock* pContentBlock) override;

    /**
    * @brief   Predicate deciding whether a surface is rasterized into the GBuffer.
    *          Return true to include (render) the surface, false to skip it.
    */
    typedef std::function<bool(const cauldron::Surface*)> SurfaceInclusionCallback;

    /**
    * @brief   Installs a custom surface-inclusion predicate for the GBuffer pass.
    *
    * When set, this predicate REPLACES the default inclusion rule (a surface is rendered unless
    * cauldron::Surface::HasTranslucency() is true, i.e. unless its material blend mode is AlphaBlend).
    * Use it to make the GBuffer's notion of a "primary visible surface" agree with another pass. For
    * example a ray-traced primary that treats high-transmission glass as see-through can install a
    * predicate that also excludes those surfaces, so the raster depth / motion vectors describe the
    * surface BEHIND the glass instead of the glass itself.
    *
    * WHEN IT RUNS:
    *   - Invoked from OnNewContentLoaded(), once per cauldron::Surface of every mesh in a loaded
    *     ContentBlock — i.e. at scene/content LOAD time, NOT per frame.
    *   - Called while the module INSTANCE's internal content lock is held. Because this predicate is
    *     static and shared across instances, it must still be thread-safe and free of side effects.
    *     re-evaluated. Install it during sample init, before the scene is loaded. Passing nullptr
    *     restores the default HasTranslucency() behaviour for subsequently loaded content.
    *
    * NOTE: this is a single global/static predicate shared by all GBufferRenderModule instances,
    * mirroring the cauldron::CameraComponent::SetJitterCallbackFunc pattern.
    */
    static void SetSurfaceInclusionCallback(SurfaceInclusionCallback callback) { s_SurfaceInclusionCallback = std::move(callback); }

private:
    // No copy, No move
    NO_COPY(GBufferRenderModule)
    NO_MOVE(GBufferRenderModule)

    // Optional client override for GBuffer surface inclusion. Null => default HasTranslucency() rule.
    // See SetSurfaceInclusionCallback for semantics and when it is evaluated.
    static SurfaceInclusionCallback s_SurfaceInclusionCallback;

    // Content creation helpers - not thread safe
    uint32_t GetPipelinePermutationID(const cauldron::Surface* pSurface);  //uint32_t vertexAttributeFlags, const Material* pMaterial);
    int32_t  AddTexture(const cauldron::Material* pMaterial, const cauldron::TextureClass textureClass, int32_t& textureSamplerIndex);
    void RemoveTexture(int32_t index);

private:

    bool                            m_VariableShading               = false;
    bool                            m_GenerateMotionVectors         = false;
    cauldron::RootSignature*        m_pRootSignature                = nullptr;
    cauldron::ParameterSet*         m_pParameterSet                 = nullptr;
    const cauldron::Texture*        m_pAlbedoRenderTarget           = nullptr;
    const cauldron::Texture*        m_pNormalRenderTarget           = nullptr;
    const cauldron::Texture*        m_pAoRoughnessMetallicTarget    = nullptr;
    const cauldron::Texture*        m_pDepthTarget                  = nullptr;
    const cauldron::Texture*        m_pMotionVector                 = nullptr;
    std::vector<const cauldron::RasterView*> m_RasterViews          = {};

    struct BoundTexture
    {
        const cauldron::Texture* pTexture = nullptr;
        uint32_t       count    = 1;
    };
    std::vector<BoundTexture>       m_Textures;
    std::vector<cauldron::Sampler*> m_Samplers;
    std::mutex m_CriticalSection;

    struct PipelineSurfaceRenderInfo
    {
        const cauldron::Entity*  pOwner   = nullptr;
        const cauldron::Surface* pSurface = nullptr;
        TextureIndices           TextureIndices;
    };

    struct PipelineRenderGroup
    {
        cauldron::PipelineObject*               m_Pipeline       = nullptr;
        uint64_t                                m_PipelineHash   = 0;
        uint32_t                                m_UsedAttributes = 0;
        std::vector<PipelineSurfaceRenderInfo>  m_RenderSurfaces = {};
    };

    std::vector<PipelineRenderGroup>            m_PipelineRenderGroups;
};
