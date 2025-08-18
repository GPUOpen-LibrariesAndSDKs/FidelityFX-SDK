// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/operators/float32/Concat.hlsli"

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier,
    const Tensor3f_NHWC<SA> a,
    const Tensor3f_NHWC<SB> b,
    const Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (axisSpecifier != AxisSpecifier::C)
        return; // Non-channel concat not implemented

    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
            for (uint z = 0 ; z < perThreadWork.z; z++)
            {
                const int3 po = workOffset + uint3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const uint c = po.z;

                float v;
                if (c < a.logicalSize.z)
                    v = Load1f(a, po);
                else
                    v = Load1f(b, po - uint3(0, 0, a.logicalSize.z));

                Store1f(output, po, v);
            }
}