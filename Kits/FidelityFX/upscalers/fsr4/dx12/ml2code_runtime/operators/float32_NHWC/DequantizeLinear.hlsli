// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"

template<typename SI, typename S, typename O, typename SO>
void DequantizeLinear(
    const QuantizedTensor3u8_NHWC<SI> input,
    const S,  // This is already baked into the input tensors quantization factors
    const O,  // This is already baked into the input tensors quantization factors (and is assumed to be 0 for now)
    Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (output.threadGroupSliceSize.z % 4)
        return; // Must be aligned to 4

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    const float scale = input.quantizationScale;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + uint3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            for (uint c = 0 ; c != perThreadWork.z; c += 4)
            {
                const int3 po = poBase + int3(0, 0, c);

                const uint8_t4_packed packed = Load4u8A(input, po);
                const float4 vs = unpack_u8u32(packed);
                Store4f(output, po, vs * scale);
            }
        }
}


template<typename SI, typename S, typename O, typename SO>
void DequantizeLinear(
    const QuantizedTensor3i8_NHWC<SI> input,
    const S,  // This is already baked into the input tensors quantization factors
    const O,  // This is already baked into the input tensors quantization factors (and is assumed to be 0 for now)
    Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (output.threadGroupSliceSize.z % 4)
        return; // Must be aligned to 4

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    const float scale = input.quantizationScale;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + uint3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            const uint readOffset = input.OffsetOf(poBase);
            const uint writeOffset = output.OffsetOf(poBase);

            for (uint c = 0 ; c != perThreadWork.z; c += 4)
            {
                const int8_t4_packed packed = input.storage.Load1(readOffset + c);
                const float4 vs = unpack_s8s32(packed);
                output.storage.Store4(writeOffset + c * 4, asuint(vs * scale));
            }
        }
}
