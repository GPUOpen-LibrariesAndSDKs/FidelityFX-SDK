// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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


#include <algorithm>    // for max used inside SPD CPU code.
#include <cmath>        // for fabs, abs, sinf, sqrt, etc.
#include <string>       // for memset
#include <cfloat>       // for FLT_EPSILON
#include <FidelityFX/host/ffx_frameinterpolation.h>

#define FFX_CPU

#include <FidelityFX/gpu/ffx_core.h>
#include <FidelityFX/gpu/spd/ffx_spd.h>
#include <ffx_object_management.h>

#include "ffx_frameinterpolation_private.h"

// max queued frames for descriptor management
static const uint32_t FSR3_MAX_QUEUED_FRAMES = 16;

// lists to map shader resource bindpoint name to resource identifier
typedef struct ResourceBinding
{
    uint32_t    index;
    wchar_t     name[64];
}ResourceBinding;

static const ResourceBinding srvResourceBindingTable[] =
{
    // Frame Interpolation textures
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_DEPTH,                              L"r_dilated_depth"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS,                     L"r_dilated_motion_vectors"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME,         L"r_reconstructed_depth_previous_frame"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME,     L"r_reconstructed_depth_interpolated_frame"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PREVIOUS_INTERPOLATION_SOURCE,              L"r_previous_interpolation_source"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE,              L"r_current_interpolation_source"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK,                          L"r_disocclusion_mask"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X,                 L"r_game_motion_vector_field_x"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y,                 L"r_game_motion_vector_field_y"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X,         L"r_optical_flow_motion_vector_field_x"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y,         L"r_optical_flow_motion_vector_field_y"},

    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR,                        L"r_optical_flow"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_CONFIDENCE,                    L"r_optical_flow_confidence"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_GLOBAL_MOTION,                 L"r_optical_flow_global_motion"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCENE_CHANGE_DETECTION,        L"r_optical_flow_scd"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT,                                     L"r_output"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID,                         L"r_inpainting_pyramid"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PRESENT_BACKBUFFER,                         L"r_present_backbuffer"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS,                                   L"r_counters"},
};

static const ResourceBinding uavResourceBindingTable[] =
{
    // Frame Interpolation textures
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_DEPTH,                              L"rw_dilated_depth"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS,                     L"rw_dilated_motion_vectors"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME,         L"rw_reconstructed_depth_previous_frame"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME,     L"rw_reconstructed_depth_interpolated_frame"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT,                                     L"rw_output"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK,                          L"rw_disocclusion_mask"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X,                 L"rw_game_motion_vector_field_x"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y,                 L"rw_game_motion_vector_field_y"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X,         L"rw_optical_flow_motion_vector_field_x"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y,         L"rw_optical_flow_motion_vector_field_y"},

    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS,                                   L"rw_counters"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_0,                L"rw_inpainting_pyramid0"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_1,                L"rw_inpainting_pyramid1"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_2,                L"rw_inpainting_pyramid2"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_3,                L"rw_inpainting_pyramid3"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_4,                L"rw_inpainting_pyramid4"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_5,                L"rw_inpainting_pyramid5"}, // extra declaration, as this is globallycoherent
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_6,                L"rw_inpainting_pyramid6"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_7,                L"rw_inpainting_pyramid7"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_8,                L"rw_inpainting_pyramid8"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_9,                L"rw_inpainting_pyramid9"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_10,               L"rw_inpainting_pyramid10"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_11,               L"rw_inpainting_pyramid11"},
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_12,               L"rw_inpainting_pyramid12"},
};

static const ResourceBinding cbResourceBindingTable[] =
{
    {FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_IDENTIFIER,                                      L"cbFI"},
    {FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER,                   L"cbInpaintingPyramid"},
};

// Broad structure of the root signature.
typedef enum FrameInterpolationRootSignatureLayout {

    FRAMEINTERPOLATION_ROOT_SIGNATURE_LAYOUT_UAVS,
    FRAMEINTERPOLATION_ROOT_SIGNATURE_LAYOUT_SRVS,
    FRAMEINTERPOLATION_ROOT_SIGNATURE_LAYOUT_CONSTANTS,
    FRAMEINTERPOLATION_ROOT_SIGNATURE_LAYOUT_CONSTANTS_REGISTER_1,
    FRAMEINTERPOLATION_ROOT_SIGNATURE_LAYOUT_PARAMETER_COUNT
} FrameInterpolationRootSignatureLayout;

typedef union FrameInterpolationSecondaryUnion
{
    InpaintingPyramidConstants inpaintingPyramidConstants;
} FrameInterpolationSecondaryUnion;

FfxConstantBuffer globalFrameInterpolationConstantBuffers[] = {   
    {sizeof(FrameInterpolationConstants) / sizeof(uint32_t)},
    {sizeof(InpaintingPyramidConstants) / sizeof(uint32_t)}
};

// Lanczos
static float lanczos2(float value)
{
    return abs(value) < FFX_EPSILON ? 1.f : (sinf(FFX_PI * value) / (FFX_PI * value)) * (sinf(0.5f * FFX_PI * value) / (0.5f * FFX_PI * value));
}

// Calculate halton number for index and base.
static float halton(int32_t index, int32_t base)
{
    float f = 1.0f, result = 0.0f;

    for (int32_t currentIndex = index; currentIndex > 0;) {

        f /= (float)base;
        result = result + f * (float)(currentIndex % base);
        currentIndex = (uint32_t)(floorf((float)(currentIndex) / (float)(base)));
    }

    return result;
}

