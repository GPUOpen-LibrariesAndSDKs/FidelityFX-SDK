// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SI, typename SO>
void MaxPool2D_k2s2(
    const Tensor3f<SI> input,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const uint2 strides = 2;
    const uint sizeX = 2, sizeY = 2;

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

                const uint3 pi = uint3(po.xy * strides, po.z);

                // special case for y == 0 so ensure we dont put magic values for maxValueSeen
                float maxValueSeen = Load1f(input, pi);
                for (uint sx = 1; sx != sizeX; ++sx)
                    maxValueSeen = max(maxValueSeen, Load1f(input, pi + uint3(sx, 0, 0)));

                for (uint sy = 1; sy != sizeY; ++sy)
                    for (uint sx = 0; sx != sizeX; ++sx)
                        maxValueSeen = max(maxValueSeen, Load1f(input, pi + uint3(sx, sy, 0)));

                Store1f(output, po, maxValueSeen);
            }
}

