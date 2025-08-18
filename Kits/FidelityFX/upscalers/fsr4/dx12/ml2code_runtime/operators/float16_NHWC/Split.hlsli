// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"

template<typename SA, typename SO1, typename SO2>
void Split(
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3h_NHWC<SA> a,
    const Tensor3h_NHWC<SO1> output0,
    const Tensor3h_NHWC<SO2> output1,
    const ComputeShaderParams computeShaderParams)
{
    if (all(output0.logicalSize != output1.logicalSize))
        return;

    {
        const uint3 workPerTick = uint3(1, 1, 2);
        const uint3 perThreadWork = SplitWork(output0.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
        const int3 workOffset = output0.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
                for (uint z = 0; z < perThreadWork.z; z+=2)
                {
                    const int3 po = workOffset + int3(x, y, z);
                    if (!ValidPosition(output0, po))
                        continue;

                    Store2hA(output0, po, Load2hA(a, po));
                }
    }

    {
        const uint3 workPerTick = uint3(1, 1, 2);
        const uint3 perThreadWork = SplitWork(output1.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
        const int3 workOffset = output1.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
                for (uint z = 0; z < perThreadWork.z; z+=2)
                {
                    const int3 po = workOffset + int3(x, y, z);
                    if (!ValidPosition(output1, po))
                        continue;

                    Store2hA(output1, po, Load2hA(a, po + int3(0,0,output0.logicalSize.z)));
                }
    }

}

template<typename SA, typename SS, typename SO1, typename SO2>
void Split(
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3h_NHWC<SA> a,
    const Tensor3h_NHWC<SO1> output0,
    const Tensor3h_NHWC<SO2> output1,
    const ComputeShaderParams computeShaderParams)
{
    if (all(output0.logicalSize != output1.logicalSize))
        return;

    {
        const uint3 workPerTick = uint3(1, 1, 2);
        const uint3 perThreadWork = SplitWork(output0.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
        const int3 workOffset = output0.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
                for (uint z = 0; z < perThreadWork.z; z+=2)
                {
                    const int3 po = workOffset + int3(x, y, z);
                    if (!ValidPosition(output0, po))
                        continue;

                    Store2hA(output0, po, Load2hA(a, po));
                }
    }

    {
        const uint3 workPerTick = uint3(1, 1, 2);
        const uint3 perThreadWork = SplitWork(output1.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
        const int3 workOffset = output1.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
                for (uint z = 0; z < perThreadWork.z; z+=2)
                {
                    const int3 po = workOffset + int3(x, y, z);
                    if (!ValidPosition(output1, po))
                        continue;

                    Store2hA(output1, po, Load2hA(a, po + int3(0,0,output0.logicalSize.z)));
                }
    }

}