static FfxErrorCode patchResourceBindings(FfxPipelineState* inoutPipeline)
{
    for (uint32_t srvIndex = 0; srvIndex < inoutPipeline->srvTextureCount; ++srvIndex)
    {
        int32_t mapIndex = 0;
        for (mapIndex = 0; mapIndex < _countof(srvResourceBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(srvResourceBindingTable[mapIndex].name, inoutPipeline->srvTextureBindings[srvIndex].name))
                break;
        }
        if (mapIndex == _countof(srvResourceBindingTable))
            return FFX_ERROR_INVALID_ARGUMENT;

        inoutPipeline->srvTextureBindings[srvIndex].resourceIdentifier = srvResourceBindingTable[mapIndex].index;
    }

    // check for UAVs where mip chains are to be bound
    for (uint32_t uavIndex = 0; uavIndex < inoutPipeline->uavTextureCount; ++uavIndex)
    {
        int32_t mapIndex = 0;
        for (mapIndex = 0; mapIndex < _countof(uavResourceBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(uavResourceBindingTable[mapIndex].name, inoutPipeline->uavTextureBindings[uavIndex].name))
                break;
        }
        if (mapIndex == _countof(uavResourceBindingTable))
            return FFX_ERROR_INVALID_ARGUMENT;

        inoutPipeline->uavTextureBindings[uavIndex].resourceIdentifier = uavResourceBindingTable[mapIndex].index;
    }
    inoutPipeline->uavTextureCount = inoutPipeline->uavTextureCount;

    for (uint32_t cbIndex = 0; cbIndex < inoutPipeline->constCount; ++cbIndex)
    {
        int32_t mapIndex = 0;
        for (mapIndex = 0; mapIndex < _countof(cbResourceBindingTable); ++mapIndex)
        {
            if (0 == wcscmp(cbResourceBindingTable[mapIndex].name, inoutPipeline->constantBufferBindings[cbIndex].name))
                break;
        }
        if (mapIndex == _countof(cbResourceBindingTable))
            return FFX_ERROR_INVALID_ARGUMENT;

        inoutPipeline->constantBufferBindings[cbIndex].resourceIdentifier = cbResourceBindingTable[mapIndex].index;
    }

    return FFX_OK;
}

static uint32_t getPipelinePermutationFlags(uint32_t contextFlags, FfxPass passId, bool fp16, bool force64, bool useLut)
{
    // work out what permutation to load.
    uint32_t flags = 0;
    flags |= (contextFlags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED) ? FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED : 0;
    flags |= (force64) ? FRAMEINTERPOLATION_SHADER_PERMUTATION_FORCE_WAVE64 : 0;
    flags |= (fp16) ? FRAMEINTERPOLATION_SHADER_PERMUTATION_ALLOW_FP16 : 0;
    return flags;
}

static FfxErrorCode createPipelineStates(FfxFrameInterpolationContext_Private* context)
{
    FFX_ASSERT(context);

    FfxPipelineDescription pipelineDescription = {};
    pipelineDescription.contextFlags           = context->contextDescription.flags;
    pipelineDescription.stage                  = FFX_BIND_COMPUTE_SHADER_STAGE;

    // Samplers
    pipelineDescription.samplerCount      = 2;
    FfxSamplerDescription samplerDescs[2] = {
        {FFX_FILTER_TYPE_MINMAGMIP_POINT, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_BIND_COMPUTE_SHADER_STAGE},
        {FFX_FILTER_TYPE_MINMAGMIP_LINEAR, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_ADDRESS_MODE_CLAMP, FFX_BIND_COMPUTE_SHADER_STAGE}};
    pipelineDescription.samplers = samplerDescs;

    // Root constants
    pipelineDescription.rootConstantBufferCount     = 2;
    FfxRootConstantDescription rootConstantDescs[2] = 
    {
        {sizeof(FrameInterpolationConstants) / sizeof(uint32_t), FFX_BIND_COMPUTE_SHADER_STAGE},
        {sizeof(InpaintingPyramidConstants) / sizeof(uint32_t), FFX_BIND_COMPUTE_SHADER_STAGE}
    };
    pipelineDescription.rootConstants               = rootConstantDescs;

    // Query device capabilities
    FfxDeviceCapabilities capabilities;
    context->contextDescription.backendInterface.fpGetDeviceCapabilities(&context->contextDescription.backendInterface, &capabilities);

    // Setup a few options used to determine permutation flags
    bool haveShaderModel66 = capabilities.minimumSupportedShaderModel >= FFX_SHADER_MODEL_6_6;
    bool supportedFP16     = capabilities.fp16Supported;
    bool canForceWave64    = false;
    bool useLut            = false;

    const uint32_t waveLaneCountMin = capabilities.waveLaneCountMin;
    const uint32_t waveLaneCountMax = capabilities.waveLaneCountMax;
    if (waveLaneCountMin == 32 && waveLaneCountMax == 64)
    {
        useLut         = true;
        canForceWave64 = haveShaderModel66;
    }
    else
        canForceWave64 = false;

    // Work out what permutation to load.
    uint32_t contextFlags = context->contextDescription.flags;

    // Set up pipeline descriptor (basically RootSignature and binding)
    auto CreateComputePipeline = [&](FfxPass pass, const wchar_t* name, FfxPipelineState* pipeline) -> FfxErrorCode {
        ffxSafeReleasePipeline(&context->contextDescription.backendInterface, pipeline, context->effectContextId);
        wcscpy_s(pipelineDescription.name, name);
        FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(
            &context->contextDescription.backendInterface,
            FFX_EFFECT_FRAMEINTERPOLATION,
            pass,
            getPipelinePermutationFlags(contextFlags, pass, supportedFP16, canForceWave64, useLut),
            &pipelineDescription,
            context->effectContextId,
            pipeline));
        patchResourceBindings(pipeline);

        return FFX_OK;
    };

    auto CreateRasterPipeline = [&](FfxPass pass, const wchar_t* name, FfxPipelineState* pipeline) -> FfxErrorCode {
        wcscpy_s(pipelineDescription.name, name);
        pipelineDescription.stage            = (FfxBindStage)(FFX_BIND_VERTEX_SHADER_STAGE | FFX_BIND_PIXEL_SHADER_STAGE);
        pipelineDescription.backbufferFormat = context->contextDescription.backBufferFormat;
        FFX_VALIDATE(context->contextDescription.backendInterface.fpCreatePipeline(
            &context->contextDescription.backendInterface,
            FFX_EFFECT_FRAMEINTERPOLATION,
            pass,
            getPipelinePermutationFlags(contextFlags, pass, supportedFP16, canForceWave64, useLut),
            &pipelineDescription,
            context->effectContextId,
            pipeline));

        return FFX_OK;
    };

    // Frame Interpolation Pipelines
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_SETUP,                                L"SETUP", &context->pipelineFiSetup);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_RECONSTRUCT_PREV_DEPTH,               L"RECONSTRUCT_PREV_DEPTH", &context->pipelineFiReconstructPreviousDepth);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_GAME_MOTION_VECTOR_FIELD,             L"GAME_MOTION_VECTOR_FIELD", &context->pipelineFiGameMotionVectorField);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_OPTICAL_FLOW_VECTOR_FIELD,            L"OPTICAL_FLOW_VECTOR_FIELD", &context->pipelineFiOpticalFlowVectorField);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_DISOCCLUSION_MASK,                    L"DISOCCLUSION_MASK", &context->pipelineFiDisocclusionMask);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_INTERPOLATION,                        L"INTERPOLATION", &context->pipelineFiScfi);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_INPAINTING_PYRAMID,                   L"INPAINTING_PYRAMID", &context->pipelineInpaintingPyramid);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_INPAINTING,                           L"INPAINTING", &context->pipelineInpainting);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_GAME_VECTOR_FIELD_INPAINTING_PYRAMID, L"GAME_VECTOR_FIELD_INPAINTING_PYRAMID", & context->pipelineGameVectorFieldInpaintingPyramid);
    CreateComputePipeline(FFX_FRAMEINTERPOLATION_PASS_DEBUG_VIEW,                           L"DEBUG_VIEW", &context->pipelineDebugView);

    return FFX_OK;
}

