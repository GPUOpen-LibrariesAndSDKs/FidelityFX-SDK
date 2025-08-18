// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SA, typename SO1, typename SO2>
void Split(
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3f<SA> a,
    const int splitNums[2],
    const Tensor3f<SO1> output0,
    const Tensor3f<SO2> output1,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint z = 0; z < perThreadWork.z; z++)
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + int3(x, y, z);

                if (!ValidPosition(output, po) || !ValidPosition(output1, po))
                    return; // TODO: Split more than just equally

                Store1f(output0, po, Load1f(a, po));
                Store1f(output1, po, Load1f(a, po + int3(0,0,16)));
            }
}