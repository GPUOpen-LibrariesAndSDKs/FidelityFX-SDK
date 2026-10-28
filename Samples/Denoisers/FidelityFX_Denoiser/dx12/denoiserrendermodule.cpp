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
#include "denoiserrendermodule.h"
// GROUP_THREAD_COUNT_X
// GROUP_THREAD_COUNT_Y
#include "shaders/DenoisingData.h"
// MAX_LIGHT_COUNT
// rr::DenoisingViewMode
// rr::DenoisingViewModeToString
// rr::LightType
// rr::PackedFrame
// rr::PackedLight
#include "shaders/PackedData.h"
// rr::F32
// rr::F32x4x4
// rr::S32
// rr::U32
#include "shaders/Types.h"

//------------------------------------------------------------------------------
// Cauldron Includes
//------------------------------------------------------------------------------

// SDKWrapper::ffxGetResourceApi
#include "Cauldron2/framework/core/backend_interface.h"
// cauldron::CameraComponent
// cauldron::CameraComponentData
// cauldron::CameraComponentMgr
// cauldron::CameraType
#include "Cauldron2/framework/core/components/cameracomponent.h"
// cauldron::LightType
#include "Cauldron2/framework/core/components/lightcomponent.h"
// cauldron::ContentBlock
// cauldron::EntityDataBlock
#include "Cauldron2/framework/core/contentloader.h"
// cauldron::ContentManager
#include "Cauldron2/framework/core/contentmanager.h"
// cauldron::Entity
#include "Cauldron2/framework/core/entity.h"
// cauldron::GetConfig
// cauldron::GetContentManager
// cauldron::GetDevice
// cauldron::GetDynamicBufferPool
// cauldron::GetDynamicResourcePool
// cauldron::GetFramework
// cauldron::GetScene
// cauldron::GetUIManager
// json
#include "Cauldron2/framework/core/framework.h"
// cauldron::Scene
#include "Cauldron2/framework/core/scene.h"
// cauldron::Task
#include "Cauldron2/framework/core/taskmanager.h"
// cauldron::UIButton
// cauldron::UICheckBox
// cauldron::UICombo
// cauldron::UISection
// cauldron::UISectionType
// cauldron::UISeparator
// cauldron::UISlider
// cauldron::UIText
#include "Cauldron2/framework/core/uimanager.h"
// cauldron::ASSERT_CRITICAL
// cauldron::ASSERT_ERROR
// CAULDRON_ASSERT
// cauldron::CauldronAssert
// cauldron::CauldronError
// cauldron::CauldronWarning
#include "Cauldron2/framework/misc/assert.h"
// CAUDRON_LOG_INFO
#include "Cauldron2/framework/misc/log.h"
// cauldron::InverseMatrix
// Mat4
// Point3
// Vec2
// Vec3
// Vec4
// cauldron::inverse
#include "Cauldron2/framework/misc/math.h"
// cauldron::BufferAddressInfo
#include "Cauldron2/framework/render/buffer.h"
// cauldron::CommandList
#include "Cauldron2/framework/render/commandlist.h"
// cauldron::Device
#include "Cauldron2/framework/render/device.h"
// cauldron::DynamicBufferPool
#include "Cauldron2/framework/render/dynamicbufferpool.h"
// cauldron::DynamicResourcePool
#include "Cauldron2/framework/render/dynamicresourcepool.h"
// cauldron::ParameterSet
#include "Cauldron2/framework/render/parameterset.h"
// cauldron::PipelineDesc
// cauldron::ShaderBuildDesc
#include "Cauldron2/framework/render/pipelinedesc.h"
// cauldron::PipelineObject
#include "Cauldron2/framework/render/pipelineobject.h"
// cauldron::GPUScopedProfileCapture
#include "Cauldron2/framework/render/profiler.h"
// cauldron::ResolutionInfo
// cauldron::ResourceFlags
// cauldron::ResourceFormat
// cauldron::ShaderBindStage
// cauldron::ShaderModel
// cauldron::ViewDimension
#include "Cauldron2/framework/render/renderdefines.h"
// cauldron::RootSignature
#include "Cauldron2/framework/render/rootsignature.h"
// cauldron::RootSignatureDesc
#include "Cauldron2/framework/render/rootsignaturedesc.h"
// cauldron::Texture
// cauldron::TextureDesc
#include "Cauldron2/framework/render/texture.h"
// cauldron::CommandListInternal
#include "Cauldron2/framework/render/dx12/commandlist_dx12.h"
// cauldron::DeviceInternal
#include "Cauldron2/framework/render/dx12/device_dx12.h"
// LightInformation
// SceneLightingInformation
#include "Cauldron2/framework/shaders/shadercommon.h"

//------------------------------------------------------------------------------
// FidelityFX Includes
//------------------------------------------------------------------------------

// ffx::CreateBackendDX12AllocationCallbacksDesc
// ffx::CreateBackendDX12Desc
#include <FidelityFX/api/include/dx12/ffx_api_dx12.hpp>
// ffx::Configure
// ffx::ConfigureDescGlobalDebug
// ffx::Context
// ffx::CreateContext
// ffx::CreateContextDescOverrideVersion
// ffx::DestroyContext
// ffx::Query
// ffx::QueryDescGetVersions
// ffx::ReturnCode
// ffxDispatch
// ffxDispatchDescHeader
// ffxReturnCode_t
// FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_VERBOSE
// FFX_API_EFFECT_ID_DENOISER
// FFX_API_MESSAGE_TYPE_WARNING
// FFX_API_RETURN_ERROR
// FFX_API_RETURN_OK
#include <FidelityFX/api/include/ffx_api.hpp>
// FfxApiEffectMemoryUsage
// FfxApiFloatBounds
// FfxApiMatrix4x4
// FfxApiResource
// FfxApiResourceDescription
// FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ
// FFX_API_RESOURCE_STATE_UNORDERED_ACCESS
#include <FidelityFX/api/include/ffx_api_types.h>
// ffx::ConfigureDescDenoiserKeyValue
// ffx::CreateContextDescDenoiser
// ffx::DispatchDescDenoiser
// ffx::DispatchDescDenoiserAmbientOcclusion
// ffx::DispatchDescDenoiserDebugView
// ffx::DispatchDescDenoiserDirectDiffuse
// ffx::DispatchDescDenoiserDirectSpecular
// ffx::DispatchDescDenoiserDominantLight
// ffx::DispatchDescDenoiserIndirectDiffuse
// ffx::DispatchDescDenoiserIndirectSpecular
// ffx::DispatchDescDenoiserSpecularOcclusion
// ffx::QueryDescDenoiserGetDefaultKeyValue
// ffx::QueryDescDenoiserGetGPUMemoryUsage
// FfxApiConfigureDenoiserKey
// FfxApiCreateContextDenoiserFlags
// FfxApiDenoiserDebugViewMode
// FfxApiDenoiserSignalFlags
// FfxApiDispatchDenoiserFlags
// FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS
// FFX_DENOISER_VERSION
#include <FidelityFX/denoisers/include/ffx_denoiser.hpp>

//------------------------------------------------------------------------------
// External Includes
//------------------------------------------------------------------------------

// std::cos
#include <cmath>
// uint64_t
#include <cstdint>
// std::memcpy
#include <cstring>
// std::function
#include <functional>
// std::numeric_limits
#include <limits>
// std::move
#include <utility>
// std::vector
#include <vector>

//------------------------------------------------------------------------------
// Sample Definitions
//------------------------------------------------------------------------------

using namespace cauldron;

namespace rr
{
    namespace
    {
        inline constexpr F32 DominantLightAngularRadius = 0.004363323f;

        // Repacks a Cauldron light into an rr::PackedLight. Cauldron DirectionRange.xyz points
        // toward the light (directional) / is the spot aim; rr dWorld is the light's propagation
        // direction (the shaders negate it), so store the negation for all types. Cauldron's
        // LightType ordering (0=Directional, 1=Spot, 2=Point) matches rr::LightType.
        [[nodiscard]]
        PackedLight CreatePackedLight(const LightInformation& light)
        {
            const Vec4 dirRange = light.DirectionRange;
            const Vec4 colInt   = light.ColorIntensity;
            const Vec4 posBias  = light.PosDepthBias;

            LightType type = LightType_Directional;
            switch (static_cast< cauldron::LightType >(light.Type))
            {
            case cauldron::LightType::Directional:
            {
                type = LightType_Directional;
                break;
            }
            case cauldron::LightType::Spot:
            {
                type = LightType_Spot;
                break;
            }
            case cauldron::LightType::Point:
            {
                type = LightType_Point;
                break;
            }
            }

            return PackedLight
            {
                .dWorld           = { -dirRange.getX(), -dirRange.getY(), -dirRange.getZ() },
                .maxDistance      = dirRange.getW(),
                .color            = { colInt.getX(), colInt.getY(), colInt.getZ() },
                .intensity        = colInt.getW(),
                .pWorld           = { posBias.getX(), posBias.getY(), posBias.getZ() },
                .cosAngularRadius = (type == LightType_Directional) ? std::cos(DominantLightAngularRadius) : 0.0f,
                // Cauldron stores half-angles in radians despite the misleading field names;
                // rr::PackedLight expects the cosine of the half-angle for cone tests.
                .cosInnerCone     = std::cos(light.InnerConeCos),
                .cosOuterCone     = std::cos(light.OuterConeCos),
                .type             = type,
            };
        }
    }