static FfxErrorCode frameinterpolationCreate(FfxFrameInterpolationContext_Private* context, const FfxFrameInterpolationContextDescription* contextDescription)
{
    FFX_ASSERT(context);
    FFX_ASSERT(contextDescription);

    // Setup the data for implementation.
    memset(context, 0, sizeof(FfxFrameInterpolationContext_Private));
    context->device = contextDescription->backendInterface.device;

    memcpy(&context->contextDescription, contextDescription, sizeof(FfxFrameInterpolationContextDescription));

        // Create the context.
    FfxErrorCode errorCode = context->contextDescription.backendInterface.fpCreateBackendContext(&context->contextDescription.backendInterface, &context->effectContextId);
    FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);

    // call out for device caps.
    errorCode = context->contextDescription.backendInterface.fpGetDeviceCapabilities(&context->contextDescription.backendInterface, &context->deviceCapabilities);
    FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);

    // set defaults
    context->firstExecution = true;

    context->constants.maxRenderSize[0]         = contextDescription->maxRenderSize.width;
    context->constants.maxRenderSize[1]         = contextDescription->maxRenderSize.height;
    context->constants.displaySize[0]           = contextDescription->displaySize.width;
    context->constants.displaySize[1]           = contextDescription->displaySize.height;
    context->constants.displaySizeRcp[0]        = 1.0f / contextDescription->displaySize.width;
    context->constants.displaySizeRcp[1]        = 1.0f / contextDescription->displaySize.height;
    context->constants.interpolationRectBase[0] = 0;
    context->constants.interpolationRectBase[1] = 0;
    context->constants.interpolationRectSize[0] = contextDescription->displaySize.width;
    context->constants.interpolationRectSize[1] = contextDescription->displaySize.height;

    // generate the data for the LUT.
    const uint32_t lanczos2LutWidth = 128;
    int16_t lanczos2Weights[lanczos2LutWidth] = { };

    for (uint32_t currentLanczosWidthIndex = 0; currentLanczosWidthIndex < lanczos2LutWidth; currentLanczosWidthIndex++) {

        const float x = 2.0f * currentLanczosWidthIndex / float(lanczos2LutWidth - 1);
        const float y = lanczos2(x);
        lanczos2Weights[currentLanczosWidthIndex] = int16_t(roundf(y * 32767.0f));
    }

    uint8_t defaultReactiveMaskData = 0U;
    uint32_t atomicInitData[2] = { 0, 0 };
    float defaultExposure[] = { 0.0f, 0.0f };
    const FfxResourceType texture1dResourceType = (context->contextDescription.flags & FFX_FRAMEINTERPOLATION_ENABLE_TEXTURE1D_USAGE) ? FFX_RESOURCE_TYPE_TEXTURE1D : FFX_RESOURCE_TYPE_TEXTURE2D;

    // declare internal resources needed
    const FfxInternalResourceDescription internalSurfaceDesc[] = {

        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME, L"FI_ReconstructedDepthInterpolatedFrame",  FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            FFX_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFX_RESOURCE_FLAGS_NONE},
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X,             L"FI_GameMotionVectorFieldX",               FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV, 
            FFX_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFX_RESOURCE_FLAGS_NONE},
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y,             L"FI_GameMotionVectorFieldY",               FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            FFX_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFX_RESOURCE_FLAGS_NONE},
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID,                     L"FI_InpaintingPyramid",                    FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT, contextDescription->displaySize.width / 2, contextDescription->displaySize.height / 2, 0, FFX_RESOURCE_FLAGS_ALIASABLE },
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS,                               L"FI_Counters",                             FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            FFX_SURFACE_FORMAT_R32_UINT, 2, 1, 1, FFX_RESOURCE_FLAGS_ALIASABLE, sizeof(atomicInitData), atomicInitData },
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X,     L"FI_OpticalFlowMotionVectorFieldX",        FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            FFX_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFX_RESOURCE_FLAGS_NONE},
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y,     L"FI_OpticalFlowMotionVectorFieldY",        FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            FFX_SURFACE_FORMAT_R32_UINT, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFX_RESOURCE_FLAGS_NONE},
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PREVIOUS_INTERPOLATION_SOURCE,          L"FI_PreviousInterpolationSouce",           FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
            contextDescription->backBufferFormat, contextDescription->displaySize.width, contextDescription->displaySize.height, 1, FFX_RESOURCE_FLAGS_NONE},
        {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK,                      L"FI_DisocclusionMask",                     FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV, 
            FFX_SURFACE_FORMAT_R8G8_UNORM, contextDescription->maxRenderSize.width, contextDescription->maxRenderSize.height, 1, FFX_RESOURCE_FLAGS_NONE},
    };

    // clear the SRV resources to NULL.
    memset(context->srvResources, 0, sizeof(context->srvResources));

    for (int32_t currentSurfaceIndex = 0; currentSurfaceIndex < FFX_ARRAY_ELEMENTS(internalSurfaceDesc); ++currentSurfaceIndex) {

        const FfxInternalResourceDescription* currentSurfaceDescription = &internalSurfaceDesc[currentSurfaceIndex];
        const FfxResourceType resourceType = currentSurfaceDescription->height > 1 ? FFX_RESOURCE_TYPE_TEXTURE2D : texture1dResourceType;
        const FfxResourceDescription resourceDescription = { resourceType, currentSurfaceDescription->format, currentSurfaceDescription->width, currentSurfaceDescription->height, 1, currentSurfaceDescription->mipCount, FFX_RESOURCE_FLAGS_NONE, currentSurfaceDescription->usage };
        FfxResourceStates initialState = FFX_RESOURCE_STATE_UNORDERED_ACCESS;
        if (currentSurfaceDescription->usage == FFX_RESOURCE_USAGE_READ_ONLY) initialState = FFX_RESOURCE_STATE_COMPUTE_READ;
        if (currentSurfaceDescription->usage == FFX_RESOURCE_USAGE_RENDERTARGET) initialState = FFX_RESOURCE_STATE_COMMON;

        const FfxCreateResourceDescription createResourceDescription = { FFX_HEAP_TYPE_DEFAULT, resourceDescription, initialState, currentSurfaceDescription->initDataSize, currentSurfaceDescription->initData, currentSurfaceDescription->name, currentSurfaceDescription->id };

        FFX_VALIDATE(context->contextDescription.backendInterface.fpCreateResource(&context->contextDescription.backendInterface, &createResourceDescription, context->effectContextId, &context->srvResources[currentSurfaceDescription->id]));
    }

    // copy resources to uavResrouces list
    memcpy(context->uavResources, context->srvResources, sizeof(context->srvResources));

    // avoid compiling pipelines on first render
    {
        context->refreshPipelineStates = false;
        errorCode = createPipelineStates(context);
        FFX_RETURN_ON_ERROR(errorCode == FFX_OK, errorCode);
    }
    return FFX_OK;
}

