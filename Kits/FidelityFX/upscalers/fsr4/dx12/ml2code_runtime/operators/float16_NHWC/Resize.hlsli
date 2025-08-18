// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"

template<typename SI, typename SO>
void Resize_nearest2x(
    const Tensor3h_NHWC<SI> input,
    const NullTensor /*_roi*/,
    const Tensor1h< ConstantBufferStorage<2> > /*_scales*/,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    const uint numChannels = perThreadWork.z;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
        {
            const int3 po = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, po))
                continue;

            const uint3 pi = uint3(po.xy >> 1, po.z);
            for (uint c = 0; c < numChannels; c += 2)
            {
                const half2 vs = Load2hA(input, pi + uint3(0,0,c));
                Store2hA(output, po + uint3(0,0,c), vs);
            }
        }
}
