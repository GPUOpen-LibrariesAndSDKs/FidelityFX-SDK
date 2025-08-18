// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SI, typename SW, typename SO>
void ConvTranspose2D_k2(
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint2 ks = 2; // kernel size
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint m = 0 ; m != perThreadWork.z; ++m)
        for (uint y = 0 ; y != perThreadWork.y; ++y)
            for (uint x = 0 ; x != perThreadWork.x; ++x)
            {
                const int3 po = workOffset + int3(x, y, m);
                if (!ValidPosition(output, po))
                    continue;

                float accumulator = 0;
                [unroll]
                for (int c = 0 ; c != weights.logicalSize.w; ++c)
                    [unroll]
                    for (int ky = 0 ; ky < ks.y; ++ky)
                        [unroll]
                        for (int kx = 0 ; kx < ks.x; ++kx)
                        {
                            const uint2 pk = ks - 1 - int2(kx, ky);
                            const float w = Load1f(weights, uint4(pk, po.z, c));

                            const int3 ps = int3(po - pk, c);
                            if (ValidPosition(input, ps))
                                accumulator += w*Load1f(input, ps);
                        }

                Store1f(output, po, accumulator);
            }
}