static FfxErrorCode frameinterpolationRelease(FfxFrameInterpolationContext_Private* context)
{
    FFX_ASSERT(context);

    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineFiSetup, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineFiReconstructPreviousDepth, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineFiGameMotionVectorField, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineFiOpticalFlowVectorField, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineFiDisocclusionMask, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineFiScfi, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineInpaintingPyramid, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineInpainting, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineGameVectorFieldInpaintingPyramid, context->effectContextId);
    ffxSafeReleasePipeline(&context->contextDescription.backendInterface, &context->pipelineDebugView, context->effectContextId);

    // unregister resources not created internally
    context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE]         = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};
    context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR]                   = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};
    context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_CONFIDENCE]               = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};
    context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_GLOBAL_MOTION]            = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};
    context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCENE_CHANGE_DETECTION]   = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};
    context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT]                                = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};
    context->uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT]                                = {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_NULL};

    // Release the copy resources for those that had init data
    ffxSafeReleaseCopyResource(&context->contextDescription.backendInterface, context->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS], context->effectContextId);

    // release internal resources
    for (int32_t currentResourceIndex = 0; currentResourceIndex < FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNT; ++currentResourceIndex) {

        ffxSafeReleaseResource(&context->contextDescription.backendInterface, context->srvResources[currentResourceIndex], context->effectContextId);
    }

    // Destroy the context
    context->contextDescription.backendInterface.fpDestroyBackendContext(&context->contextDescription.backendInterface, context->effectContextId);

    return FFX_OK;
}

