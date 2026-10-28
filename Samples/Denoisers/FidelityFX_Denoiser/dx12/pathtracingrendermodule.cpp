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
#include "pathtracingrendermodule.h"
// rr::CreatePackedFrame
// rr::DenoiserRenderModule
#include "denoiserrendermodule.h"
// rr::InvalidIndex
// rr::Map
// rr::MaterialFlag_DoubleSided
// rr::MaterialFlag_Opaque
// rr::PackedFrame
// rr::PackedInstance
// rr::PackedMaterial
// rr::PackedSurface
#include "shaders/PackedData.h"
// IBV_BEGIN_SLOT
// MAX_IBV_COUNT
// MAX_SAMPLER_COUNT
// MAX_TEXTURE_COUNT
// MAX_VBV_COUNT
// PACKED_INSTANCE_SLOT
// PACKED_MATERIAL_SLOT
// PACKED_SURFACE_SLOT
// SAMPLER_BEGIN_SLOT
// TEXTURE_BEGIN_SLOT
// VBV_BEGIN_SLOT
#include "shaders/PathTracingData.h"
// rr::F32
// rr::S32
// rr::U32
#include "shaders/Types.h"

//------------------------------------------------------------------------------
// Cauldron Includes
//------------------------------------------------------------------------------

// cauldron::Component
#include "Cauldron2/framework/core/component.h"
// cauldron::LightType
#include "Cauldron2/framework/core/components/lightcomponent.h"
// cauldron::MeshComponent
// cauldron::MeshComponentMgr
#include "Cauldron2/framework/core/components/meshcomponent.h"
// cauldron::ContentBlock
// cauldron::EntityDataBlock
#include "Cauldron2/framework/core/contentloader.h"
// cauldron::CauldronConfig
// cauldron::GetConfig
// cauldron::GetContentManager
// cauldron::GetDynamicBufferPool
// cauldron::GetDynamicResourcePool
// cauldron::GetFramework
// cauldron::GetScene
// cauldron::GetUIManager
// json
#include "Cauldron2/framework/core/framework.h"
// cauldron::IBLTexture
// cauldron::Scene
#include "Cauldron2/framework/core/scene.h"
// cauldron::UISection
// cauldron::UISectionType
// cauldron::UISlider
#include "Cauldron2/framework/core/uimanager.h"
// cauldron::ASSERT_CRITICAL
// cauldron::ASSERT_WARNING
// cauldron::CauldronAssert
// cauldron::CauldronError
#include "Cauldron2/framework/misc/assert.h"
// cauldron::Buffer
// cauldron::BufferAddressInfo
// cauldron::BufferDesc
#include "Cauldron2/framework/render/buffer.h"
// cauldron::DynamicBufferPool
#include "Cauldron2/framework/render/dynamicbufferpool.h"
// cauldron::DynamicResourcePool
#include "Cauldron2/framework/render/dynamicresourcepool.h"
// cauldron::Material
// cauldron::MaterialBlend
// cauldron::TextureClass
// cauldron::TextureInfo
#include "Cauldron2/framework/render/material.h"
// cauldron::Mesh
// cauldron::Surface
#include "Cauldron2/framework/render/mesh.h"
// cauldron::ParameterSet
#include "Cauldron2/framework/render/parameterset.h"
// cauldron::PipelineDesc
// cauldron::ShaderBuildDesc
#include "Cauldron2/framework/render/pipelinedesc.h"
// cauldron::PipelineObject
#include "Cauldron2/framework/render/pipelineobject.h"
// cauldron::GPUScopedProfileCapture
#include "Cauldron2/framework/render/profiler.h"
// cauldron::AddressMode
// cauldron::FilterFunc
// cauldron::ResolutionInfo
// cauldron::ResourceFlags
// cauldron::ResourceFormat
// cauldron::ResourceState
// cauldron::ShaderBindStage
// cauldron::ShaderModel
// cauldron::VertexAttributeFlag_Normal
// cauldron::VertexAttributeFlag_Position
// cauldron::VertexAttributeFlag_Tangent
// cauldron::VertexAttributeFlag_Texcoord0
// cauldron::VertexAttributeFlag_Texcoord1
// cauldron::VertexAttributeType
// cauldron::ViewDimension
#include "Cauldron2/framework/render/renderdefines.h"
// cauldron::RootSignature
#include "Cauldron2/framework/render/rootsignature.h"
// cauldron::RootSignatureDesc
#include "Cauldron2/framework/render/rootsignaturedesc.h"
// cauldron::ASManager
#include "Cauldron2/framework/render/rtresources.h"
// cauldron::Sampler
// cauldron::SamplerDesc
#include "Cauldron2/framework/render/sampler.h"
// cauldron::Texture
// cauldron::TextureDesc
#include "Cauldron2/framework/render/texture.h"
// LightInformation
// SceneLightingInformation
#include "Cauldron2/framework/shaders/shadercommon.h"

