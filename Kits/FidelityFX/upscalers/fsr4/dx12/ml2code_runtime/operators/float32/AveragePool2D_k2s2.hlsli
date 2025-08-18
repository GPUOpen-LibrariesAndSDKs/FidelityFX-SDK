// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SI, typename SO>
void AveragePool2D_k2s2(
    const Tensor3f<SI> input,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const uint2 strides = 2;

    if (any(threadWorkStart >= output.threadGroupSliceSize) || any(threadWorkStart >= output.logicalSize))
        return;

    const uint numChannels = perThreadWork.z;
    for (uint c = 0; c < numChannels; c++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + int3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const uint3 basePi = uint3(po.xy * strides, po.z);

                float2 sum = Load2f(input, basePi);
                sum += Load2f(input, basePi + uint3(0, 1, 0));

                Store1f(output, po, (sum.x + sum.y) * 0.25);
            }
}