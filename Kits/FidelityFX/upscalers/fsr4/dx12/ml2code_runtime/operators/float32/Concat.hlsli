// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier,
    const Tensor1f<SA> a,
    const Tensor1f<SB> b,
    const Tensor1f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (axisSpecifier != AxisSpecifier::C)
        return; // Non-channel concat not implemented

    const uint perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint x = 0; x < perThreadWork; x++)
    {
        const int po = workOffset + x;
        if (!ValidPosition(output, po))
            continue;

        float v;
        if (po < a.logicalSize)
            v = Load1f(a, po);
        else
            v = Load1f(b, po - a.logicalSize);

        Store1f(output, po, v);
    }
}

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier,
    const Tensor3f<SA> a,
    const Tensor3f<SB> b,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (axisSpecifier != AxisSpecifier::C)
        return; // Non-channel concat not implemented

    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint z = 0 ; z < perThreadWork.z; z++)
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
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