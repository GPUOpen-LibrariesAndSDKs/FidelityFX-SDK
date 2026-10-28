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

#include "../../api/include/ffx_api.hpp"
#include "ffx_framegeneration.h"

// Helper types for header initialization. Api definition is in .h file.

namespace ffx
{

template<>
struct struct_type<ffxCreateContextDescFrameGeneration> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION>
{};

struct CreateContextDescFrameGeneration : public InitHelper<ffxCreateContextDescFrameGeneration>
{};

template<>
struct struct_type<ffxConfigureDescFrameGeneration> : std::integral_constant<uint64_t, FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION>
{};

struct ConfigureDescFrameGeneration : public InitHelper<ffxConfigureDescFrameGeneration>
{};

template<>
struct struct_type<ffxDispatchDescFrameGeneration> : std::integral_constant<uint64_t, FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION>
{};

struct DispatchDescFrameGeneration : public InitHelper<ffxDispatchDescFrameGeneration>
{};

#pragma FFX_PRAGMA_WARNING_PUSH
#pragma FFX_PRAGMA_WARNING_DISABLE_DEPRECATIONS

template<>
struct FFX_DEPRECATION("ffxDispatchDescFrameGenerationPrepare is deprecated, use ffxDispatchDescFrameGeneration" FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX) struct_type<ffxDispatchDescFrameGenerationPrepare> : std::integral_constant<uint64_t, FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE>
{};

struct FFX_DEPRECATION("DispatchDescFrameGenerationPrepare is deprecated, use DispatchDescFrameGeneration" FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX) DispatchDescFrameGenerationPrepare : public InitHelper<ffxDispatchDescFrameGenerationPrepare>
{};

#pragma FFX_PRAGMA_WARNING_POP

template<>
struct struct_type<ffxConfigureDescFrameGenerationKeyValue> : std::integral_constant<uint64_t, FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_KEYVALUE>
{};

struct ConfigureDescFrameGenerationKeyValue : public InitHelper<ffxConfigureDescFrameGenerationKeyValue>
{};

template<>
struct struct_type<ffxQueryDescFrameGenerationGetGPUMemoryUsage> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE>
{};

struct QueryDescFrameGenerationGetGPUMemoryUsage : public InitHelper<ffxQueryDescFrameGenerationGetGPUMemoryUsage>
{};

template<>
struct struct_type<ffxConfigureDescFrameGenerationRegisterDistortionFieldResource> : std::integral_constant<uint64_t, FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE>
{};

struct ConfigureDescFrameGenerationRegisterDistortionFieldResource : public InitHelper<ffxConfigureDescFrameGenerationRegisterDistortionFieldResource>
{};

template<>
struct struct_type<ffxCreateContextDescFrameGenerationHudless> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_HUDLESS>
{};

struct CreateContextDescFrameGenerationHudless : public InitHelper<ffxCreateContextDescFrameGenerationHudless>
{};

#pragma FFX_PRAGMA_WARNING_PUSH
#pragma FFX_PRAGMA_WARNING_DISABLE_DEPRECATIONS

template<>
struct FFX_DEPRECATION("ffxDispatchDescFrameGenerationPrepareCameraInfo is deprecated, use ffxDispatchDescFrameGeneration" FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX) struct_type<ffxDispatchDescFrameGenerationPrepareCameraInfo> : std::integral_constant<uint64_t, FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_CAMERAINFO>
{};

struct FFX_DEPRECATION("DispatchDescFrameGenerationPrepareCameraInfo is deprecated, use DispatchDescFrameGeneration" FFX_FRAMEGENERATION_PREPARE_REPLACEMENT_SUFFIX) DispatchDescFrameGenerationPrepareCameraInfo : public InitHelper<ffxDispatchDescFrameGenerationPrepareCameraInfo>
{};

#pragma FFX_PRAGMA_WARNING_POP

template<>
struct struct_type<ffxQueryDescFrameGenerationGetGPUMemoryUsageV2> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2>
{};

struct QueryDescFrameGenerationGetGPUMemoryUsageV2 : public InitHelper<ffxQueryDescFrameGenerationGetGPUMemoryUsageV2>
{};

template<>
struct struct_type<ffxDispatchDescFrameGenerationPrepareV2> : std::integral_constant<uint64_t, FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2>
{};

struct DispatchDescFrameGenerationPrepareV2 : public InitHelper<ffxDispatchDescFrameGenerationPrepareV2>{};


template<>
struct struct_type<ffxCallbackDescFrameGenerationPresentPremulAlpha> : std::integral_constant<uint64_t, FFX_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT_PREMUL_ALPHA>
{};

struct CallbackDescFrameGenerationPresentPremulAlpha : public InitHelper<ffxCallbackDescFrameGenerationPresentPremulAlpha>
{};

template<>
struct struct_type<ffxCreateContextDescFrameGenerationVersion> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION>
{};

struct CreateContextDescFrameGenerationVersion : public InitHelper<ffxCreateContextDescFrameGenerationVersion>
{};


}  // namespace ffx
