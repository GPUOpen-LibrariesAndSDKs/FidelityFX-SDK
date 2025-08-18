// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename S1, typename S2>
void LeakyRelu(const float alpha, const Tensor3f<S1> input, const Tensor3f<S2> output, const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = max(output.threadGroupSliceSize / computeShaderParams.numThreads, 1);
    const uint3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint c = 0 ; c < perThreadWork.z; c++)
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (any(po >= output.logicalSize))
                    continue;

                const float v = Load1f(input, po);
                Store1f(output, po, LeakyRelu(alpha, v));
            }
}