//------------------------------------------------------------------------------
// External Includes
//------------------------------------------------------------------------------

// memset
#include <cstring>
// std::lock_guard
// std::mutex
#include <mutex>
// std::vector
#include <vector>

//------------------------------------------------------------------------------
// Sample Definitions
//------------------------------------------------------------------------------

using namespace cauldron;

namespace rr
{
    [[nodiscard]]
    PathTracingRenderModule::PathTracingRenderModule()
        : RenderModule(L"PathTracingRenderModule")
    {
    }

    PathTracingRenderModule::~PathTracingRenderModule() = default;

    void PathTracingRenderModule::Init(const json& initData)
    {
        CauldronAssert(ASSERT_CRITICAL, GetFramework()->GetConfig()->RT_1_1, L"Error: Pathtracing requires RT1.1");
        CauldronAssert(ASSERT_CRITICAL, GetFramework()->GetConfig()->MinShaderModel >= ShaderModel::SM6_6, L"Error: Pathtracing requires SM6_6 or greater");

        m_pDenoiserRenderModule = static_cast< DenoiserRenderModule* >(GetFramework()->GetRenderModule("DenoiserRenderModule"));
        CauldronAssert(ASSERT_CRITICAL, m_pDenoiserRenderModule != nullptr, L"Error: DenoiserRenderModule required.");

        m_SpecularSampler.AddressW = AddressMode::Wrap;
        m_SpecularSampler.Filter = FilterFunc::MinMagMipLinear;
        m_SpecularSampler.MaxAnisotropy = 1;

        if (not InitPipelineObjects())
        {
            CauldronError(L"FidelityFX Denoiser Sample: Error: Could not initialize pathtracing pipeline objects.");
            return;
        }

        if (not InitResources())
        {
            CauldronError(L"FidelityFX Denoiser Sample: Error: Could not initialize pathtracing resources.");
            return;
        }

        BuildUI();

        // Register for content change updates
        GetContentManager()->AddContentListener(this);
        SetModuleReady(true);
    }

    void PathTracingRenderModule::EnableModule(bool enabled)
    {
        if (m_pDenoiserRenderModule and not enabled)
            m_pDenoiserRenderModule->EnableModule(enabled); // Denoiser requires PT module

        RenderModule::EnableModule(enabled);
    }

    void PathTracingRenderModule::Execute(double deltaTime, CommandList* pCmdList)
    {
        if ((not m_pBRDFTexture) or (not m_pPrefilteredEnvironmentMap))
        {
            m_pBRDFTexture               = GetScene()->GetBRDFLutTexture();
            m_pPrefilteredEnvironmentMap = GetScene()->GetIBLTexture(IBLTexture::Prefiltered);

            // These might not yet be loaded
            if (m_pBRDFTexture and m_pPrefilteredEnvironmentMap)
            {
                m_pTraceRaysDenoiserParameterSet->SetTextureSRV(m_pBRDFTexture, ViewDimension::Texture2D, 1);
                m_pTraceRaysDenoiserParameterSet->SetTextureSRV(m_pPrefilteredEnvironmentMap, ViewDimension::TextureCube, 2);
            }
            return;
        }

        GPUScopedProfileCapture sampleMarker(pCmdList, L"Pathtracing");

        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        CameraComponent*      pCamera = GetScene()->GetCurrentCamera();

        const SceneLightingInformation& sceneLightInfo    = GetScene()->GetSceneLightInfo();
        const bool                      useDominantLight  = m_pDenoiserRenderModule->UseDominantLightVisibility();
        U32                             dominantLightIndex = 0u;
        for (U32 i = 0u; i < static_cast< U32 >(sceneLightInfo.LightCount); ++i)
        {
            if (sceneLightInfo.LightInfo[i].Type == static_cast< S32 >(cauldron::LightType::Directional))
            {
                dominantLightIndex = i;
                break;
            }
        }

        PackedFrame frame = CreatePackedFrame(resInfo,
                                             pCamera,
                                             GetConfig()->InvertedDepth,
                                             static_cast< U32 >(GetFramework()->GetFrameID()),
                                             sceneLightInfo,
                                             useDominantLight,
                                             dominantLightIndex);
        frame.ambientOcclusionA  = m_AmbientOcclusionA;
        frame.ambientOcclusionB  = m_AmbientOcclusionB;
        frame.specularOcclusionA = m_SpecularOcclusionA;
        frame.specularOcclusionB = m_SpecularOcclusionB;
        frame.lighting.iblFactor = GetScene()->GetIBLFactor();
        frame.checkerboard       = m_pDenoiserRenderModule->IsCheckerboardingEnabled() ? 1u : 0u;

        BufferAddressInfo frameBufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(frame), static_cast< const void* >(&frame));
        m_pTraceRaysDenoiserParameterSet->UpdateRootConstantBuffer(&frameBufferInfo, 0);

