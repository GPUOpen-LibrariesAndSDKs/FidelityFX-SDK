// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"

template<typename SI, typename SO>
void FusedSinCos(const Tensor3h_NHWC<SI> input,
    const Tensor3h_NHWC<SO> outputSin, const Tensor3h_NHWC<SO> outputCos,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(outputSin.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    if (any(threadWorkStart >= outputSin.threadGroupSliceSize))
        return;
    const int3 workOffset = outputSin.threadGroupSliceStart + threadWorkStart;
    for (int y = 0; y < perThreadWork.y; y++)
    {
        for (int x = 0; x < perThreadWork.x; x++)
        {
            for (int c = 0; c < perThreadWork.z; c += 2)
            {
                const int3 po = workOffset + int3(x, y, c);
                if (!ValidPosition(outputSin, po))
                    continue;
                const half2 v = Load2hA(input, po);
                Store2hA(outputSin, po, sin(v));
                Store2hA(outputCos, po, cos(v));
            }
        }
    }
}