    [[nodiscard]]
    PackedFrame CreatePackedFrame(const ResolutionInfo& resInfo,
                                 CameraComponent* pCamera,
                                 bool usesReversedZ,
                                 U32 frameIndex,
                                 const SceneLightingInformation& sceneLightInfo,
                                 bool useDominantLight,
                                 U32 dominantLightIndex)
    {
        // Cauldron Mat4 (math::Matrix4) is 16 contiguous floats, layout-compatible with F32x4x4, so a
        // byte copy reinterprets it directly. (Mat4 is not trivially copyable, so std::bit_cast is
        // unavailable.)
        const auto toMatrix = [](const Mat4& src)
        {
            F32x4x4 dst;
            std::memcpy(&dst, &src, sizeof(dst));
            return dst;
        };

        PackedFrame frame =
        {
            .renderSize        = { resInfo.RenderWidth,  resInfo.RenderHeight },
            .upscaleSize       = { resInfo.UpscaleWidth, resInfo.UpscaleHeight },
            .invRenderSize     = { 1.0f / resInfo.RenderWidth,  1.0f / resInfo.RenderHeight },
            .invUpscaleSize    = { 1.0f / resInfo.UpscaleWidth, 1.0f / resInfo.UpscaleHeight },
            .motionVectorScale = { 1.0f, 1.0f, 1.0f },
            .frameIndex        = frameIndex,
            .debugging         = 0u,
            .viewModeWeights   = { 1.0f, 1.0f, 1.0f, 1.0f },
            .camera =
            {
                .worldToClip          = toMatrix(pCamera->GetViewProjection()),
                .clipToWorld          = toMatrix(pCamera->GetInverseViewProjection()),
                .viewToClip           = toMatrix(pCamera->GetProjection()),
                .clipToView           = toMatrix(pCamera->GetInverseProjection()),
                .worldToView          = toMatrix(pCamera->GetView()),
                .viewToWorld          = toMatrix(pCamera->GetInverseView()),
                .worldToClipPrev      = toMatrix(pCamera->GetPreviousViewProjection()),
                .clipToWorldPrev      = toMatrix(InverseMatrix(pCamera->GetPreviousViewProjection())),
                .viewToClipPrev       = toMatrix(pCamera->GetProjection()), // projection is constant across frames
                .clipToViewPrev       = toMatrix(pCamera->GetInverseProjection()),
                .worldToViewPrev      = toMatrix(pCamera->GetPreviousView()),
                .viewToWorldPrev      = toMatrix(InverseMatrix(pCamera->GetPreviousView())),
                .aspectRatio          = static_cast< F32 >(resInfo.RenderWidth) / static_cast< F32 >(resInfo.RenderHeight),
                .nearView             = pCamera->GetNearPlane(),
                .farView              = pCamera->GetFarPlane(),
                .fovAngleVertical     = pCamera->GetFovY(),
                .jitterOffsets        = { pCamera->GetJitterOffsets().getX(), pCamera->GetJitterOffsets().getY() },
                .usesInfiniteFarPlane = 0u,
                .usesReversedZ        = usesReversedZ ? 1u : 0u,
            },
        };

        // Dominant light is handled separately via DLV hit-distance + compose re-add, matching
        // the old PC path tracer which excluded it from the direct lighting loop entirely.
        // Including it in lights[] would double-count it (loop + compose) and it contributes
        // nothing indoors anyway (ray hits ceiling -> soft shadow factor = 0).
        if (useDominantLight and (dominantLightIndex < static_cast< U32 >(sceneLightInfo.LightCount)))
        {
            frame.lighting.dominantLight = CreatePackedLight(sceneLightInfo.LightInfo[dominantLightIndex]);
        }

        U32 lightIndex = 0u;
        for (U32 i = 0u; (i < static_cast< U32 >(sceneLightInfo.LightCount)) and (lightIndex < static_cast< U32 >(MAX_LIGHT_COUNT)); ++i)
        {
            if (useDominantLight and (i == dominantLightIndex))
            {
                continue;
            }
            frame.lighting.lights[lightIndex++] = CreatePackedLight(sceneLightInfo.LightInfo[i]);
        }
        frame.lighting.lightCount = lightIndex;

        return frame;
    }

    [[nodiscard]]
    DenoiserRenderModule::DenoiserRenderModule()
        : RenderModule(L"DenoiserRenderModule")
    {
    }

    DenoiserRenderModule::~DenoiserRenderModule()
    {
        DestroyDenoiserContext();
    }

    void DenoiserRenderModule::Init(const json& initData)
    {
        CauldronAssert(ASSERT_CRITICAL, GetFramework()->GetConfig()->MinShaderModel >= ShaderModel::SM6_6, L"Error: Denoiser requires SM6_6 or greater");

        CreatePipelineObjects();
        CreateResources();

        // Logging support
        {
            ffx::ConfigureDescGlobalDebug configureDesc = {};
            configureDesc.effectId  = FFX_API_EFFECT_ID_DENOISER;
            configureDesc.fpMessage = [](U32 type, const wchar_t* message)
            {
                if (type == FFX_API_MESSAGE_TYPE_WARNING)
                {
                    CauldronWarning(message);
                }
                else
                {
                    CauldronError(message);
                }
            };
            configureDesc.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_VERBOSE;

            ffx::ReturnCode retCode = ffx::Configure(configureDesc);
            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't configure global logging: %d", static_cast< U32 >(retCode));
        }

        // Query Denoiser versions
        {
            uint64_t DenoiserVersionCount = 0;
            ffx::QueryDescGetVersions queryDesc = {};
            queryDesc.createDescType = FFX_API_EFFECT_ID_DENOISER;
            queryDesc.device         = GetDevice()->GetImpl()->DX12Device();
            queryDesc.outputCount    = &DenoiserVersionCount;
            ffx::Query(queryDesc);

            m_DenoiserVersionIds.resize(DenoiserVersionCount);
            m_DenoiserVersionStrings.resize(DenoiserVersionCount);

            queryDesc.versionIds   = m_DenoiserVersionIds.data();
            queryDesc.versionNames = m_DenoiserVersionStrings.data();
            ffx::ReturnCode retCode = ffx::Query(queryDesc);
            CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't query versions: %d", static_cast< U32 >(retCode));
        }

        m_DenoiserAvailable = not m_DenoiserVersionIds.empty();
        if (not m_DenoiserAvailable)
        {
            m_ViewModeLeft  = static_cast< S32 >(DenoisingViewMode_Noisy_Composited);
            m_ViewModeRight = m_ViewModeLeft;
        }

        BuildUI();
        EnableModule(true);
        CreateContent();
    }

    void DenoiserRenderModule::EnableModule(bool enabled)
    {
        if (enabled)
        {
            CreateDenoiserContext();
            SetModuleEnabled(enabled);
        }
        else
        {
            SetModuleEnabled(enabled);
            DestroyDenoiserContext();
        }
    }

    void DenoiserRenderModule::OnPreFrame()
    {
        if (NeedsReInit())
        {
            GetDevice()->FlushAllCommandQueues();
            EnableModule(false);
            EnableModule(true);
            ClearReInit();
        }

        m_ViewModeLeft  = RemapViewMode(m_ViewModeLeft);
        m_ViewModeRight = RemapViewMode(m_ViewModeRight);
    }

    void DenoiserRenderModule::Execute(double deltaTime, CommandList* pCmdList)
    {
        GPUScopedProfileCapture sampleMarker(pCmdList, L"FSR Denoising");

        Vec3 dominantLightDir      = Vec3(0.0f);
        Vec3 dominantLightEmission = Vec3(0.0f);
        U32 dominantLightIndex     = 0;
        const SceneLightingInformation& sceneLightInfo = GetScene()->GetSceneLightInfo();
        if (UseDominantLightVisibility())
        {
            for (U32 i = 0; i < static_cast< U32 >(sceneLightInfo.LightCount); ++i)
            {
                if (sceneLightInfo.LightInfo[i].Type == static_cast< U32 >(cauldron::LightType::Directional))
                {
                    dominantLightDir = sceneLightInfo.LightInfo[i].DirectionRange.getXYZ();
                    dominantLightEmission = sceneLightInfo.LightInfo[i].ColorIntensity.getXYZ() * sceneLightInfo.LightInfo[i].ColorIntensity.getW();
                    dominantLightIndex = i;
                    break;
                }
            }
        }

        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        CameraComponent*      pCamera = GetScene()->GetCurrentCamera();
        const PackedFrame frame   = CreatePackedFrame(resInfo, pCamera, sceneLightInfo, dominantLightIndex);

        DispatchPrepare(deltaTime, pCmdList, frame);
        if (m_DenoiserAvailable)
        {
            DispatchDenoise(deltaTime, pCmdList, dominantLightDir, dominantLightEmission);
        }
        DispatchCompose(deltaTime, pCmdList, frame);
    }

