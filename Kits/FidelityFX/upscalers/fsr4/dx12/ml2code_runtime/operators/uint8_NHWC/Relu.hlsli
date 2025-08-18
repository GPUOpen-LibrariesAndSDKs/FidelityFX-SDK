// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename SI, typename SO>
void PipelinedRelu(
    int3 start,
    uint numChannels,
    const Tensor3h_NHWC<SI> input,
    const QuantizedTensor3u8_NHWC<SO> output)
{
    const float rcpScale = 1.0 / output.quantizationScale;
    const uint readOffset = input.OffsetOf(start);
    const uint writeOffset = output.OffsetOf(start);

    [unroll]
    for (uint c = 0; c < numChannels; )
    {
        const uint remainder = numChannels - c;
        if (remainder >= 8)
        {
            const uint4 packedHalfs = input.storage.Load4(readOffset + c * 2);
            const uint2 packedU8 = uint2(
                pack_clamp_u8(int4(round(Unpack4h(packedHalfs.xy) * rcpScale))),
                pack_clamp_u8(int4(round(Unpack4h(packedHalfs.zw) * rcpScale)))
            );

            output.storage.Store2(writeOffset + c * 1, packedU8);

            c += 8;
        }
        else if (remainder >= 4)
        {
            const uint2 packedHalfs = input.storage.Load2(readOffset + c * 2);
            const uint8_t4_packed packedU8 = pack_clamp_u8(int4(round(Unpack4h(packedHalfs) * rcpScale)));
            output.storage.Store2(writeOffset + c * 1, packedU8);

            c += 4;
        }
    }
}

template<typename SI, typename SO>
void Relu(
    const Tensor3h_NHWC<SI> input,
    const QuantizedTensor3u8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const int3 workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            PipelinedRelu(poBase, perThreadWork.z, input, output);
        }
}