static void scheduleDispatch(FfxFrameInterpolationContext_Private* context, const FfxPipelineState* pipeline, uint32_t dispatchX, uint32_t dispatchY)
{
    FfxComputeJobDescription jobDescriptor = {};

    for (uint32_t currentShaderResourceViewIndex = 0; currentShaderResourceViewIndex < pipeline->srvTextureCount; ++currentShaderResourceViewIndex)
    {
        const uint32_t            currentResourceId               = pipeline->srvTextureBindings[currentShaderResourceViewIndex].resourceIdentifier;
        const FfxResourceInternal currentResource = context->srvResources[currentResourceId];
        jobDescriptor.srvTextures[currentShaderResourceViewIndex] = currentResource;
        wcscpy_s(jobDescriptor.srvTextureNames[currentShaderResourceViewIndex], pipeline->srvTextureBindings[currentShaderResourceViewIndex].name);
    }

    for (uint32_t currentUnorderedAccessViewIndex = 0; currentUnorderedAccessViewIndex < pipeline->uavTextureCount; ++currentUnorderedAccessViewIndex) {

        const uint32_t currentResourceId = pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].resourceIdentifier;
        wcscpy_s(jobDescriptor.uavTextureNames[currentUnorderedAccessViewIndex], pipeline->uavTextureBindings[currentUnorderedAccessViewIndex].name);

        if (currentResourceId >= FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_0 && currentResourceId <= FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_12)
        {
            const FfxResourceInternal currentResource = context->uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID];
            jobDescriptor.uavTextures[currentUnorderedAccessViewIndex] = currentResource;
            jobDescriptor.uavTextureMips[currentUnorderedAccessViewIndex] = currentResourceId - FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_0;
        }
        else
        {
            const FfxResourceInternal currentResource = context->uavResources[currentResourceId];
            jobDescriptor.uavTextures[currentUnorderedAccessViewIndex] = currentResource;
            jobDescriptor.uavTextureMips[currentUnorderedAccessViewIndex] = 0;
        }
    }

    jobDescriptor.dimensions[0] = dispatchX;
    jobDescriptor.dimensions[1] = dispatchY;
    jobDescriptor.dimensions[2] = 1;
    jobDescriptor.pipeline = *pipeline;

    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < pipeline->constCount; ++currentRootConstantIndex) {
        wcscpy_s(jobDescriptor.cbNames[currentRootConstantIndex], pipeline->constantBufferBindings[currentRootConstantIndex].name);
        jobDescriptor.cbs[currentRootConstantIndex] = globalFrameInterpolationConstantBuffers[pipeline->constantBufferBindings[currentRootConstantIndex].resourceIdentifier];
    }

    FfxGpuJobDescription dispatchJob = { FFX_GPU_JOB_COMPUTE };
    dispatchJob.computeJobDescriptor = jobDescriptor;

    context->contextDescription.backendInterface.fpScheduleGpuJob(&context->contextDescription.backendInterface, &dispatchJob);
}

FfxErrorCode ffxFrameInterpolationContextCreate(FfxFrameInterpolationContext* context, FfxFrameInterpolationContextDescription* contextDescription)
{
    // zero context memory
    //memset(context, 0, sizeof(FfxFrameinterpolationContext));

    // check pointers are valid.
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        contextDescription,
        FFX_ERROR_INVALID_POINTER);

    // validate that all callbacks are set for the interface
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpGetDeviceCapabilities, FFX_ERROR_INCOMPLETE_INTERFACE);
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpCreateBackendContext, FFX_ERROR_INCOMPLETE_INTERFACE);
    FFX_RETURN_ON_ERROR(contextDescription->backendInterface.fpDestroyBackendContext, FFX_ERROR_INCOMPLETE_INTERFACE);

    // if a scratch buffer is declared, then we must have a size
    if (contextDescription->backendInterface.scratchBuffer) {

        FFX_RETURN_ON_ERROR(contextDescription->backendInterface.scratchBufferSize, FFX_ERROR_INCOMPLETE_INTERFACE);
    }

    // ensure the context is large enough for the internal context.
    FFX_STATIC_ASSERT(sizeof(FfxFrameInterpolationContext) >= sizeof(FfxFrameInterpolationContext_Private));

    // create the context.
    FfxFrameInterpolationContext_Private* contextPrivate = (FfxFrameInterpolationContext_Private*)(context);
    FfxErrorCode errorCode = frameinterpolationCreate(contextPrivate, contextDescription);

    return errorCode;
}

FfxErrorCode ffxFrameInterpolationContextDestroy(FfxFrameInterpolationContext* context)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);

    // destroy the context.
    FfxFrameInterpolationContext_Private* contextPrivate = (FfxFrameInterpolationContext_Private*)(context);
    const FfxErrorCode      errorCode      = frameinterpolationRelease(contextPrivate);

    return errorCode;
}

FfxErrorCode ffxFrameInterpolationContextEnqueueRefreshPipelineRequest(FfxFrameInterpolationContext* context)
{
    FFX_RETURN_ON_ERROR(
        context,
        FFX_ERROR_INVALID_POINTER);

    FfxFrameInterpolationContext_Private* contextPrivate = (FfxFrameInterpolationContext_Private*)context;
    contextPrivate->refreshPipelineStates = true;

    return FFX_OK;
}