    void DenoiserRenderModule::OnResize(const ResolutionInfo& resInfo)
    {
        m_NeedReInit = true;
    }

    void DenoiserRenderModule::UpdateUI(double /*deltaTime*/)
    {
    }

    void DenoiserRenderModule::ShowUI(bool show)
    {
        if (m_pUISection)
        {
            m_pUISection->Show(show);
        }
    }

    void DenoiserRenderModule::BuildUI()
    {
        m_pUISection = GetUIManager()->RegisterUIElements("FSR Denoising", UISectionType::Sample);
        UISection* uiSection = m_pUISection;

        m_ViewModeValues.clear();
        m_ViewModeLabels.clear();
        for (U32 mode = 0; mode < static_cast< U32 >(DenoisingViewMode_Count); ++mode)
        {
            const char* label = DenoisingViewModeToString(static_cast< DenoisingViewMode >(mode));
            m_ViewModeValues.push_back(static_cast< DenoisingViewMode >(mode));
            m_ViewModeLabels.push_back(label ? label : "");
        }

        if (not m_DenoiserAvailable)
        {
            auto text = uiSection->RegisterUIElement< UIText >("FSR Ray Regeneration is not supported on this device.");
            text->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
            return;
        }

        //--------------------------------------------------------------------------
        // Static Configuration
        //--------------------------------------------------------------------------
        uiSection->RegisterUIElement< UIText >("");
        uiSection->RegisterUIElement< UIText >("Static Configuration:");
        uiSection->RegisterUIElement< UISeparator >();

        uiSection->RegisterUIElement< UICombo >("Version", reinterpret_cast< S32& >(m_SelectedDenoiserVersion), m_DenoiserVersionStrings, m_DenoiserAvailable, [this](S32, S32) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable direct diffuse denoising",                     m_EnableDirectDiffuse,                                  m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable direct specular denoising",                    m_EnableDirectSpecular,                                 m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable indirect diffuse denoising",                   m_EnableIndirectDiffuse,                                m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable indirect specular denoising",                  m_EnableIndirectSpecular,                               m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable dominant light visibility denoising",          m_EnableDominantLightVisibility,                        m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable separate dominant light visibility denoising", m_EnableSeparateDominantLightVisibilityDenoiserContext, m_EnableDominantLightVisibility, [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable ambient occlusion denoising",                  m_EnableAmbientOcclusion,                               m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable specular occlusion denoising",                 m_EnableSpecularOcclusion,                              m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable checkerboarding",                              m_EnableCheckerboarding,                                m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable debugging",                                    m_EnableDebugging,                                      m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });
        uiSection->RegisterUIElement< UICheckBox >("Enable validation",                                   m_EnableValidation,                                     m_DenoiserAvailable,             [this](S32 cur, S32 old) { m_NeedReInit = true; });

        //--------------------------------------------------------------------------
        // Dynamic Configuration
        //--------------------------------------------------------------------------
        uiSection->RegisterUIElement< UIText >("");
        uiSection->RegisterUIElement< UIText >("Dynamic Configuration:");
        uiSection->RegisterUIElement< UISeparator >();

        uiSection->RegisterUIElement< UISlider< F32 > >("Cross bilateral normal strength", m_DenoiserConfiguration.m_CrossBilateralNormalStrength, 0.0f, 1.0f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Disocclusion threshold", m_DenoiserConfiguration.m_DisocclusionThreshold, 0.01f, 0.05f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Gaussian kernel relaxation", m_DenoiserConfiguration.m_GaussianKernelRelaxation, 0.0f, 1.0f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Max radiance", m_DenoiserConfiguration.m_MaxRadiance, 0.0f, 65504.0f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Radiance std clip", m_DenoiserConfiguration.m_RadianceClipStdK, 0.0f, 65504.0f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Stability bias", m_DenoiserConfiguration.m_StabilityBias, 0.0f, 1.0f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Debug view linear depth max bound", m_DenoiserConfiguration.m_DebugViewLinearDepthBounds.max, 0.001f, 1024.0f, m_DenoiserAvailable, [this](F32 cur, F32 old) { ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS); });

        //--------------------------------------------------------------------------
        // Display
        //--------------------------------------------------------------------------
        uiSection->RegisterUIElement< UIText >("");
        uiSection->RegisterUIElement< UIText >("Display:");
        uiSection->RegisterUIElement< UISeparator >();

        uiSection->RegisterUIElement< UICombo >("View mode (left)", m_ViewModeLeft, m_ViewModeLabels, m_DenoiserAvailable,
            [this](S32, S32) { m_ViewModeLeft = RemapViewMode(m_ViewModeLeft); });
        uiSection->RegisterUIElement< UICombo >("View mode (right)", m_ViewModeRight, m_ViewModeLabels, m_DenoiserAvailable,
            [this](S32, S32) { m_ViewModeRight = RemapViewMode(m_ViewModeRight); });
        uiSection->RegisterUIElement< UISlider< F32 > >("Split position", m_ViewModeSplit, 0.0f, 1.0f, m_DenoiserAvailable);
        uiSection->RegisterUIElement< UICheckBox >("R", m_DebugShowChannelR, m_DenoiserAvailable, nullptr, true, false);
        uiSection->RegisterUIElement< UICheckBox >("G", m_DebugShowChannelG, m_DenoiserAvailable, nullptr, true, true);
        uiSection->RegisterUIElement< UICheckBox >("B", m_DebugShowChannelB, m_DenoiserAvailable, nullptr, true, true);
        uiSection->RegisterUIElement< UICheckBox >("A", m_DebugShowChannelA, m_DenoiserAvailable, nullptr, true, true);

        uiSection->RegisterUIElement< UICheckBox >("Show debug view", m_ShowDebugView, m_EnableDebugging);
        std::vector< const char* > debugViewModes =
        {
            "Overview",
            "Fullscreen Target"
        };
        uiSection->RegisterUIElement< UICombo >("Debug view mode", reinterpret_cast< S32& >(m_DebugViewMode), std::move(debugViewModes), m_ShowDebugView);
        uiSection->RegisterUIElement< UISlider< S32 > >("Debug viewport index", m_DebugViewport, 0, (FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS - 1), m_ShowDebugView);

        //--------------------------------------------------------------------------

