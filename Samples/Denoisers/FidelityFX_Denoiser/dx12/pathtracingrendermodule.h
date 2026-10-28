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

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

// rr::DenoiserRenderModule
#include "denoiserrendermodule.h"
// rr::PackedInstance
// rr::PackedMaterial
// rr::PackedSurface
#include "shaders/PackedData.h"
// rr::F32
// rr::S32
// rr::U32
#include "shaders/Types.h"

//------------------------------------------------------------------------------
// Cauldron Includes
//------------------------------------------------------------------------------

// cauldron::ContentListener
#include "Cauldron2/framework/core/contentmanager.h"
// json
#include "Cauldron2/framework/core/framework.h"
// cauldron::Material
// cauldron::TextureClass
#include "Cauldron2/framework/render/material.h"
// cauldron::RenderModule
#include "Cauldron2/framework/render/rendermodule.h"
// cauldron::Sampler
// cauldron::SamplerDesc
#include "Cauldron2/framework/render/sampler.h"

//------------------------------------------------------------------------------
// External Includes
//------------------------------------------------------------------------------

// std::mutex
#include <mutex>
// std::vector
#include <vector>

//------------------------------------------------------------------------------
// Cauldron Forward Declarations
//------------------------------------------------------------------------------

namespace cauldron
{
    class Material;
    class ParameterSet;
    class ResourceView;
    class RootSignature;
    class Texture;
    class UIRenderModule;

    struct SamplerDesc;
}  // namespace cauldron

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    //--------------------------------------------------------------------------
    // PathTracingRenderModule
    //--------------------------------------------------------------------------
    
    class PathTracingRenderModule final : public cauldron::RenderModule, public cauldron::ContentListener
    {

    public:

        //----------------------------------------------------------------------
        // Member Methods: Constructors and Destructors
        //----------------------------------------------------------------------

        [[nodiscard]]
        PathTracingRenderModule();

        ~PathTracingRenderModule() override;

        //----------------------------------------------------------------------
        // Member Methods
        //----------------------------------------------------------------------

        void Init(const json& initData);
        void EnableModule(bool enabled) override;

        /**
         * @brief   Setup parameters that the Denoiser context needs this frame and then call the FFX Dispatch.
         */
        void Execute(double deltaTime, cauldron::CommandList* pCmdList) override;

        /**
         * @brief   Build UI.
         */
        void BuildUI();

        /**
         * Prepare shading information for raytracing passes.
         */
        void OnNewContentLoaded(cauldron::ContentBlock* pContentBlock) override;
        /**
         * @copydoc ContentListener::OnContentUnloaded()
         */
        void OnContentUnloaded(cauldron::ContentBlock* pContentBlock) override;

    private:

        //----------------------------------------------------------------------
        // Member Methods
        //----------------------------------------------------------------------

        [[nodiscard]]
        bool InitPipelineObjects();

        [[nodiscard]]
        bool InitResources();

        [[nodiscard]]
        S32 AddTexture(const cauldron::Material* pMaterial, const cauldron::TextureClass textureClass, S32& textureSamplerIndex);
        
        void RemoveTexture(S32 index);

        //----------------------------------------------------------------------
        // Member Variables
        //----------------------------------------------------------------------

        struct RTInfoTables
        {
            struct BoundTexture
            {
                const cauldron::Texture* pTexture = {};
                U32 count    = 1;
            };

            std::vector< const cauldron::Buffer* > m_VertexBuffers;
            std::vector< const cauldron::Buffer* > m_IndexBuffers;
            std::vector< BoundTexture > m_Textures;
            std::vector< cauldron::Sampler* > m_Samplers;

            std::vector< PackedMaterial > m_cpuMaterialBuffer;
            std::vector< PackedInstance > m_cpuInstanceBuffer;
            std::vector< PackedSurface > m_cpuSurfaceBuffer;

            const cauldron::Buffer* m_pMaterialBuffer = {};  // PackedMaterial buffer
            const cauldron::Buffer* m_pSurfaceBuffer = {};  // PackedSurface buffer
            const cauldron::Buffer* m_pInstanceBuffer = {};  // PackedInstance buffer
        };
        RTInfoTables m_rtInfoTables;

        F32 m_AmbientOcclusionA  = 0.5f;
        F32 m_AmbientOcclusionB  = 0.1f;
        F32 m_SpecularOcclusionA = 0.5f;
        F32 m_SpecularOcclusionB = 0.1f;

        const cauldron::Texture* m_pColorTarget = {};

        cauldron::RootSignature* m_pTraceRaysDenoiserRootSignature = {};
        cauldron::PipelineObject* m_pTraceRaysDenoiserPipeline = {};
        cauldron::ParameterSet*m_pTraceRaysDenoiserParameterSet = {};

        const cauldron::Texture* m_pDirectSpecularOutput          = {};
        const cauldron::Texture* m_pDirectDiffuseOutput           = {};
        const cauldron::Texture* m_pIndirectSpecularOutput        = {};
        const cauldron::Texture* m_pIndirectDiffuseOutput         = {};
        const cauldron::Texture* m_pDominantLightVisibilityOutput = {};
        const cauldron::Texture* m_pAmbientOcclusionOutput        = {};
        const cauldron::Texture* m_pSpecularOcclusionOutput       = {};
        const cauldron::Texture* m_pNormals                       = {};
        const cauldron::Texture* m_pDiffuseAlbedo                 = {};
        const cauldron::Texture* m_pSpecularAlbedo                = {};
        const cauldron::Texture* m_pSkipSignal                    = {};

        const cauldron::Texture* m_pPrefilteredEnvironmentMap = {};
        const cauldron::Texture* m_pBRDFTexture = {};

        cauldron::SamplerDesc m_SpecularSampler;

        std::mutex m_Mutex;

        DenoiserRenderModule* m_pDenoiserRenderModule = {};
    };
} // namespace rr