static void setupDeviceDepthToViewSpaceDepthParams(FfxFrameInterpolationContext_Private* context, const FfxFrameInterpolationRenderDescription* params, FrameInterpolationConstants* constants)
{
    const bool bInverted = (context->contextDescription.flags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED) == FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED;
    const bool bInfinite = (context->contextDescription.flags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE) == FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE;

    // make sure it has no impact if near and far plane values are swapped in dispatch params
    // the flags "inverted" and "infinite" will decide what transform to use
    float fMin = FFX_MINIMUM(params->cameraNear, params->cameraFar);
    float fMax = FFX_MAXIMUM(params->cameraNear, params->cameraFar);

    if (bInverted) {
        float tmp = fMin;
        fMin = fMax;
        fMax = tmp;
    }

    // a 0 0 0   x
    // 0 b 0 0   y
    // 0 0 c d   z
    // 0 0 e 0   1

    const float fQ = fMax / (fMin - fMax);
    const float d = -1.0f; // for clarity

    const float matrix_elem_c[2][2] = {
        fQ,                     // non reversed, non infinite
        -1.0f - FLT_EPSILON,    // non reversed, infinite
        fQ,                     // reversed, non infinite
        0.0f + FLT_EPSILON      // reversed, infinite
    };

    const float matrix_elem_e[2][2] = {
        fQ * fMin,              // non reversed, non infinite
        -fMin - FLT_EPSILON,    // non reversed, infinite
        fQ * fMin,              // reversed, non infinite
        fMax,                   // reversed, infinite
    };

    constants->deviceToViewDepth[0] = d * matrix_elem_c[bInverted][bInfinite];
    constants->deviceToViewDepth[1] = matrix_elem_e[bInverted][bInfinite] * params->viewSpaceToMetersFactor;

    // revert x and y coords
    const float aspect = params->renderSize.width / float(params->renderSize.height);
    const float cotHalfFovY = cosf(0.5f * params->cameraFovAngleVertical) / sinf(0.5f * params->cameraFovAngleVertical);
    const float a = cotHalfFovY / aspect;
    const float b = cotHalfFovY;

    constants->deviceToViewDepth[2] = (1.0f / a);
    constants->deviceToViewDepth[3] = (1.0f / b);
    
}

static const float debugBarColorSequence[] = {
    0.0f, 1.0f, 1.0f,   // teal
    1.0f, 0.42f, 0.0f,  // orange
    0.0f, 0.16f, 1.0f,  // blue
    0.74f, 1.0f, 0.0f,  // lime
    0.68f, 0.0f, 1.0f,  // purple
    0.0f, 1.0f, 0.1f,   // green
    1.0f, 1.0f, 0.48f   // bright yellow
};
const size_t debugBarColorSequenceLength = 7;

