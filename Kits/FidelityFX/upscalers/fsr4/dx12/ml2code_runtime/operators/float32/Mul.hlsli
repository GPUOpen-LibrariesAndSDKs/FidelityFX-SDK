// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"


template<typename SI, typename SI2, typename SO>
void Mul(const Tensor3f<SI> input, const Tensor1f<SI2> input2, const Tensor3f<SO> output, const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const int3 workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;

    const half multiplicator = Load1f(input2, 0);

    for (int c = 0; c < perThreadWork.z; c++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + int3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                Store1f(output, po, Load1f(input, po) * multiplicator);
            }
}

template<typename SI, typename SI2, typename SO>
void Mul(const Tensor3f<SI> input, const Tensor3f<SI2> input2, const Tensor3f<SO> output, const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const int3 workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;

    for (int c = 0; c < perThreadWork.z; c++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + int3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const float a = Load1f(input, po);
                const float b = Load1f(input2, po);
                Store1f(output, po, a * b);
            }
}