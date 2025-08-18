// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename S1, typename S2>
void Relu(const Tensor1f<S1> input, const Tensor1f<S2> output, const ComputeShaderParams computeShaderParams)
{
    const uint perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const int workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;

    for (uint x = 0; x < perThreadWork.x; x++)
    {
        const int po = workOffset + x;
        if (!ValidPosition(output, po))
            return;

        const float v = Load1f(input, po);
        Store1f(output, po, Relu(v));
    }
}

template<typename S1, typename S2>
void Relu(const Tensor2f<S1> input, const Tensor2f<S2> output, const ComputeShaderParams computeShaderParams)
{
    const uint2 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int2 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const int2 workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int2 po = workOffset + uint2(x, y);
            if (!ValidPosition(output, po))
                continue;

            const float v = Load1f(input, po);
            Store1f(output, po, Relu(v));
        }
}

template<typename S1, typename S2>
void Relu(const Tensor3f<S1> input, const Tensor3f<S2> output, const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const int3 workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;

    for (uint c = 0 ; c < perThreadWork.z; c++)
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + uint3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const float v = Load1f(input, po);
                Store1f(output, po, Relu(v));
            }
}