FFX_API FfxErrorCode ffxFrameInterpolationDispatch(FfxFrameInterpolationContext* context, const FfxFrameInterpolationDispatchDescription* params)
{
    FfxFrameInterpolationContext_Private*         contextPrivate = (FfxFrameInterpolationContext_Private*)(context);
    const FfxFrameInterpolationRenderDescription* renderDesc     = &contextPrivate->renderDescription;

    if (contextPrivate->refreshPipelineStates) {

        createPipelineStates(contextPrivate);
        contextPrivate->refreshPipelineStates = false;
    }

    contextPrivate->constants.renderSize[0]         = params->renderSize.width;
    contextPrivate->constants.renderSize[1]         = params->renderSize.height;
    contextPrivate->constants.displaySize[0]        = params->displaySize.width;
    contextPrivate->constants.displaySize[1]        = params->displaySize.height;
    contextPrivate->constants.displaySizeRcp[0]     = 1.0f / params->displaySize.width;
    contextPrivate->constants.displaySizeRcp[1]     = 1.0f / params->displaySize.height;
    contextPrivate->constants.upscalerTargetSize[0] = params->displaySize.width;
    contextPrivate->constants.upscalerTargetSize[1] = params->displaySize.height;
    contextPrivate->constants.Mode                  = 0;
    contextPrivate->constants.Reset                 = params->reset;
    contextPrivate->constants.deltaTime             = params->frameTimeDelta;
    contextPrivate->constants.HUDLessAttachedFactor = params->currentBackBuffer_HUDLess.resource ? 1 : 0;

    contextPrivate->constants.opticalFlowScale[0]   = params->opticalFlowScale.x;
    contextPrivate->constants.opticalFlowScale[1]   = params->opticalFlowScale.y;
    contextPrivate->constants.opticalFlowBlockSize  = params->opticalFlowBlockSize;// displaySize.width / params->opticalFlowBufferSize.width;
    contextPrivate->constants.dispatchFlags         = params->flags;

    contextPrivate->constants.cameraNear            = params->cameraNear;
    contextPrivate->constants.cameraFar             = params->cameraFar;

    contextPrivate->constants.interpolationRectBase[0] = params->interpolationRect.left;
    contextPrivate->constants.interpolationRectBase[1] = params->interpolationRect.top;
    contextPrivate->constants.interpolationRectSize[0] = params->interpolationRect.width;
    contextPrivate->constants.interpolationRectSize[1] = params->interpolationRect.height;

    // Debug bar
    static size_t dbgIdx = 0;
    memcpy(contextPrivate->constants.debugBarColor, &debugBarColorSequence[dbgIdx * 3], 3 * sizeof(float));
    dbgIdx = (dbgIdx + 1) % debugBarColorSequenceLength;

    contextPrivate->constants.backBufferTransferFunction = params->backBufferTransferFunction;
    contextPrivate->constants.minMaxLuminance[0]         = params->minMaxLuminance[0];
    contextPrivate->constants.minMaxLuminance[1]         = params->minMaxLuminance[1];

    const float aspectRatio                             = (float)params->renderSize.width / (float)params->renderSize.height;
    const float cameraAngleHorizontal                   = atan(tan(params->cameraFovAngleVertical / 2) * aspectRatio) * 2;
    contextPrivate->constants.fTanHalfFOV               = tanf(cameraAngleHorizontal * 0.5f);

    contextPrivate->renderDescription.cameraFar                 = params->cameraFar;
    contextPrivate->renderDescription.cameraNear                = params->cameraNear;
    contextPrivate->renderDescription.viewSpaceToMetersFactor = (params->viewSpaceToMetersFactor > 0.0f) ? params->viewSpaceToMetersFactor : 1.0f;
    contextPrivate->renderDescription.cameraFovAngleVertical    = params->cameraFovAngleVertical;
    contextPrivate->renderDescription.renderSize                = params->renderSize;
    contextPrivate->renderDescription.upscaleSize               = params->displaySize;
    setupDeviceDepthToViewSpaceDepthParams(contextPrivate, renderDesc, &contextPrivate->constants);

    memcpy(&globalFrameInterpolationConstantBuffers[FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_IDENTIFIER].data,
       &contextPrivate->constants,
           globalFrameInterpolationConstantBuffers[FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_IDENTIFIER].num32BitEntries * sizeof(uint32_t));

    if (contextPrivate->constants.HUDLessAttachedFactor == 1) {

        FFX_ASSERT_MESSAGE(params->currentBackBuffer.description.format == params->currentBackBuffer_HUDLess.description.format, "HUDLess and Backbuffer resource formats have to be identical.");

        contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->currentBackBuffer, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PRESENT_BACKBUFFER]);
        contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->currentBackBuffer_HUDLess, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE]);
    }
    else {
        contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->currentBackBuffer, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE]);
    }
    
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->dilatedDepth, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_DEPTH]);
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->dilatedMotionVectors, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS]);
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->reconstructPrevNearDepth, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME]);

    // Register output as SRV and UAV
    contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->output, contextPrivate->effectContextId, &contextPrivate->uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT]);
    contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT] = contextPrivate->uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT];

    // set optical flow buffers
    if (params->opticalFlowScale.x > 0)
    {
        contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->opticalFlowVector, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR]);
        contextPrivate->contextDescription.backendInterface.fpRegisterResource(&contextPrivate->contextDescription.backendInterface, &params->opticalFlowSceneChangeDetection, contextPrivate->effectContextId, &contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCENE_CHANGE_DETECTION]);
    }
    else
    {
        contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_CONFIDENCE]             = {};
        contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_GLOBAL_MOTION]          = {};
        contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCENE_CHANGE_DETECTION] = {};
        contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR] = {};
    }

    uint32_t displayDispatchSizeX = uint32_t(params->displaySize.width + 7) / 8;
    uint32_t displayDispatchSizeY = uint32_t(params->displaySize.height + 7) / 8;

    uint32_t renderDispatchSizeX = uint32_t(params->renderSize.width + 7) / 8;
    uint32_t renderDispatchSizeY = uint32_t(params->renderSize.height + 7) / 8;

    uint32_t opticalFlowDispatchSizeX = uint32_t(params->displaySize.width / float(params->opticalFlowBlockSize) + 7) / 8;
    uint32_t opticalFlowDispatchSizeY = uint32_t(params->displaySize.height / float(params->opticalFlowBlockSize) + 7) / 8;
    // Schedule work for the interpolation command list
    {
        // clear estimated depth resources
        {
            FfxGpuJobDescription clearJob = {FFX_GPU_JOB_CLEAR_FLOAT};

            const bool  bInverted = (contextPrivate->contextDescription.flags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED) == FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED;
            const float clearDepthValue[]{bInverted ? 0.f : 1.f, bInverted ? 0.f : 1.f, bInverted ? 0.f : 1.f, bInverted ? 0.f : 1.f};
            memcpy(clearJob.clearJobDescriptor.color, clearDepthValue, 4 * sizeof(float));

            clearJob.clearJobDescriptor.target = contextPrivate->uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME];
            contextPrivate->contextDescription.backendInterface.fpScheduleGpuJob(&contextPrivate->contextDescription.backendInterface, &clearJob);
        }

        scheduleDispatch(contextPrivate, &contextPrivate->pipelineFiSetup, renderDispatchSizeX, renderDispatchSizeY);
        scheduleDispatch(contextPrivate, &contextPrivate->pipelineFiReconstructPreviousDepth, renderDispatchSizeX, renderDispatchSizeY);
        scheduleDispatch(contextPrivate, &contextPrivate->pipelineFiGameMotionVectorField, renderDispatchSizeX, renderDispatchSizeY);

        // game vector field inpainting pyramid
        auto scheduleDispatchGameVectorFieldInpaintingPyramid = [&]() {
            // Auto exposure
            uint32_t dispatchThreadGroupCountXY[2];
            uint32_t workGroupOffset[2];
            uint32_t numWorkGroupsAndMips[2];
            uint32_t rectInfo[4] = {0, 0, params->renderSize.width, params->renderSize.height};
            ffxSpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);

            // downsample
            contextPrivate->inpaintingPyramidContants.numworkGroups      = numWorkGroupsAndMips[0];
            contextPrivate->inpaintingPyramidContants.mips               = numWorkGroupsAndMips[1];
            contextPrivate->inpaintingPyramidContants.workGroupOffset[0] = workGroupOffset[0];
            contextPrivate->inpaintingPyramidContants.workGroupOffset[1] = workGroupOffset[1];

            memcpy(&globalFrameInterpolationConstantBuffers[FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER].data,
                    &contextPrivate->inpaintingPyramidContants,
                    globalFrameInterpolationConstantBuffers[FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER].num32BitEntries *
                        sizeof(uint32_t));

            scheduleDispatch(contextPrivate, &contextPrivate->pipelineGameVectorFieldInpaintingPyramid, dispatchThreadGroupCountXY[0], dispatchThreadGroupCountXY[1]);
        };
        scheduleDispatchGameVectorFieldInpaintingPyramid();

        scheduleDispatch(contextPrivate, &contextPrivate->pipelineFiOpticalFlowVectorField, opticalFlowDispatchSizeX, opticalFlowDispatchSizeY);

        scheduleDispatch(contextPrivate, &contextPrivate->pipelineFiDisocclusionMask, renderDispatchSizeX, renderDispatchSizeY);
        scheduleDispatch(contextPrivate, &contextPrivate->pipelineFiScfi, displayDispatchSizeX, displayDispatchSizeY);

        // inpainting pyramid
        {
            // Auto exposure
            uint32_t dispatchThreadGroupCountXY[2];
            uint32_t workGroupOffset[2];
            uint32_t numWorkGroupsAndMips[2];
            uint32_t rectInfo[4] = { 0, 0, params->displaySize.width, params->displaySize.height };
            ffxSpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);

            // downsample
            contextPrivate->inpaintingPyramidContants.numworkGroups      = numWorkGroupsAndMips[0];
            contextPrivate->inpaintingPyramidContants.mips               = numWorkGroupsAndMips[1];
            contextPrivate->inpaintingPyramidContants.workGroupOffset[0] = workGroupOffset[0];
            contextPrivate->inpaintingPyramidContants.workGroupOffset[1] = workGroupOffset[1];

            memcpy(&globalFrameInterpolationConstantBuffers[FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER].data,
                &contextPrivate->inpaintingPyramidContants,
                globalFrameInterpolationConstantBuffers[FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER].num32BitEntries * sizeof(uint32_t));

            scheduleDispatch(contextPrivate, &contextPrivate->pipelineInpaintingPyramid, dispatchThreadGroupCountXY[0], dispatchThreadGroupCountXY[1]);
        }

        scheduleDispatch(contextPrivate, &contextPrivate->pipelineInpainting, displayDispatchSizeX, displayDispatchSizeY);

        if (params->flags & FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_VIEW)
        {
            scheduleDispatchGameVectorFieldInpaintingPyramid();
            scheduleDispatch(contextPrivate, &contextPrivate->pipelineDebugView, displayDispatchSizeX, displayDispatchSizeY);
        }

        // store current buffer
        {
            FfxGpuJobDescription copyJobs[] = { {FFX_GPU_JOB_COPY} };
            FfxResourceInternal  copySources[_countof(copyJobs)] = { contextPrivate->srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE] };
            FfxResourceInternal destSources[_countof(copyJobs)] = { contextPrivate->uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PREVIOUS_INTERPOLATION_SOURCE] };

            for (int i = 0; i < _countof(copyJobs); ++i)
            {
                copyJobs[i].copyJobDescriptor.src = copySources[i];
                copyJobs[i].copyJobDescriptor.dst = destSources[i];
                contextPrivate->contextDescription.backendInterface.fpScheduleGpuJob(&contextPrivate->contextDescription.backendInterface, &copyJobs[i]);
            }
        }

        // declare internal resources needed
        struct FfxInternalResourceStates
        {
            FfxUInt32 id;
            FfxResourceUsage usage;
        };
        const FfxInternalResourceStates internalSurfaceDesc[] = {

            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME, FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X,             FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y,             FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID,                     FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS,                               FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X,     FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y,     FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PREVIOUS_INTERPOLATION_SOURCE,          FFX_RESOURCE_USAGE_UAV},
            {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK,                      FFX_RESOURCE_USAGE_UAV},
        };

        for (int32_t currentSurfaceIndex = 0; currentSurfaceIndex < FFX_ARRAY_ELEMENTS(internalSurfaceDesc); ++currentSurfaceIndex) {

            const FfxInternalResourceStates* currentSurfaceDescription = &internalSurfaceDesc[currentSurfaceIndex];
            FfxResourceStates initialState = FFX_RESOURCE_STATE_UNORDERED_ACCESS;
            if (currentSurfaceDescription->usage == FFX_RESOURCE_USAGE_READ_ONLY) initialState = FFX_RESOURCE_STATE_COMPUTE_READ;
            if (currentSurfaceDescription->usage == FFX_RESOURCE_USAGE_RENDERTARGET) initialState = FFX_RESOURCE_STATE_COMMON;

            FfxGpuJobDescription barrier = {FFX_GPU_JOB_BARRIER};
            barrier.barrierDescriptor.resource = contextPrivate->srvResources[currentSurfaceDescription->id];
            barrier.barrierDescriptor.subResourceID = 0;
            barrier.barrierDescriptor.newState = (currentSurfaceDescription->id == FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS) ? FFX_RESOURCE_STATE_COPY_DEST : initialState;
            barrier.barrierDescriptor.barrierType = FFX_BARRIER_TYPE_TRANSITION;
            contextPrivate->contextDescription.backendInterface.fpScheduleGpuJob(&contextPrivate->contextDescription.backendInterface, &barrier);
        }

        // schedule optical flow and frame interpolation
        contextPrivate->contextDescription.backendInterface.fpExecuteGpuJobs(&contextPrivate->contextDescription.backendInterface, params->commandList);
    }

    // release dynamic resources
    contextPrivate->contextDescription.backendInterface.fpUnregisterResources(&contextPrivate->contextDescription.backendInterface, params->commandList, contextPrivate->effectContextId);

    return FFX_OK;
}