        m_pTraceRaysDenoiserParameterSet->Bind(pCmdList, m_pTraceRaysDenoiserPipeline);
        SetPipelineState(pCmdList, m_pTraceRaysDenoiserPipeline);
        const U32 threadGroupCountX = (resInfo.RenderWidth + GROUP_THREAD_COUNT_X - 1) / GROUP_THREAD_COUNT_X;
        const U32 threadGroupCountY = (resInfo.RenderHeight + GROUP_THREAD_COUNT_Y - 1) / GROUP_THREAD_COUNT_Y;
        Dispatch(pCmdList, threadGroupCountX, threadGroupCountY, 1);
    }

    void PathTracingRenderModule::BuildUI()
    {
        static const bool Enable = false;

        if (Enable)
        {
            UISection* uiSection = GetUIManager()->RegisterUIElements("Path Tracing", UISectionType::Sample);

            uiSection->RegisterUIElement< UISlider< F32 > >("Ambient occlusion A",  m_AmbientOcclusionA,  0.01f, 1.0f, Enable, [](F32 cur, F32 old) {});
            uiSection->RegisterUIElement< UISlider< F32 > >("Ambient occlusion B",  m_AmbientOcclusionB,  0.00f, 1.0f, Enable, [](F32 cur, F32 old) {});
            uiSection->RegisterUIElement< UISlider< F32 > >("Specular occlusion A", m_SpecularOcclusionA, 0.01f, 1.0f, Enable, [](F32 cur, F32 old) {});
            uiSection->RegisterUIElement< UISlider< F32 > >("Specular occlusion B", m_SpecularOcclusionB, 0.00f, 1.0f, Enable, [](F32 cur, F32 old) {});
        }
    }

    void PathTracingRenderModule::OnNewContentLoaded(ContentBlock* pContentBlock)
    {
        std::lock_guard< std::mutex > pipelineLock(m_Mutex);

        // Materials accumulate across content blocks, so remember where this block's materials
        // begin; surface material indices are global (into m_cpuMaterialBuffer), not block-local.
        const U32 materialOffset = static_cast< U32 >(m_rtInfoTables.m_cpuMaterialBuffer.size());

        for (Material* pMat : pContentBlock->Materials)
        {
            // Resolve textures first; AddTexture appends to the bindless texture / sampler tables
            // and returns InvalidIndex (as -1) when the texture is absent.
            Map baseColorMap = { .samplerIndex = InvalidIndex, .textureIndex = InvalidIndex };
            Map armMap       = { .samplerIndex = InvalidIndex, .textureIndex = InvalidIndex };
            Map normalMap    = { .samplerIndex = InvalidIndex, .textureIndex = InvalidIndex };
            Map emissionMap  = { .samplerIndex = InvalidIndex, .textureIndex = InvalidIndex };

            S32 samplerIndex;
            if (pMat->HasPBRInfo())
            {
                baseColorMap.textureIndex = static_cast< U32 >(AddTexture(pMat, TextureClass::Albedo, samplerIndex));
                baseColorMap.samplerIndex = static_cast< U32 >(samplerIndex);

                if (pMat->HasPBRMetalRough())
                {
                    armMap.textureIndex = static_cast< U32 >(AddTexture(pMat, TextureClass::MetalRough, samplerIndex));
                    armMap.samplerIndex = static_cast< U32 >(samplerIndex);
                }
                else if (pMat->HasPBRSpecGloss())
                {
                    armMap.textureIndex = static_cast< U32 >(AddTexture(pMat, TextureClass::SpecGloss, samplerIndex));
                    armMap.samplerIndex = static_cast< U32 >(samplerIndex);
                }
            }

            normalMap.textureIndex   = static_cast< U32 >(AddTexture(pMat, TextureClass::Normal, samplerIndex));
            normalMap.samplerIndex   = static_cast< U32 >(samplerIndex);
            emissionMap.textureIndex = static_cast< U32 >(AddTexture(pMat, TextureClass::Emissive, samplerIndex));
            emissionMap.samplerIndex = static_cast< U32 >(samplerIndex);

            const Vec4 albedoColor   = pMat->GetAlbedoColor();
            const Vec4 emissiveColor = pMat->GetEmissiveColor();
            const Vec4 pbrInfo       = pMat->GetPBRInfo();

            U32 flags = 0u;
            if (pMat->GetBlendMode() == MaterialBlend::Opaque)
            {
                flags |= MaterialFlag_Opaque;
            }
            if (pMat->HasDoubleSided())
            {
                flags |= MaterialFlag_DoubleSided;
            }

            const PackedMaterial packedMaterial =
            {
                .baseColorFactor                    = { { albedoColor.getX(), albedoColor.getY(), albedoColor.getZ(), albedoColor.getW() } },
                .alphaCutoff                        = pMat->GetAlphaCutOff(),
                .ambientOcclusionFactor             = 1.0f,
                .roughnessFactor                    = pbrInfo.getY(),
                .metalnessFactor                    = pbrInfo.getX(),
                .emissionFactor                     = { { emissiveColor.getX(), emissiveColor.getY(), emissiveColor.getZ() } },
                .flags                              = flags,
                .ambientOcclusionRoughnessMetalness = armMap,
                .baseColor                          = baseColorMap,
                .emission                           = emissionMap,
                .normal                             = normalMap,
            };

            m_rtInfoTables.m_cpuMaterialBuffer.push_back(packedMaterial);
        }

        MeshComponentMgr* pMeshComponentManager = MeshComponentMgr::Get();

        U32 nodeID = 0;
        for (EntityDataBlock* pEntityData : pContentBlock->EntityDataBlocks)
        {
            for (Component* pComponent : pEntityData->Components)
            {
                if (pComponent->GetManager() == pMeshComponentManager)
                {
                    const U32 surfaceOffset = static_cast< U32 >(m_rtInfoTables.m_cpuSurfaceBuffer.size());
                    const Mesh*    pMesh         = reinterpret_cast< MeshComponent* >(pComponent)->GetData().pMesh;
                    const size_t   surfaceCount   = pMesh->GetNumSurfaces();

                    for (U32 i = 0; i < surfaceCount; ++i)
                    {
                        const Surface*  pSurface  = pMesh->GetSurface(i);
                        const Material* pMaterial = pSurface->GetMaterial();

                        PackedSurface packedSurface{};
                        // Absent vertex attributes are left InvalidIndex (0xFFFFFFFF).
                        memset(&packedSurface, -1, sizeof(packedSurface));
                        packedSurface.indexCount  = pSurface->GetIndexBuffer().Count;
                        packedSurface.vertexCount = pSurface->GetVertexBuffer(VertexAttributeType::Position).Count;

                        S32 foundIndex = -1;
                        for (size_t i = 0; i < m_rtInfoTables.m_IndexBuffers.size(); i++)
                        {
                            if (m_rtInfoTables.m_IndexBuffers[i] == pSurface->GetIndexBuffer().pBuffer)
                            {
                                foundIndex = static_cast< S32 >(i);
                                break;
                            }
                        }

                        packedSurface.ibvIndex = static_cast< U32 >(foundIndex >= 0 ? foundIndex : static_cast< S32 >(m_rtInfoTables.m_IndexBuffers.size()));
                        if (foundIndex < 0)
                            m_rtInfoTables.m_IndexBuffers.push_back(pSurface->GetIndexBuffer().pBuffer);

                        switch (pSurface->GetIndexBuffer().IndexFormat)
                        {
                        case ResourceFormat::R16_UINT:
                            packedSurface.indexByteCount = 2u;
                            break;
                        case ResourceFormat::R32_UINT:
                            packedSurface.indexByteCount = 4u;
                            break;
                        default:
                            CauldronError(L"Unsupported resource format for ray tracing indices");
                        }

                        U32 usedAttributes = VertexAttributeFlag_Position | VertexAttributeFlag_Normal | VertexAttributeFlag_Tangent |
                                                  VertexAttributeFlag_Texcoord0 | VertexAttributeFlag_Texcoord1;

                        const U32 surfaceAttributes = pSurface->GetVertexAttributes();
                        usedAttributes = usedAttributes & surfaceAttributes;

                        for (U32 attribute = 0; attribute < static_cast< U32 >(VertexAttributeType::Count); ++attribute)
                        {
                            // Check if the attribute is present
                            if (usedAttributes & (0x1 << attribute))
                            {
                                S32 foundIndex = -1;
                                for (size_t i = 0; i < m_rtInfoTables.m_VertexBuffers.size(); i++)
                                {
                                    if (m_rtInfoTables.m_VertexBuffers[i] == pSurface->GetVertexBuffer(static_cast< VertexAttributeType >(attribute)).pBuffer)
                                    {
                                        foundIndex = static_cast< S32 >(i);
                                        break;
                                    }
                                }
                                if (foundIndex < 0)
                                    m_rtInfoTables.m_VertexBuffers.push_back(pSurface->GetVertexBuffer(static_cast< VertexAttributeType >(attribute)).pBuffer);
                                const U32 vbvIndex = static_cast< U32 >(foundIndex >= 0 ? foundIndex : static_cast< S32 >(m_rtInfoTables.m_VertexBuffers.size()) - 1);
                                switch (static_cast< VertexAttributeType >(attribute))
                                {
                                case VertexAttributeType::Position:
                                    packedSurface.positionVbvIndex = vbvIndex;
                                    break;
                                case VertexAttributeType::Normal:
                                    packedSurface.normalVbvIndex = vbvIndex;
                                    break;
                                case VertexAttributeType::Tangent:
                                    packedSurface.tangentVbvIndex = vbvIndex;
                                    break;
                                case VertexAttributeType::Texcoord0:
                                    packedSurface.texcoord0VbvIndex = vbvIndex;
                                    break;
                                case VertexAttributeType::Texcoord1:
                                    packedSurface.texcoord1VbvIndex = vbvIndex;
                                    break;
                                default:
                                    break;
                                }
                            }
                        }

                        for (size_t i = 0; i < pContentBlock->Materials.size(); i++)
                        {
                            if (pContentBlock->Materials[i] == pMaterial)
                            {
                                packedSurface.materialIndex = materialOffset + static_cast< U32 >(i);
                                break;
                            }
                        }
                        m_rtInfoTables.m_cpuSurfaceBuffer.push_back(packedSurface);
                    }

                    const PackedInstance packedInstance =
                    {
                        .nodeIndex     = nodeID++,
                        .meshIndex     = 0u, // unused by the path tracer
                        .surfaceOffset = surfaceOffset,
                        .surfaceCount  = static_cast< U32 >(surfaceCount),
                    };
                    m_rtInfoTables.m_cpuInstanceBuffer.push_back(packedInstance);
                }
            }
        }

        if (m_rtInfoTables.m_cpuSurfaceBuffer.size() > 0)
        {
            // Upload
            BufferDesc bufferMaterial = BufferDesc::Data(
                L"PTMaterialBuffer", static_cast< U32 >(m_rtInfoTables.m_cpuMaterialBuffer.size() * sizeof(PackedMaterial)), sizeof(PackedMaterial), 0, ResourceFlags::None);
            m_rtInfoTables.m_pMaterialBuffer = GetDynamicResourcePool()->CreateBuffer(&bufferMaterial, ResourceState::CopyDest);
            const_cast< Buffer* >(m_rtInfoTables.m_pMaterialBuffer)
                ->CopyData(m_rtInfoTables.m_cpuMaterialBuffer.data(), m_rtInfoTables.m_cpuMaterialBuffer.size() * sizeof(PackedMaterial));

            BufferDesc bufferInstance = BufferDesc::Data(
                L"PTInstanceBuffer", static_cast< U32 >(m_rtInfoTables.m_cpuInstanceBuffer.size() * sizeof(PackedInstance)), sizeof(PackedInstance), 0, ResourceFlags::None);
            m_rtInfoTables.m_pInstanceBuffer = GetDynamicResourcePool()->CreateBuffer(&bufferInstance, ResourceState::CopyDest);
            const_cast< Buffer* >(m_rtInfoTables.m_pInstanceBuffer)
                ->CopyData(m_rtInfoTables.m_cpuInstanceBuffer.data(), m_rtInfoTables.m_cpuInstanceBuffer.size() * sizeof(PackedInstance));

            BufferDesc bufferSurface = BufferDesc::Data(
                L"PTSurfaceBuffer", static_cast< U32 >(m_rtInfoTables.m_cpuSurfaceBuffer.size() * sizeof(PackedSurface)), sizeof(PackedSurface), 0, ResourceFlags::None);
            m_rtInfoTables.m_pSurfaceBuffer = GetDynamicResourcePool()->CreateBuffer(&bufferSurface, ResourceState::CopyDest);
            const_cast< Buffer* >(m_rtInfoTables.m_pSurfaceBuffer)
                ->CopyData(m_rtInfoTables.m_cpuSurfaceBuffer.data(), m_rtInfoTables.m_cpuSurfaceBuffer.size() * sizeof(PackedSurface));

            m_pTraceRaysDenoiserParameterSet->SetBufferSRV(m_rtInfoTables.m_pMaterialBuffer, PACKED_MATERIAL_SLOT);
            m_pTraceRaysDenoiserParameterSet->SetBufferSRV(m_rtInfoTables.m_pInstanceBuffer, PACKED_INSTANCE_SLOT);
            m_pTraceRaysDenoiserParameterSet->SetBufferSRV(m_rtInfoTables.m_pSurfaceBuffer,  PACKED_SURFACE_SLOT);
        }

        {
            // Update the parameter set with loaded texture entries
            CauldronAssert(ASSERT_CRITICAL, m_rtInfoTables.m_Textures.size() <= MAX_TEXTURE_COUNT, L"Too many textures.");
            for (U32 i = 0; i < m_rtInfoTables.m_Textures.size(); ++i)
            {
                m_pTraceRaysDenoiserParameterSet->SetTextureSRV(m_rtInfoTables.m_Textures[i].pTexture, ViewDimension::Texture2D, i + TEXTURE_BEGIN_SLOT);
            }

            // Update sampler bindings as well
            CauldronAssert(ASSERT_CRITICAL, m_rtInfoTables.m_Samplers.size() <= MAX_SAMPLER_COUNT, L"Too many samplers.");
            for (U32 i = 0; i < m_rtInfoTables.m_Samplers.size(); ++i)
            {
                m_pTraceRaysDenoiserParameterSet->SetSampler(m_rtInfoTables.m_Samplers[i], i + SAMPLER_BEGIN_SLOT);
            }

            CauldronAssert(ASSERT_CRITICAL, m_rtInfoTables.m_IndexBuffers.size() <= MAX_IBV_COUNT, L"Too many index buffers.");
            for (U32 i = 0; i < m_rtInfoTables.m_IndexBuffers.size(); ++i)
            {
                m_pTraceRaysDenoiserParameterSet->SetBufferSRV(m_rtInfoTables.m_IndexBuffers[i], i + IBV_BEGIN_SLOT);
            }

            CauldronAssert(ASSERT_CRITICAL, m_rtInfoTables.m_VertexBuffers.size() <= MAX_VBV_COUNT, L"Too many vertex buffers.");
            for (U32 i = 0; i < m_rtInfoTables.m_VertexBuffers.size(); ++i)
            {
                m_pTraceRaysDenoiserParameterSet->SetBufferSRV(m_rtInfoTables.m_VertexBuffers[i], i + VBV_BEGIN_SLOT);
            }
        }
    }

    void PathTracingRenderModule::OnContentUnloaded(ContentBlock* pContentBlock)
    {
        for (const PackedMaterial& packedMaterial : m_rtInfoTables.m_cpuMaterialBuffer)
        {
            if (packedMaterial.baseColor.textureIndex != InvalidIndex)
            {
                RemoveTexture(static_cast< S32 >(packedMaterial.baseColor.textureIndex));
            }
            if (packedMaterial.ambientOcclusionRoughnessMetalness.textureIndex != InvalidIndex)
            {
                RemoveTexture(static_cast< S32 >(packedMaterial.ambientOcclusionRoughnessMetalness.textureIndex));
            }
            if (packedMaterial.emission.textureIndex != InvalidIndex)
            {
                RemoveTexture(static_cast< S32 >(packedMaterial.emission.textureIndex));
            }
            if (packedMaterial.normal.textureIndex != InvalidIndex)
            {
                RemoveTexture(static_cast< S32 >(packedMaterial.normal.textureIndex));
            }
        }
    }

    bool PathTracingRenderModule::InitPipelineObjects()
    {
        auto AddTraceRaysRootSignatureSrvs = [this](RootSignatureDesc& rootSignatureDesc)
        {
                rootSignatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1); // g_frame (PackedFrame)
                rootSignatureDesc.AddRTAccelerationStructureSet(0, ShaderBindStage::Compute, 1); // g_tlas (t0)
                rootSignatureDesc.AddTextureSRVSet(1, ShaderBindStage::Compute, 1); // g_brdf
                rootSignatureDesc.AddTextureSRVSet(2, ShaderBindStage::Compute, 1); // g_prefilteredCube
                rootSignatureDesc.AddBufferSRVSet(PACKED_INSTANCE_SLOT, ShaderBindStage::Compute, 1);
                rootSignatureDesc.AddBufferSRVSet(PACKED_MATERIAL_SLOT, ShaderBindStage::Compute, 1);
                rootSignatureDesc.AddBufferSRVSet(PACKED_SURFACE_SLOT,  ShaderBindStage::Compute, 1);
                rootSignatureDesc.AddBufferSRVSet(IBV_BEGIN_SLOT,  ShaderBindStage::Compute, MAX_IBV_COUNT);
                rootSignatureDesc.AddBufferSRVSet(VBV_BEGIN_SLOT, ShaderBindStage::Compute, MAX_VBV_COUNT);
                rootSignatureDesc.AddTextureSRVSet(TEXTURE_BEGIN_SLOT, ShaderBindStage::Compute, MAX_TEXTURE_COUNT);
                rootSignatureDesc.AddStaticSamplers(0, ShaderBindStage::Compute, 1, &m_SpecularSampler);  // g_brdfSampler (s0)
                rootSignatureDesc.AddStaticSamplers(1, ShaderBindStage::Compute, 1, &m_SpecularSampler);  // g_prefilteredCubeSampler (s1)
                rootSignatureDesc.AddSamplerSet(SAMPLER_BEGIN_SLOT, ShaderBindStage::Compute, MAX_SAMPLER_COUNT);
            };

        {
            RootSignatureDesc rootSignatureDesc;
            AddTraceRaysRootSignatureSrvs(rootSignatureDesc);

            rootSignatureDesc.AddTextureUAVSet( 0, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 1, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 2, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 3, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 4, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 5, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 6, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 7, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 8, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet( 9, ShaderBindStage::Compute, 1);
            rootSignatureDesc.AddTextureUAVSet(10, ShaderBindStage::Compute, 1);

            m_pTraceRaysDenoiserRootSignature = RootSignature::CreateRootSignature(L"TraceRaysDenoiser_RootSignature", rootSignatureDesc);
            if (not m_pTraceRaysDenoiserRootSignature)
                return false;

            PipelineDesc pipelineDesc;
            pipelineDesc.SetRootSignature(m_pTraceRaysDenoiserRootSignature);
            ShaderBuildDesc shaderDesc = ShaderBuildDesc::Compute(L"PathTracing.cs.hlsl", L"Main", ShaderModel::SM6_6, nullptr);
            shaderDesc.AdditionalParams = L"-HV 2021"; // rr headers use templates/namespaces/float32_t (overrides the framework's -HV 2018)
            pipelineDesc.AddShaderDesc(shaderDesc);
            m_pTraceRaysDenoiserPipeline = PipelineObject::CreatePipelineObject(L"TraceRaysDenoiser_Pipeline", pipelineDesc);
            if (not m_pTraceRaysDenoiserPipeline)
                return false;

            m_pTraceRaysDenoiserParameterSet = ParameterSet::CreateParameterSet(m_pTraceRaysDenoiserRootSignature);
            m_pTraceRaysDenoiserParameterSet->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(PackedFrame), 0);
            m_pTraceRaysDenoiserParameterSet->SetAccelerationStructure(GetScene()->GetASManager()->GetTLAS(), 0);
        }

        return true;
    }

    bool PathTracingRenderModule::InitResources()
    {
        U32 renderWidth  = GetFramework()->GetResolutionInfo().RenderWidth;
        U32 renderHeight = GetFramework()->GetResolutionInfo().RenderHeight;
        auto renderSizeFn = [](TextureDesc& desc, U32 displayWidth, U32 displayHeight, U32 renderingWidth, U32 renderingHeight)
        {
            desc.Width  = renderingWidth;
            desc.Height = renderingHeight;
        };

        m_pColorTarget       = GetFramework()->GetColorTargetForCallback(GetName());
        m_pDirectDiffuseOutput           = GetFramework()->GetRenderTexture(L"DenoiserDirectDiffuseTarget");
        m_pDirectSpecularOutput          = GetFramework()->GetRenderTexture(L"DenoiserDirectSpecularTarget");
        m_pIndirectDiffuseOutput         = GetFramework()->GetRenderTexture(L"DenoiserIndirectDiffuseTarget");
        m_pIndirectSpecularOutput        = GetFramework()->GetRenderTexture(L"DenoiserIndirectSpecularTarget");
        m_pDominantLightVisibilityOutput = GetFramework()->GetRenderTexture(L"DenoiserDominantLightVisibilityTarget");
        m_pAmbientOcclusionOutput        = GetFramework()->GetRenderTexture(L"DenoiserAmbientOcclusionTarget");
        m_pSpecularOcclusionOutput       = GetFramework()->GetRenderTexture(L"DenoiserSpecularOcclusionTarget");

        m_pDiffuseAlbedo                 = GetFramework()->GetRenderTexture(L"DenoiserDiffuseAlbedoTarget");
        m_pSpecularAlbedo                = GetFramework()->GetRenderTexture(L"DenoiserSpecularAlbedoTarget");
        m_pNormals                       = GetFramework()->GetRenderTexture(L"DenoiserNormalsTarget");
        m_pSkipSignal                    = GetFramework()->GetRenderTexture(L"DenoiserSkipSignalTarget");

        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pDirectSpecularOutput,          ViewDimension::Texture2D,  0);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pDirectDiffuseOutput,           ViewDimension::Texture2D,  1);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pIndirectSpecularOutput,        ViewDimension::Texture2D,  2);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pIndirectDiffuseOutput,         ViewDimension::Texture2D,  3);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pDominantLightVisibilityOutput, ViewDimension::Texture2D,  4);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pAmbientOcclusionOutput,        ViewDimension::Texture2D,  5);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pSpecularOcclusionOutput,       ViewDimension::Texture2D,  6);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pSkipSignal,                    ViewDimension::Texture2D,  7);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pDiffuseAlbedo,                 ViewDimension::Texture2D,  8);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pSpecularAlbedo,                ViewDimension::Texture2D,  9);
        m_pTraceRaysDenoiserParameterSet->SetTextureUAV(m_pNormals,                       ViewDimension::Texture2D, 10);

        return true;
    }

    // Add texture index info and return the index to the texture in the texture array
    S32 PathTracingRenderModule::AddTexture(const Material* pMaterial, const TextureClass textureClass, S32& textureSamplerIndex)
    {
        const TextureInfo* pTextureInfo = pMaterial->GetTextureInfo(textureClass);
        if (pTextureInfo != nullptr)
        {
            // Check if the texture's sampler is already one we have, and if not add it
            for (textureSamplerIndex = 0; textureSamplerIndex < m_rtInfoTables.m_Samplers.size(); ++textureSamplerIndex)
            {
                if (m_rtInfoTables.m_Samplers[textureSamplerIndex]->GetDesc() == pTextureInfo->TexSamplerDesc)
                    break;  // found
            }

            // If we didn't find the sampler, add it
            if (textureSamplerIndex == m_rtInfoTables.m_Samplers.size())
            {
                Sampler* pSampler = Sampler::CreateSampler(L"PTSampler", pTextureInfo->TexSamplerDesc);
                CauldronAssert(ASSERT_WARNING, pSampler, L"Could not create sampler for loaded content %s", pTextureInfo->pTexture->GetDesc().Name.c_str());
                m_rtInfoTables.m_Samplers.push_back(pSampler);
            }

            // Find a slot for the texture
            S32 firstFreeIndex = -1;
            for (S32 i = 0; i < m_rtInfoTables.m_Textures.size(); ++i)
            {
                RTInfoTables::BoundTexture& boundTexture = m_rtInfoTables.m_Textures[i];

                // If this texture is already mapped, bump it's reference count
                if (pTextureInfo->pTexture == boundTexture.pTexture)
                {
                    boundTexture.count += 1;
                    return i;
                }

                // Try to re-use an existing entry that was released
                else if ((firstFreeIndex < 0) and (boundTexture.count == 0))
                {
                    firstFreeIndex = i;
                }
            }

            // Texture wasn't found
            RTInfoTables::BoundTexture b = {pTextureInfo->pTexture, 1};
            if (firstFreeIndex < 0)
            {
                m_rtInfoTables.m_Textures.push_back(b);
                return static_cast< S32 >(m_rtInfoTables.m_Textures.size()) - 1;
            }
            else
            {
                m_rtInfoTables.m_Textures[firstFreeIndex] = b;
                return firstFreeIndex;
            }
        }
        return -1;
    }

    void PathTracingRenderModule::RemoveTexture(S32 index)
    {
        if (index >= 0)
        {
            m_rtInfoTables.m_Textures[index].count -= 1;
            if (m_rtInfoTables.m_Textures[index].count == 0)
            {
                m_rtInfoTables.m_Textures[index].pTexture = nullptr;
            }
        }
    }
} // namespace rr
