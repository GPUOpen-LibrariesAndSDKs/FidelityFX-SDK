// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SI, typename S, typename O, typename SO>
void QuantizeLinear(
    const Tensor3h_NHWC<SI> input,
    const S,  // This is already baked into the output tensors quantization factors
    const O,  // This is already baked into the output tensors quantization factors (and is assumed to be 0 for now)
    QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    if (perThreadWork.z % 4)
        return; // Must be aligned to 4

    const float rcpScale = 1.0 / output.quantizationScale;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + uint3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            for (uint c = 0 ; c != perThreadWork.z; c += 4)
            {
                const int3 po = poBase + int3(0, 0, c);

                const half4 vs = Load4hA(input, po);
                const int8_t4_packed packed = pack_clamp_s8(int4(round(vs * rcpScale)));
                Store4i8A(output, po, packed);
            }
        }
}

template<typename SI, typename S, typename O, typename SO>
void QuantizeLinear(
    const Tensor3f_NHWC<SI> input,
    const S,  // This is already baked into the output tensors quantization factors
    const O,  // This is already baked into the output tensors quantization factors (and is assumed to be 0 for now)
    QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    if (perThreadWork.z % 4)
        return; // Must be aligned to 4

    const float rcpScale = 255.0 / output.quantizationScale;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + uint3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            for (uint c = 0 ; c != perThreadWork.z; c += 4)
            {
                const int3 po = poBase + int3(0, 0, c);

                const float4 vs = Load4f(input, po);
                const int8_t4_packed packed = pack_clamp_s8(int4(vs * rcpScale));
                Store4i8A(output, po, packed);
            }
        }
}