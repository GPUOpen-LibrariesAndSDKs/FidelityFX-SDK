// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D(
    const uint beginX, const uint beginY,
    const uint endX, const uint endY,
    const uint dilationX, const uint dilationY,
    const uint strideX, const uint strideY,
    const Tensor3f_NHWC<SI> input,
    const Tensor4f_NHWC<SW> weights,
    const Tensor1f<SB> bias,
    const Tensor3f_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint4 pads = uint4(beginX, beginY, endX, endY);
    if (any(pads != 0))
        return; // not implemented

    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint y = 0 ; y != perThreadWork.y; ++y)
        for (uint x = 0 ; x != perThreadWork.x; ++x)
            for (uint m = 0 ; m != perThreadWork.z; ++m)
            {
                const int3 po = workOffset + int3(x, y, m);
                if (!ValidPosition(output, po))
                    continue;

                if (strideX == 1 && strideY == 1)
                {
                    float accumulator = Load1f(bias, po.z);
                    [unroll]
                    for (int ky = 0 ; ky < ks.y; ++ky)
                        [unroll]
                        for (int kx = 0 ; kx < ks.x; ++kx)
                            [unroll]
                            for (int c = 0 ; c != weights.logicalSize.w; ++c)
                            {
                                const uint2 pk = ks - 1 - int2(kx, ky);
                                const float w = Load1f(weights, uint4(pk, po.z, c));

                                const int3 ps = int3(po - pk, c);
                                if (ValidPosition(input, ps))
                                    accumulator += w*Load1f(input, ps);
                            }

                    Store1f(output, po, accumulator);
                }
                else
                {
                    // TODO: this needs to be verified to generate sane ISA. It is trivial to translate to bit shifts and masks for power-of-2 strides
                    const uint2 convStrides = uint2(strideX, strideY);

                    float accumulator = Load1f(bias, po.z);
                    [unroll]
                    for (int ky = 0 ; ky < ks.y; ++ky)
                        [unroll]
                        for (int kx = 0 ; kx < ks.x; ++kx)
                            [unroll]
                            for (int c = 0 ; c != weights.logicalSize.w; ++c)
                            {
                                const int2 pk = ks - 1 - int2(kx, ky);
                                const float w = Load1f(weights, uint4(pk, po.z, c));

                                const int2 stridedPs = po + uint2(kx, ky) - (ks/2) - (ks & 1);
                                if (any((stridedPs.xy % convStrides))) // accesses must be aligned to convStrides
                                    continue;

                                const int3 ps = int3(stridedPs / convStrides, c);
                                if (ValidPosition(input, ps))
                                    accumulator += w * Load1f(input, ps);
                            }

                    Store1f(output, po, accumulator);
                }
            }
}



