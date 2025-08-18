// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"

template<typename SI, typename SO>
void AveragePool2D_k2s2(
    const Tensor3h_NHWC<SI> input,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const uint2 strides = 2;

    if (any(threadWorkStart >= output.threadGroupSliceSize) || any(threadWorkStart >= output.logicalSize))
        return;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
        {
            const uint numChannels = perThreadWork.z;
            for (uint c = 0; c < numChannels; c += 2)
            {
                const int3 po = workOffset + int3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const uint3 basePi = uint3(po.xy * strides, po.z);

                half2 sum = Load2hA(input, basePi);
                sum += Load2hA(input, basePi + uint3(1, 0, 0));
                sum += Load2hA(input, basePi + uint3(0, 1, 0));
                sum += Load2hA(input, basePi + uint3(1, 1, 0));

                Store2hA(output, po, sum * 0.25);
            }
        }
}