// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

#include "ml2code_runtime/operators/float32/Add.hlsli"

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor1f<SA> a,
    const Tensor3f_NHWC<SB> b,
    const Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    const float addend = Load1f(a, 0);

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
            for (uint c = 0 ; c < perThreadWork.z; c++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (any(po >= output.logicalSize))
                    continue;

                Store1f(output, po, addend + Load1f(b, po));
            }
}

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor3f_NHWC<SA> a,
    const Tensor1f<SB> b,
    const Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    const float addend = Load1f(b, 0);

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
            for (uint c = 0 ; c < perThreadWork.z; c++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (any(po >= output.logicalSize))
                    continue;

                Store1f(output, po, Load1f(a, po) + addend);
            }
}

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor3f_NHWC<SA> a,
    const Tensor3f_NHWC<SB> b,
    const Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
            for (uint c = 0 ; c < perThreadWork.z; c++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (any(po >= output.logicalSize))
                    continue;

                Store1f(output, po, Load1f(a, po) + Load1f(b, po));
            }
}