        uiSection->RegisterUIElement< UIText >("");
        uiSection->RegisterUIElement< UIButton >("Reset", m_DenoiserAvailable, [this]() { m_ForceReset = true; });
        uiSection->RegisterUIElement< UIText >("");
    }

    static ffxReturnCode_t AllocateResource(U32 effectId,
                                            D3D12_RESOURCE_STATES initialState,
                                            const D3D12_HEAP_PROPERTIES* pHeapProps,
                                            const D3D12_RESOURCE_DESC* pD3DDesc,
                                            const FfxApiResourceDescription* pFfxDesc,
                                            const D3D12_CLEAR_VALUE* pOptimizedClear,
                                            ID3D12Resource** ppD3DResource)
    {
        ID3D12Device* device = GetDevice()->GetImpl()->DX12Device();
        HRESULT hr = device->CreateCommittedResource(
            pHeapProps,
            D3D12_HEAP_FLAG_NONE,
            pD3DDesc,
            initialState,
            pOptimizedClear,
            IID_PPV_ARGS(ppD3DResource));

        Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Allocated FFX Resource through callback.");
        return SUCCEEDED(hr) ? FFX_API_RETURN_OK : FFX_API_RETURN_ERROR;
    }

    static ffxReturnCode_t DeallocateResource(U32 effectId, ID3D12Resource* pResource)
    {
        if (pResource)
        {
            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Deallocated FFX Resource through callback.");
            pResource->Release();
        }
        return FFX_API_RETURN_OK;
    }

    ffxReturnCode_t AllocateHeap(U32 effectId, const D3D12_HEAP_DESC* pHeapDesc, bool aliasable, ID3D12Heap** ppD3DHeap, uint64_t* pHeapStartOffset)
    {
        ID3D12Device* device = GetDevice()->GetImpl()->DX12Device();
        HRESULT hr = device->CreateHeap(
            pHeapDesc,
            IID_PPV_ARGS(ppD3DHeap));

        Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Allocated %s FFX heap with size %.3f MB through callback.", aliasable ? L"aliasable":L"persistent", pHeapDesc->SizeInBytes / 1048576.f);
        return SUCCEEDED(hr) ? FFX_API_RETURN_OK : FFX_API_RETURN_ERROR;
    }

    ffxReturnCode_t DeallocateHeap(U32 effectId, ID3D12Heap* pD3DHeap, uint64_t heapStartOffset, uint64_t heapSize)
    {
        if (pD3DHeap)
        {
            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__, L"Deallocated FFX Heap through callback.");
            pD3DHeap->Release();
        }
        return FFX_API_RETURN_OK;
    }

    [[nodiscard]]
    S32 DenoiserRenderModule::RemapViewMode(S32 mode)
    {
        // Composition and FSR upscaling do not support mixing checkerboarded (noisy) and
        // non-checkerboarded textures, so noisy view modes are unavailable while checkerboarding.
        if (not m_EnableCheckerboarding)
        {
            return mode;
        }

        switch (m_ViewModeValues[mode])
        {
        case DenoisingViewMode_Noisy_Composited:
        case DenoisingViewMode_Noisy_Composited_Direct:
        case DenoisingViewMode_Noisy_Composited_DirectDiffuse:
        case DenoisingViewMode_Noisy_Composited_DirectSpecular:
        case DenoisingViewMode_Noisy_Composited_Indirect:
        case DenoisingViewMode_Noisy_Composited_IndirectDiffuse:
        case DenoisingViewMode_Noisy_Composited_IndirectSpecular:
        case DenoisingViewMode_Noisy_Composited_DominantLightVisibility:
        case DenoisingViewMode_Noisy_Signal_DirectDiffuse:
        case DenoisingViewMode_Noisy_Signal_DirectSpecular:
        case DenoisingViewMode_Noisy_Signal_IndirectDiffuse:
        case DenoisingViewMode_Noisy_Signal_IndirectSpecular:
        case DenoisingViewMode_Noisy_Signal_DominantLightVisibility:
        case DenoisingViewMode_Noisy_Signal_AmbientOcclusion:
        case DenoisingViewMode_Noisy_Signal_SpecularOcclusion:
            CauldronWarning(L"Noisy (input) view modes are not supported when checkerboarding is enabled.");
            return static_cast< S32 >(DenoisingViewMode_Denoised_Composited);
        default:
            return mode;
        }
    }

    bool DenoiserRenderModule::CreateDenoiserContext()
    {
        if (not m_EnableDominantLightVisibility)
        {
            m_EnableSeparateDominantLightVisibilityDenoiserContext = false;
        }

        if (not CreateMainDenoiserContext())
        {
            return false;
        }

        if (not CreateDominantLightVisibilityDenoiserContext())
        {
            return false;
        }

        SetDefaultConfiguration();

        return true;
    }

    bool DenoiserRenderModule::CreateMainDenoiserContext()
    {
        if (not m_DenoiserAvailable)
        {
            return true;
        }

        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

        ffx::CreateBackendDX12Desc dx12BackendDesc = {};
        dx12BackendDesc.device = GetDevice()->GetImpl()->DX12Device();

        ffx::CreateBackendDX12AllocationCallbacksDesc dx12BackendAllocatorsDesc = {};
        dx12BackendAllocatorsDesc.pfnFfxResourceAllocator       = &AllocateResource;
        dx12BackendAllocatorsDesc.pfnFfxResourceDeallocator     = &DeallocateResource;
        dx12BackendAllocatorsDesc.pfnFfxHeapAllocator           = &AllocateHeap;
        dx12BackendAllocatorsDesc.pfnFfxHeapDeallocator         = &DeallocateHeap;
        dx12BackendAllocatorsDesc.pfnFfxConstantBufferAllocator = nullptr;

        ffx::CreateContextDescDenoiser createContextDesc = {};
        createContextDesc.version = FFX_DENOISER_VERSION;
        createContextDesc.maxRenderSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
        createContextDesc.signalFlags = FFX_DENOISER_SIGNAL_NONE;
        if (m_EnableDirectDiffuse)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE;
        }
        if (m_EnableDirectSpecular)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_DIRECT_SPECULAR;
        }
        if (m_EnableIndirectDiffuse)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE;
        }
        if (m_EnableIndirectSpecular)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR;
        }
        if (m_EnableDominantLightVisibility and not m_EnableSeparateDominantLightVisibilityDenoiserContext)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY;
        }
        if (m_EnableAmbientOcclusion)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION;
        }
        if (m_EnableSpecularOcclusion)
        {
            createContextDesc.signalFlags |= FFX_DENOISER_SIGNAL_SPECULAR_OCCLUSION;
        }
        if (createContextDesc.signalFlags == FFX_DENOISER_SIGNAL_NONE)
        {
            return true;
        }
        // Reconstruction Mode
        createContextDesc.checkerboardSignalFlags = FFX_DENOISER_SIGNAL_NONE;
        if (m_EnableCheckerboarding)
        {
            if (m_EnableDirectDiffuse)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE;
            }
            if (m_EnableDirectSpecular)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_DIRECT_SPECULAR;
            }
            if (m_EnableIndirectDiffuse)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE;
            }
            if (m_EnableIndirectSpecular)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR;
            }
            if (m_EnableDominantLightVisibility and not m_EnableSeparateDominantLightVisibilityDenoiserContext)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY;
            }
            if (m_EnableAmbientOcclusion)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION;
            }
            if (m_EnableSpecularOcclusion)
            {
                createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_SPECULAR_OCCLUSION;
            }
        }
        // Flags
        createContextDesc.flags = {};
        if (m_EnableDebugging)
        {
            createContextDesc.flags |= FFX_DENOISER_ENABLE_DEBUGGING;
        }
        if (m_EnableValidation)
        {
            createContextDesc.flags |= FFX_DENOISER_ENABLE_VALIDATION;
        }

        ffx::CreateContextDescOverrideVersion versionOverride = {};
        versionOverride.versionId = m_DenoiserVersionIds[m_SelectedDenoiserVersion];

        // Query predicted GPU memory usage before context creation.
        {
            FfxApiEffectMemoryUsage memory = {};
            ffx::QueryDescDenoiserGetGPUMemoryUsage queryDesc = {};
            queryDesc.device                  = dx12BackendDesc.device;
            queryDesc.maxRenderSize           = createContextDesc.maxRenderSize;
            queryDesc.signalFlags             = createContextDesc.signalFlags;
            queryDesc.checkerboardSignalFlags = createContextDesc.checkerboardSignalFlags;
            queryDesc.flags                   = createContextDesc.flags;
            queryDesc.gpuMemoryUsage          = &memory;
            ffx::ReturnCode retCode           = ffx::Query(queryDesc, versionOverride);
            CauldronAssert(ASSERT_CRITICAL,
                                     retCode == ffx::ReturnCode::Ok,
                                     L"Couldn't query predicted GPU memory usage: %d",
                                     static_cast< U32 >(retCode));
            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__,
                                         L"Denoiser version %S Query predicted GPU memory usage: totalUsageInBytes %.3f MB aliasableUsageInBytes %.3f MB",
                                         m_DenoiserVersionStrings[m_SelectedDenoiserVersion],
                                         memory.totalUsageInBytes     / 1048576.f,
                                         memory.aliasableUsageInBytes / 1048576.f);
        }

        // Create the main denoiser context.
        ffx::ReturnCode retCode = ffx::CreateContext(m_pMainDenoiserContext, nullptr, createContextDesc, dx12BackendDesc, dx12BackendAllocatorsDesc, versionOverride);
        CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't create the main denoiser context: %d", static_cast< U32 >(retCode));

        // Query actual GPU memory usage after context creation.
        {
            FfxApiEffectMemoryUsage memory = {};
            ffx::QueryDescDenoiserGetGPUMemoryUsage queryDesc = {};
            queryDesc.device                  = dx12BackendDesc.device;
            queryDesc.maxRenderSize           = createContextDesc.maxRenderSize;
            queryDesc.signalFlags             = createContextDesc.signalFlags;
            queryDesc.checkerboardSignalFlags = createContextDesc.checkerboardSignalFlags;
            queryDesc.flags                   = createContextDesc.flags;
            queryDesc.gpuMemoryUsage          = &memory;
            ffx::ReturnCode retCode2          = ffx::Query(m_pMainDenoiserContext, queryDesc);
            CauldronAssert(ASSERT_CRITICAL,
                                     retCode2 == ffx::ReturnCode::Ok,
                                     L"Couldn't query actual GPU memory usage: %d",
                                     static_cast< U32 >(retCode2));
            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__,
                                         L"Denoiser version %S Query actual GPU memory usage: totalUsageInBytes %.3f MB aliasableUsageInBytes %.3f MB",
                                         m_DenoiserVersionStrings[m_SelectedDenoiserVersion],
                                         memory.totalUsageInBytes     / 1048576.f,
                                         memory.aliasableUsageInBytes / 1048576.f);
        }

        return retCode == ffx::ReturnCode::Ok;
    }

    bool DenoiserRenderModule::CreateDominantLightVisibilityDenoiserContext()
    {
        if (not m_EnableSeparateDominantLightVisibilityDenoiserContext)
        {
            return true;
        }

        // In production code this context can be dispatched at a different point in the frame;
        // here both dispatches happen sequentially for exposition purposes only.
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

        ffx::CreateBackendDX12Desc dx12BackendDesc = {};
        dx12BackendDesc.device = GetDevice()->GetImpl()->DX12Device();

        ffx::CreateBackendDX12AllocationCallbacksDesc dx12BackendAllocatorsDesc = {};
        dx12BackendAllocatorsDesc.pfnFfxResourceAllocator       = &AllocateResource;
        dx12BackendAllocatorsDesc.pfnFfxResourceDeallocator     = &DeallocateResource;
        dx12BackendAllocatorsDesc.pfnFfxHeapAllocator           = &AllocateHeap;
        dx12BackendAllocatorsDesc.pfnFfxHeapDeallocator         = &DeallocateHeap;
        dx12BackendAllocatorsDesc.pfnFfxConstantBufferAllocator = nullptr;

        ffx::CreateContextDescOverrideVersion versionOverride = {};
        versionOverride.versionId = m_DenoiserVersionIds[m_SelectedDenoiserVersion];

        ffx::CreateContextDescDenoiser createContextDesc = {};
        createContextDesc.version       = FFX_DENOISER_VERSION;
        createContextDesc.maxRenderSize = { resInfo.UpscaleWidth, resInfo.UpscaleHeight };
        // Signals
        createContextDesc.signalFlags = FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY;
        // Reconstruction Mode
        createContextDesc.checkerboardSignalFlags = FFX_DENOISER_SIGNAL_NONE;
        if (m_EnableCheckerboarding)
        {
            createContextDesc.checkerboardSignalFlags |= FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY;
        }
        // Flags
        createContextDesc.flags = {};
        if (m_EnableDebugging)
        {
            createContextDesc.flags |= FFX_DENOISER_ENABLE_DEBUGGING;
        }
        if (m_EnableValidation)
        {
            createContextDesc.flags |= FFX_DENOISER_ENABLE_VALIDATION;
        }

        // Query predicted GPU memory usage before context creation.
        {
            FfxApiEffectMemoryUsage memory = {};
            ffx::QueryDescDenoiserGetGPUMemoryUsage queryDesc = {};
            queryDesc.device                  = dx12BackendDesc.device;
            queryDesc.maxRenderSize           = createContextDesc.maxRenderSize;
            queryDesc.signalFlags             = createContextDesc.signalFlags;
            queryDesc.checkerboardSignalFlags = createContextDesc.checkerboardSignalFlags;
            queryDesc.flags                   = createContextDesc.flags;
            queryDesc.gpuMemoryUsage          = &memory;
            ffx::ReturnCode retCode           = ffx::Query(queryDesc, versionOverride);
            CauldronAssert(ASSERT_CRITICAL,
                                     retCode == ffx::ReturnCode::Ok,
                                     L"Couldn't query predicted dominant light visibility context GPU memory usage: %d",
                                     static_cast< U32 >(retCode));
            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__,
                                         L"Denoiser version %S dominant light visibility context Query predicted GPU memory usage: totalUsageInBytes %.3f MB aliasableUsageInBytes %.3f MB",
                                         m_DenoiserVersionStrings[m_SelectedDenoiserVersion],
                                         memory.totalUsageInBytes     / 1048576.f,
                                         memory.aliasableUsageInBytes / 1048576.f);
        }

        // Create the dominant light visibility denoiser context.
        ffx::ReturnCode retCode = ffx::CreateContext(m_pDominantLightVisibilityDenoiserContext, nullptr, createContextDesc, dx12BackendDesc, dx12BackendAllocatorsDesc, versionOverride);
        CauldronAssert(ASSERT_CRITICAL, retCode == ffx::ReturnCode::Ok, L"Couldn't create the dominant light visibility denoiser context: %d", static_cast< U32 >(retCode));

        // Query actual GPU memory usage after context creation.
        {
            FfxApiEffectMemoryUsage memory = {};
            ffx::QueryDescDenoiserGetGPUMemoryUsage queryDesc = {};
            queryDesc.device                  = dx12BackendDesc.device;
            queryDesc.maxRenderSize           = createContextDesc.maxRenderSize;
            queryDesc.signalFlags             = createContextDesc.signalFlags;
            queryDesc.checkerboardSignalFlags = createContextDesc.checkerboardSignalFlags;
            queryDesc.flags                   = createContextDesc.flags;
            queryDesc.gpuMemoryUsage          = &memory;
            ffx::ReturnCode retCode2          = ffx::Query(m_pDominantLightVisibilityDenoiserContext, queryDesc);
            CauldronAssert(ASSERT_CRITICAL,
                                     retCode2 == ffx::ReturnCode::Ok,
                                     L"Couldn't query actual dominant light visibility context GPU memory usage: %d",
                                     static_cast< U32 >(retCode2));
            Log::WriteDetailed(LOGLEVEL_INFO, WFILE, __LINE__,
                                         L"Denoiser version %S dominant light visibility context Query actual GPU memory usage: totalUsageInBytes %.3f MB aliasableUsageInBytes %.3f MB",
                                         m_DenoiserVersionStrings[m_SelectedDenoiserVersion],
                                         memory.totalUsageInBytes     / 1048576.f,
                                         memory.aliasableUsageInBytes / 1048576.f);
        }

        return retCode == ffx::ReturnCode::Ok;
    }

    void DenoiserRenderModule::DestroyDenoiserContext()
    {
        DestroyDominantLightVisibilityDenoiserContext();
        DestroyMainDenoiserContext();
    }

    void DenoiserRenderModule::DestroyMainDenoiserContext()
    {
        if (m_pMainDenoiserContext != nullptr)
        {
            ffx::DestroyContext(m_pMainDenoiserContext);
            m_pMainDenoiserContext = nullptr;
        }
    }

    void DenoiserRenderModule::DestroyDominantLightVisibilityDenoiserContext()
    {
        if (m_pDominantLightVisibilityDenoiserContext != nullptr)
        {
            ffx::DestroyContext(m_pDominantLightVisibilityDenoiserContext);
            m_pDominantLightVisibilityDenoiserContext = nullptr;
        }
    }

    bool DenoiserRenderModule::CreateResources()
    {
        auto renderSizeFn = [](TextureDesc& desc, U32 displayWidth, U32 displayHeight, U32 renderingWidth, U32 renderingHeight)
        {
            desc.Width  = renderingWidth;
            desc.Height = renderingHeight;
        };
        auto displaySizeFn = [](TextureDesc& desc, U32 displayWidth, U32 displayHeight, U32 renderingWidth, U32 renderingHeight)
        {
            desc.Width  = displayWidth;
            desc.Height = displayHeight;
        };

        m_pColorTarget             = GetFramework()->GetColorTargetForCallback(GetName());
        m_pDepthTarget             = GetFramework()->GetRenderTexture(L"DepthTarget");
        m_pGBufferMotionVectors    = GetFramework()->GetRenderTexture(L"GBufferMotionVectorRT");

        m_pDiffuseAlbedo           = GetFramework()->GetRenderTexture(L"DenoiserDiffuseAlbedoTarget");
        m_pSpecularAlbedo          = GetFramework()->GetRenderTexture(L"DenoiserSpecularAlbedoTarget");
        m_pNormals                 = GetFramework()->GetRenderTexture(L"DenoiserNormalsTarget");
        m_pSkipSignal              = GetFramework()->GetRenderTexture(L"DenoiserSkipSignalTarget");

        m_pDebugView               = GetFramework()->GetRenderTexture(L"DenoiserDebugViewTarget");

        // Noisy input signals
        m_pDirectDiffuse           = GetFramework()->GetRenderTexture(L"DenoiserDirectDiffuseTarget");
        m_pDirectSpecular          = GetFramework()->GetRenderTexture(L"DenoiserDirectSpecularTarget");
        m_pIndirectDiffuse         = GetFramework()->GetRenderTexture(L"DenoiserIndirectDiffuseTarget");
        m_pIndirectSpecular        = GetFramework()->GetRenderTexture(L"DenoiserIndirectSpecularTarget");
        m_pDominantLightVisibility = GetFramework()->GetRenderTexture(L"DenoiserDominantLightVisibilityTarget");
        m_pAmbientOcclusion        = GetFramework()->GetRenderTexture(L"DenoiserAmbientOcclusionTarget");
        m_pSpecularOcclusion       = GetFramework()->GetRenderTexture(L"DenoiserSpecularOcclusionTarget");

        TextureDesc desc = m_pDirectDiffuse->GetDesc();
        desc.Flags |= ResourceFlags::AllowUnorderedAccess;

        // Denoised output signals
        desc.Name = L"Denoiser_DenoisedDirectDiffuse";
        m_pDenoisedDirectDiffuse = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Name = L"Denoiser_DenoisedDirectSpecular";
        m_pDenoisedDirectSpecular = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Name = L"Denoiser_DenoisedIndirectDiffuse";
        m_pDenoisedIndirectDiffuse = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Name = L"Denoiser_DenoisedIndirectSpecular";
        m_pDenoisedIndirectSpecular = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Name = L"Denoiser_DenoisedDominantLightVisibility";
        desc.Format = ResourceFormat::R8_UNORM;
        m_pDenoisedDominantLightVisibility = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Name = L"Denoiser_DenoisedAmbientOcclusion";
        desc.Format = ResourceFormat::R8_UNORM;
        m_pDenoisedAmbientOcclusion = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Name = L"Denoiser_DenoisedSpecularOcclusion";
        desc.Format = ResourceFormat::R8_UNORM;
        m_pDenoisedSpecularOcclusion = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        desc.Format = ResourceFormat::R32_FLOAT;
        desc.Name = L"Denoiser_LinearDepth";
        m_pLinearDepth = GetDynamicResourcePool()->CreateRenderTexture(&desc, displaySizeFn);

        // SRVs
        m_pPrePassParameterSet->SetTextureSRV(m_pDepthTarget,                             ViewDimension::Texture2D, 0); // g_depth
        // UAVs (rr order: specular before diffuse)
        m_pPrePassParameterSet->SetTextureUAV(m_pLinearDepth,                             ViewDimension::Texture2D, 0);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedDirectSpecular,                  ViewDimension::Texture2D, 1);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedDirectDiffuse,                   ViewDimension::Texture2D, 2);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedIndirectSpecular,                ViewDimension::Texture2D, 3);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedIndirectDiffuse,                 ViewDimension::Texture2D, 4);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedDominantLightVisibility,         ViewDimension::Texture2D, 5);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedAmbientOcclusion,                ViewDimension::Texture2D, 6);
        m_pPrePassParameterSet->SetTextureUAV(m_pDenoisedSpecularOcclusion,               ViewDimension::Texture2D, 7);

        for (U32 i = 0; i < m_composeParameterSetCount; ++i)
        {
            // Denoised signals (t0-t6, rr order: specular before diffuse)
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedDirectSpecular,          ViewDimension::Texture2D, 0);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedDirectDiffuse,           ViewDimension::Texture2D, 1);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedIndirectSpecular,        ViewDimension::Texture2D, 2);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedIndirectDiffuse,         ViewDimension::Texture2D, 3);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedDominantLightVisibility, ViewDimension::Texture2D, 4);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedAmbientOcclusion,        ViewDimension::Texture2D, 5);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDenoisedSpecularOcclusion,       ViewDimension::Texture2D, 6);
            // Noisy signals (t7-t13); t11 is the dominant-light hit distance the PT writes
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDirectSpecular,                  ViewDimension::Texture2D, 7);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDirectDiffuse,                   ViewDimension::Texture2D, 8);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pIndirectSpecular,                ViewDimension::Texture2D, 9);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pIndirectDiffuse,                 ViewDimension::Texture2D, 10);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDominantLightVisibility,         ViewDimension::Texture2D, 11);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pAmbientOcclusion,                ViewDimension::Texture2D, 12);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pSpecularOcclusion,               ViewDimension::Texture2D, 13);
            // G-buffer / auxiliary (t14-t20)
            m_pComposeParameterSets[i]->SetTextureSRV(m_pSkipSignal,                      ViewDimension::Texture2D, 14);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pDiffuseAlbedo,                   ViewDimension::Texture2D, 15);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pSpecularAlbedo,                  ViewDimension::Texture2D, 16);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pLinearDepth,                     ViewDimension::Texture2D, 17);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pGBufferMotionVectors,            ViewDimension::Texture2D, 18);
            m_pComposeParameterSets[i]->SetTextureSRV(m_pNormals,                         ViewDimension::Texture2D, 19);
            // UAVs
            m_pComposeParameterSets[i]->SetTextureUAV(m_pColorTarget,                     ViewDimension::Texture2D, 0);
        }
        m_pCurrentComposeParameterSet = m_pComposeParameterSets[m_pCurrentComposeParameterSetIndex];

        return true;
    }

    bool DenoiserRenderModule::CreatePipelineObjects()
    {
        RootSignatureDesc prePassRootSignatureDesc;
        prePassRootSignatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1); // g_frame
        prePassRootSignatureDesc.AddTextureSRVSet(0, ShaderBindStage::Compute, 1);      // g_depth
        for (U32 u = 0; u <= 7; ++u) // u0 linear depth, u1-u7 denoised
        {
            prePassRootSignatureDesc.AddTextureUAVSet(u, ShaderBindStage::Compute, 1);
        }
        m_pPrePassRootSignature = RootSignature::CreateRootSignature(L"PrePass_RootSignature", prePassRootSignatureDesc);
        if (not m_pPrePassRootSignature)
            return false;

        PipelineDesc prePassPipelineDesc;
        prePassPipelineDesc.SetRootSignature(m_pPrePassRootSignature);
        ShaderBuildDesc prePassDesc = ShaderBuildDesc::Compute(L"DenoisingPrepare.cs.hlsl", L"Main", ShaderModel::SM6_6, nullptr);
        prePassDesc.AdditionalParams = L"-HV 2021"; // rr headers use templates/namespaces/float32_t (overrides the framework's -HV 2018)
        prePassPipelineDesc.AddShaderDesc(prePassDesc);
        m_pPrePassPipeline = PipelineObject::CreatePipelineObject(L"PrePass_Pipeline", prePassPipelineDesc);
        if (not m_pPrePassPipeline)
            return false;

        m_pPrePassParameterSet = ParameterSet::CreateParameterSet(m_pPrePassRootSignature);
        m_pPrePassParameterSet->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(PackedFrame), 0);

        RootSignatureDesc composeRootSignatureDesc;
        composeRootSignatureDesc.AddConstantBufferView(0, ShaderBindStage::Compute, 1); // g_frame
        for (U32 t = 0; t <= 20; ++t) // t0-t6 denoised, t7-t13 noisy, t14 skip, t15/16 albedo, t17 depth, t18 mv, t19 normals, t20 debug
        {
            composeRootSignatureDesc.AddTextureSRVSet(t, ShaderBindStage::Compute, 1);
        }
        composeRootSignatureDesc.AddTextureUAVSet(0, ShaderBindStage::Compute, 1);      // g_composed
        m_pComposeRootSignature = RootSignature::CreateRootSignature(L"Compose_RootSignature", composeRootSignatureDesc);
        if (not m_pComposeRootSignature)
            return false;

        PipelineDesc composePipelineDesc;
        composePipelineDesc.SetRootSignature(m_pComposeRootSignature);
        ShaderBuildDesc composeDesc = ShaderBuildDesc::Compute(L"DenoisingCompose.cs.hlsl", L"Main", ShaderModel::SM6_6, nullptr);
        composeDesc.AdditionalParams = L"-HV 2021"; // rr headers use templates/namespaces/float32_t (overrides the framework's -HV 2018)
        composePipelineDesc.AddShaderDesc(composeDesc);
        m_pComposePipeline = PipelineObject::CreatePipelineObject(L"Compose_Pipeline", composePipelineDesc);
        if (not m_pComposePipeline)
            return false;

        for (U32 i = 0; i < m_composeParameterSetCount; ++i)
        {
            m_pComposeParameterSets[i] = ParameterSet::CreateParameterSet(m_pComposeRootSignature);
            m_pComposeParameterSets[i]->SetRootConstantBufferResource(GetDynamicBufferPool()->GetResource(), sizeof(PackedFrame), 0);
        }

        return true;
    }

    bool DenoiserRenderModule::CreateContent()
    {
        // Need to create our content on a background thread so proper notifiers can be called
        std::function< void(void*) > createContent = [this](void*)
            {
                CameraComponentData cameraComponentData = {};
                cameraComponentData.Name = L"DenoiserCamera";
                cameraComponentData.Type = CameraType::Perspective;
                cameraComponentData.Zfar = std::numeric_limits< F32 >::max();
                cameraComponentData.Perspective.AspectRatio = GetFramework()->GetAspectRatio();
                cameraComponentData.Perspective.Yfov = 0.883572936f;

                ContentBlock* pContentBlock = new ContentBlock();

                // Memory backing camera creation
                EntityDataBlock* pCameraDataBlock = new EntityDataBlock();
                pContentBlock->EntityDataBlocks.push_back(pCameraDataBlock);
                pCameraDataBlock->pEntity = new Entity(cameraComponentData.Name.c_str());
                m_pDenoiserCamera = pCameraDataBlock->pEntity;
                CauldronAssert(ASSERT_CRITICAL, m_pDenoiserCamera, L"Could not allocate denoiser camera entity");

                // Calculate transform
                Mat4 lookAt = Mat4::lookAt(Point3(-6.8f, 2.0f, -5.8f), Point3(1.0f, 0.5f, -0.5f), Vec3(0.0f, 1.0f, 0.0f));
                Mat4 transform = InverseMatrix(lookAt);
                m_pDenoiserCamera->SetTransform(transform);

                CameraComponentData* pCameraComponentData = new CameraComponentData(cameraComponentData);
                pCameraDataBlock->ComponentsData.push_back(pCameraComponentData);
                m_pDenoiserCameraComponent = CameraComponentMgr::Get()->SpawnCameraComponent(m_pDenoiserCamera, pCameraComponentData);
                pCameraDataBlock->Components.push_back(m_pDenoiserCameraComponent);
                pContentBlock->ActiveCamera = m_pDenoiserCamera;

                GetContentManager()->StartManagingContent(L"DenoiserRenderModule", pContentBlock, false);

                // We are now ready for use
                SetModuleReady(true);
            };

        // Queue a task to create needed content after setup (but before run)
        Task createContentTask(createContent, nullptr);
        GetFramework()->AddContentCreationTask(createContentTask);

        return true;
    }

    void DenoiserRenderModule::ApplyConfiguration()
    {
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH);
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS);
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE);
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K);
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION);
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD);
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS);
    }

    void DenoiserRenderModule::ApplyConfiguration(FfxApiConfigureDenoiserKey key)
    {
        ffx::ConfigureDescDenoiserKeyValue configureDesc = {};
        configureDesc.key   = key;
        configureDesc.count = 1u;

        const auto applyToContext = [&configureDesc](ffx::Context& context, const wchar_t* paramName)
        {
            if (context == nullptr)
            {
                return;
            }
            const ffx::ReturnCode retCode = ffx::Configure(context, configureDesc);
            CauldronAssert(ASSERT_ERROR, retCode == ffx::ReturnCode::Ok, L"Couldn't configure %s parameter: %d", paramName, static_cast< S32 >(retCode));
        };
        const auto applyToContexts = [this, &applyToContext](const wchar_t* paramName)
        {
            applyToContext(m_pMainDenoiserContext, paramName);
            applyToContext(m_pDominantLightVisibilityDenoiserContext, paramName);
        };

        switch (key)
        {

        case FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_CrossBilateralNormalStrength;
            applyToContexts(L"CrossBilateralNormalStrength");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_StabilityBias;
            applyToContexts(L"StabilityBias");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_MaxRadiance;
            applyToContexts(L"MaxRadiance");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_RadianceClipStdK;
            applyToContexts(L"RadianceClipStdK");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_GaussianKernelRelaxation;
            applyToContexts(L"GaussianKernelRelaxation");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_DisocclusionThreshold;
            applyToContexts(L"DisocclusionThreshold");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS:
        {
            configureDesc.data = &m_DenoiserConfiguration.m_DebugViewLinearDepthBounds;
            applyToContexts(L"DebugViewLinearDepthBounds");
            break;
        }
        default:
        {
            CauldronAssert(ASSERT_ERROR, false, L"Unknown configuration parameter key: %d", static_cast< S32 >(key));
            break;
        }

        }
    }

    void DenoiserRenderModule::SetDefaultConfiguration()
    {
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH);
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS);
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE);
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K);
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION);
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD);
        SetDefaultConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS);
    }

    void DenoiserRenderModule::SetDefaultConfiguration(FfxApiConfigureDenoiserKey key)
    {
        ffx::QueryDescDenoiserGetDefaultKeyValue queryDesc = {};
        queryDesc.key   = key;
        queryDesc.count = 1u;

        const auto applyToContext = [&queryDesc](ffx::Context& context, const wchar_t* paramName)
        {
            if (context == nullptr)
            {
                return;
            }
            const ffx::ReturnCode retCode = ffx::Query(context, queryDesc);
            CauldronAssert(ASSERT_ERROR, retCode == ffx::ReturnCode::Ok, L"Couldn't query default %s parameter: %d", paramName, static_cast< S32 >(retCode));
        };
        const auto applyToContexts = [this, &applyToContext](const wchar_t* paramName)
        {
            applyToContext(m_pMainDenoiserContext, paramName);
            applyToContext(m_pDominantLightVisibilityDenoiserContext, paramName);
        };

        switch (key)
        {

        case FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_CrossBilateralNormalStrength;
            applyToContexts(L"CrossBilateralNormalStrength");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_StabilityBias;
            applyToContexts(L"StabilityBias");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_MaxRadiance;
            applyToContexts(L"MaxRadiance");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_RadianceClipStdK;
            applyToContexts(L"RadianceClipStdK");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_GaussianKernelRelaxation;
            applyToContexts(L"GaussianKernelRelaxation");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_DisocclusionThreshold;
            applyToContexts(L"DisocclusionThreshold");
            break;
        }
        case FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS:
        {
            queryDesc.data = &m_DenoiserConfiguration.m_DebugViewLinearDepthBounds;
            applyToContexts(L"DebugViewLinearDepthBounds");
            break;
        }
        default:
        {
            CauldronAssert(ASSERT_ERROR, false, L"Unknown configuration parameter key: %d", static_cast< S32 >(key));
            break;
        }

        }
    }

    PackedFrame DenoiserRenderModule::CreatePackedFrame(const ResolutionInfo& resInfo, CameraComponent* pCamera, const SceneLightingInformation& sceneLightInfo, U32 dominantLightIndex) const
    {
        PackedFrame frame = rr::CreatePackedFrame(resInfo,
                                                  pCamera,
                                                  GetConfig()->InvertedDepth,
                                                  static_cast< U32 >(GetFramework()->GetFrameID()),
                                                  sceneLightInfo,
                                                  UseDominantLightVisibility(),
                                                  dominantLightIndex);

        // Per-channel output mask (the compose shader multiplies its RGBA result by this).
        frame.viewModeWeights =
        {
            m_DebugShowChannelR ? 1.0f : 0.0f,
            m_DebugShowChannelG ? 1.0f : 0.0f,
            m_DebugShowChannelB ? 1.0f : 0.0f,
            m_DebugShowChannelA ? 1.0f : 0.0f,
        };

        // m_ViewModeLeft / m_ViewModeRight index m_ViewModeValues (the DenoisingViewMode set the dropdown exposes).
        const auto resolveViewMode = [this](S32 index)
        {
            return ((index >= 0) and (index < static_cast< S32 >(m_ViewModeValues.size())))
                 ? m_ViewModeValues[index]
                 : DenoisingViewMode_Denoised_Composited;
        };

        frame.viewModeLeft  = resolveViewMode(m_ViewModeLeft);
        frame.viewModeRight = resolveViewMode(m_ViewModeRight);
        frame.viewModeSplit = m_ViewModeSplit;
        // m_DebugViewport is a zero-based FFX viewport index; add 1 to skip DenoisingDebugViewMode_None.
        frame.debugViewMode = m_ShowDebugView
                            ? static_cast< DenoisingDebugViewMode >(m_DebugViewport + 1u)
                            : DenoisingDebugViewMode_None;

        return frame;
    }

    void DenoiserRenderModule::DispatchPrepare(double deltaTime, CommandList* pCmdList, const PackedFrame& frame)
    {
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

        BufferAddressInfo constantsBufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(frame), static_cast< const void* >(&frame));
        m_pPrePassParameterSet->UpdateRootConstantBuffer(&constantsBufferInfo, 0);

        m_pPrePassParameterSet->Bind(pCmdList, m_pPrePassPipeline);
        SetPipelineState(pCmdList, m_pPrePassPipeline);
        const U32 threadGroupCountX = (resInfo.RenderWidth + GROUP_THREAD_COUNT_X - 1) / GROUP_THREAD_COUNT_X;
        const U32 threadGroupCountY = (resInfo.RenderHeight + GROUP_THREAD_COUNT_Y - 1) / GROUP_THREAD_COUNT_Y;
        Dispatch(pCmdList, threadGroupCountX, threadGroupCountY, 1);
    }

    void DenoiserRenderModule::DispatchDenoise(double deltaTime, CommandList* pCmdList, const Vec3& dominantLightDir, const Vec3& dominantLightEmission)
    {
        if (NeedsReInit())
        {
            return;
        }

        bool                  reset   = m_ForceReset;
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();
        CameraComponent*      pCamera = GetScene()->GetCurrentCamera();

        // Needs to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiser dispatchDesc = {};
        dispatchDesc.commandList          = pCmdList->GetImpl()->DX12CmdList();
        dispatchDesc.renderSize.width     = resInfo.RenderWidth;
        dispatchDesc.renderSize.height    = resInfo.RenderHeight;
        dispatchDesc.motionVectorScale.x  = 1.0f; // Motion Vectors are expressed in UV space.
        dispatchDesc.motionVectorScale.y  = 1.0f; // Motion Vectors are expressed in UV space.
        dispatchDesc.motionVectorScale.z  = 1.0f;

        const Vec2 jitterOffsets   = pCamera->GetJitterOffsets();
        dispatchDesc.jitterOffsets = { jitterOffsets.getX(), jitterOffsets.getY() };

        const Vec3 cameraPosition        = pCamera->GetCameraPos();
        const Vec3 cameraPositionDelta   = m_PrevCameraPosition - cameraPosition;
        dispatchDesc.cameraPositionDelta = { cameraPositionDelta.getX(), cameraPositionDelta.getY(), cameraPositionDelta.getZ() };

        // Cauldron: column-major with column vectors
        // FFX:      row-major    with row vectors
        //        == same memory layout
        const Mat4 view       = pCamera->GetView();
        const Mat4 projection = pCamera->GetProjection();
        static_assert(sizeof(FfxApiMatrix4x4) == sizeof(Mat4));
        std::memcpy(&dispatchDesc.view,       &view,       sizeof(FfxApiMatrix4x4));
        std::memcpy(&dispatchDesc.projection, &projection, sizeof(FfxApiMatrix4x4));

        dispatchDesc.linearDepthBounds      = { 0.0f, 1024.0f };

        dispatchDesc.frameIndex             = static_cast< U32 >(GetFramework()->GetFrameID());

        dispatchDesc.flags = 0;
        if (reset)
        {
            dispatchDesc.flags |= FFX_DENOISER_DISPATCH_RESET;
        }

        dispatchDesc.linearDepth    = SDKWrapper::ffxGetResourceApi(m_pLinearDepth->GetResource(),          FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchDesc.motionVectors  = SDKWrapper::ffxGetResourceApi(m_pGBufferMotionVectors->GetResource(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchDesc.normals        = SDKWrapper::ffxGetResourceApi(m_pNormals->GetResource(),              FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchDesc.specularAlbedo = SDKWrapper::ffxGetResourceApi(m_pSpecularAlbedo->GetResource(),       FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchDesc.diffuseAlbedo  = SDKWrapper::ffxGetResourceApi(m_pDiffuseAlbedo->GetResource(),        FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        // Both contexts require a ffx::DispatchDescDenoiser.
        ffx::DispatchDescDenoiser dispatchDescDominantLightVisibility = dispatchDesc;

        ffxDispatchDescHeader* prevMainHeader = &dispatchDesc.header;
        const auto linkToMain = [&prevMainHeader](auto& desc)
        {
            ffxDispatchDescHeader* const nextHeader = &desc.header;
            prevMainHeader->pNext = nextHeader;
            prevMainHeader = nextHeader;
        };

        ffxDispatchDescHeader* prevDominantLightVisibilityHeader = &dispatchDescDominantLightVisibility.header;
        const auto linkToDominantLightVisibility = [&prevDominantLightVisibilityHeader](auto& desc)
        {
            ffxDispatchDescHeader* const nextHeader = &desc.header;
            prevDominantLightVisibilityHeader->pNext = nextHeader;
            prevDominantLightVisibilityHeader = nextHeader;
        };

        const U32 checkerboardOrigin = m_EnableCheckerboarding ? (dispatchDesc.frameIndex & 0b1u) : 0b0u;

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserAmbientOcclusion dispatchDescAmbientOcclusion = {};
        if (m_EnableAmbientOcclusion)
        {
            dispatchDescAmbientOcclusion.signal.input               = SDKWrapper::ffxGetResourceApi(m_pAmbientOcclusion->GetResource(),                FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescAmbientOcclusion.signal.output              = SDKWrapper::ffxGetResourceApi(m_pDenoisedAmbientOcclusion->GetResource(),        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescAmbientOcclusion.signal.checkerboardOrigin  = checkerboardOrigin;

            linkToMain(dispatchDescAmbientOcclusion);
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserDirectDiffuse dispatchDescDirectDiffuse = {};
        if (m_EnableDirectDiffuse)
        {
            dispatchDescDirectDiffuse.signal.input                  = SDKWrapper::ffxGetResourceApi(m_pDirectDiffuse->GetResource(),                   FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescDirectDiffuse.signal.output                 = SDKWrapper::ffxGetResourceApi(m_pDenoisedDirectDiffuse->GetResource(),           FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescDirectDiffuse.signal.checkerboardOrigin     = checkerboardOrigin;

            linkToMain(dispatchDescDirectDiffuse);
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserDirectSpecular dispatchDescDirectSpecular = {};
        if (m_EnableDirectSpecular)
        {
            dispatchDescDirectSpecular.signal.input                 = SDKWrapper::ffxGetResourceApi(m_pDirectSpecular->GetResource(),                  FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescDirectSpecular.signal.output                = SDKWrapper::ffxGetResourceApi(m_pDenoisedDirectSpecular->GetResource(),          FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescDirectSpecular.signal.checkerboardOrigin    = checkerboardOrigin;

            linkToMain(dispatchDescDirectSpecular);
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserDominantLight dispatchDescDominantLight = {};
        if (m_EnableDominantLightVisibility)
        {
            dispatchDescDominantLight.emission                      = { dominantLightEmission.getX(), dominantLightEmission.getY(), dominantLightEmission.getZ() };
            dispatchDescDominantLight.direction                     = { -dominantLightDir.getX(), -dominantLightDir.getY(), -dominantLightDir.getZ() };
            dispatchDescDominantLight.angularRadius                 = DominantLightAngularRadius;
            dispatchDescDominantLight.signal.input                  = SDKWrapper::ffxGetResourceApi(m_pDominantLightVisibility->GetResource(),         FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescDominantLight.signal.output                 = SDKWrapper::ffxGetResourceApi(m_pDenoisedDominantLightVisibility->GetResource(), FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescDominantLight.signal.checkerboardOrigin     = checkerboardOrigin;

            if (m_pDominantLightVisibilityDenoiserContext != nullptr)
            {
                linkToDominantLightVisibility(dispatchDescDominantLight);
            }
            else
            {
                linkToMain(dispatchDescDominantLight);
            }
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserIndirectDiffuse dispatchDescIndirectDiffuse = {};
        if (m_EnableIndirectDiffuse)
        {
            dispatchDescIndirectDiffuse.signal.input                = SDKWrapper::ffxGetResourceApi(m_pIndirectDiffuse->GetResource(),                 FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescIndirectDiffuse.signal.output               = SDKWrapper::ffxGetResourceApi(m_pDenoisedIndirectDiffuse->GetResource(),         FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescIndirectDiffuse.signal.checkerboardOrigin   = checkerboardOrigin;

            linkToMain(dispatchDescIndirectDiffuse);
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserIndirectSpecular dispatchDescIndirectSpecular = {};
        if (m_EnableIndirectSpecular)
        {
            dispatchDescIndirectSpecular.signal.input               = SDKWrapper::ffxGetResourceApi(m_pIndirectSpecular->GetResource(),                FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescIndirectSpecular.signal.output              = SDKWrapper::ffxGetResourceApi(m_pDenoisedIndirectSpecular->GetResource(),        FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescIndirectSpecular.signal.checkerboardOrigin  = checkerboardOrigin;

            linkToMain(dispatchDescIndirectSpecular);
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserSpecularOcclusion dispatchDescSpecularOcclusion = {};
        if (m_EnableSpecularOcclusion)
        {
            dispatchDescSpecularOcclusion.signal.input              = SDKWrapper::ffxGetResourceApi(m_pSpecularOcclusion->GetResource(),               FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatchDescSpecularOcclusion.signal.output             = SDKWrapper::ffxGetResourceApi(m_pDenoisedSpecularOcclusion->GetResource(),       FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescSpecularOcclusion.signal.checkerboardOrigin = checkerboardOrigin;

            linkToMain(dispatchDescSpecularOcclusion);
        }

        // Need to persist in the ::ffxDispatch scope.
        ffx::DispatchDescDenoiserDebugView dispatchDescDebugView = {};
        if (m_EnableDebugging and m_ShowDebugView)
        {
            dispatchDescDebugView.output                            = SDKWrapper::ffxGetResourceApi(m_pDebugView->GetResource(),                       FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescDebugView.outputSize                        = { m_pDebugView->GetDesc().Width, m_pDebugView->GetDesc().Height };
            dispatchDescDebugView.mode                              = m_DebugViewMode;
            dispatchDescDebugView.viewportIndex                     = static_cast< U32 >(m_DebugViewport);

            linkToMain(dispatchDescDebugView);
        }

        if (m_pMainDenoiserContext != nullptr)
        {
            // ffx::Dispatch cannot be used because it will reset pNext.
            ffxReturnCode_t ret = ::ffxDispatch(&m_pMainDenoiserContext, &dispatchDesc.header);
            CAULDRON_ASSERT(ret == FFX_API_RETURN_OK);
        }

        if (m_pDominantLightVisibilityDenoiserContext != nullptr)
        {
            // ffx::Dispatch cannot be used because it will reset pNext.
            ffxReturnCode_t ret = ::ffxDispatch(&m_pDominantLightVisibilityDenoiserContext, &dispatchDescDominantLightVisibility.header);
            CAULDRON_ASSERT(ret == FFX_API_RETURN_OK);
        }

        // Reset all descriptor heaps
        SetAllResourceViewHeaps(pCmdList);
        m_ForceReset = false;
        m_PrevCameraPosition = cameraPosition;
    }

    void DenoiserRenderModule::DispatchCompose(double deltaTime, CommandList* pCmdList, const PackedFrame& frame)
    {
        const ResolutionInfo& resInfo = GetFramework()->GetResolutionInfo();

        // triple buffered to work around CPU/GPU overlap
        m_pCurrentComposeParameterSetIndex = (m_pCurrentComposeParameterSetIndex + 1) % m_composeParameterSetCount;
        m_pCurrentComposeParameterSet = m_pComposeParameterSets[m_pCurrentComposeParameterSetIndex];

        // All SRVs are statically bound in CreateResources; the shader selects signals via frame.viewModeLeft/viewModeRight.
        BufferAddressInfo constantsBufferInfo = GetDynamicBufferPool()->AllocConstantBuffer(sizeof(frame), static_cast< const void* >(&frame));
        m_pCurrentComposeParameterSet->UpdateRootConstantBuffer(&constantsBufferInfo, 0);

        m_pCurrentComposeParameterSet->Bind(pCmdList, m_pComposePipeline);
        SetPipelineState(pCmdList, m_pComposePipeline);
        const U32 threadGroupCountX = (resInfo.RenderWidth + GROUP_THREAD_COUNT_X - 1) / GROUP_THREAD_COUNT_X;
        const U32 threadGroupCountY = (resInfo.RenderHeight + GROUP_THREAD_COUNT_Y - 1) / GROUP_THREAD_COUNT_Y;
        Dispatch(pCmdList, threadGroupCountX, threadGroupCountY, 1);
    }
} // namespace rr
