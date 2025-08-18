// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename S1, typename S2>
void Relu(const Tensor3f_NHWC<S1> input, const Tensor3f_NHWC<S2> output, const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
            for (uint c = 0 ; c < perThreadWork.z; c++)
            {
                const int3 po = workOffset + uint3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const float v = Load1f(input, po);
                Store1f(output, po, Relu(v));
            }
}
