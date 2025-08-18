// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"


template<typename SI, typename SW, typename SO>
void Conv2D_k2s2(
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = 2;
    const int beginX = 0;
    const int beginY = 0;

    for (uint z = 0 ; z < perThreadWork.z ; z++)
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const int3 pi = int3(po.xy*convStrides, 0) - int3(beginX, beginY, 0);

                float accumulator = 0;
                [unroll]
                for (uint c = 0; c != weights.logicalSize.z; ++c)
                    [unroll]
                    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
                        [unroll]
                        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
                        {
                            const uint4 pw = uint4(kx, ky, c, po.z); // position in weights
                            const float w = Load1f(weights, pw);

                            const int3 ps = pi + pw.xyz; // position in sample in input
                            if (ValidPosition(input, ps))
                                accumulator += w * Load1f(input, ps);
                        }

                Store1f(output, po, accumulator);
            }
}