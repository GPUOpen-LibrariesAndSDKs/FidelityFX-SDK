// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SI, typename SW, typename SO>
void ConvTranspose2D_stride1x1_nopad_nodilation(
    const Tensor2f<SI> input,
    const Tensor2f<SW> weights,
    const Tensor2f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint2 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int2 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize; // kernel size

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int2 po = workOffset + int2(x, y);
            if (!ValidPosition(output, po))
                continue;

            float accumulator = 0;
            for (int ky = 0 ; ky < ks.y; ++ky)
                for (int kx = 0 ; kx < ks.x; ++kx)
                {
                    const int2 pk = ks - 1 - int2(kx, ky);
                    const float w = Load1f(weights, pk);

                    const int2 ps = po - pk;
                    if (ValidPosition(input, ps))
                        accumulator += w * Load1f(input, ps);

                }

            Store1f(output, po, accumulator);
        }
}

template<typename SI, typename SW, typename SO>
void ConvTranspose2D_2x2_stride2x2_nopad_nodilation(
    const Tensor2f<SI> input,
    const Tensor2f<SW> weights,
    const Tensor2f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint2 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int2 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    const uint stride = 2;
    const uint2 stridedInputSize = input.logicalSize * stride - 1;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int2 po = workOffset + int2(x, y);
            if (!ValidPosition(output, po))
                continue;

            const int2 poBase = po - 1;
            const int2 poBaseMisalignment = poBase & 0x1; // poBase % stride;

            const int2 kxy = poBaseMisalignment;
            const int2 ps = (poBase + kxy) / 2;
            if (!ValidPosition(input, ps))
                continue;

            const int2 pk = 1 - kxy;
            const float w = Load1f(weights, pk);
            const float i = Load1f(input, ps);

            const float accumulator = w*i;
            Store1f(output, po, accumulator);
        }
}

template<typename SI, typename SW, typename SO>
void ConvTranspose2D(
    const uint beginX, const uint beginY,
    const uint endX, const uint endY,
    const uint dilationX, const uint dilationY,
    const uint strideX, const uint strideY,
    const Tensor2f<SI> input,
    const Tensor2f<SW> weights,
    const Tensor2f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint4 pads = uint4(beginX, beginY, endX, endY);
    if (any(pads != 0))
        return; // not implemented

    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    if (strideX == 2 && strideY == 2 && all(weights.logicalSize.xy == 2))
        return ConvTranspose2D_2x2_stride2x2_nopad_nodilation(input, weights, output, computeShaderParams);

    if (strideX == 1 && strideY == 1)
        return ConvTranspose2D_stride1x1_nopad_nodilation(input, weights, output, computeShaderParams);

    const uint2 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const uint2 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize; // kernel size

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int2 po = workOffset + int2(x, y);
            if (!ValidPosition(output, po))
                continue;

            // TODO: this needs to be verified to generate sane ISA. It is trivial to translate to bit shifts and masks for power-of-2 strides
            const uint2 strides = uint2(strideX, strideY);
            const uint2 stridedInputSize = input.logicalSize * strides - 1;
            const int2 poBase = po - (ks/2) - (ks & 1);

            float accumulator = 0;
            for (int ky = 0 ; ky < ks.y; ++ky)
                for (int kx = 0 ; kx < ks.x; ++kx)
                {
                    const int2 stridedPs = poBase + uint2(kx, ky);
                    if (any(stridedPs % strides)) // must be aligned to stride
                        continue;

                    const int2 pk = ks - 1 - int2(kx, ky);
                    const float w = Load1f(weights, pk);

                    const int2 ps = stridedPs / strides;
                    if (ValidPosition(input, ps))
                        accumulator += w * Load1f(input, ps);
                }

            Store1f(output, po, accumulator);
        }
}

template<typename SI, typename SW, typename SO>
void ConvTranspose2D(
    const uint beginX, const uint beginY,
    const uint endX, const uint endY,
    const uint dilationX, const uint dilationY,
    const uint strideX, const uint strideY,
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor3f<SO> output,
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

    for (uint m = 0 ; m != perThreadWork.z; ++m)
        for (uint y = 0 ; y != perThreadWork.y; ++y)
            for (uint x = 0 ; x != perThreadWork.x; ++x)
            {
                const int3 po = workOffset + int3(x, y, m);
                if (!ValidPosition(output, po))
                    continue;

                if (strideX == 1 && strideY == 1)
                {
                    float accumulator = 0;

                    [unroll]
                    for (int c = 0 ; c != weights.logicalSize.w; ++c)
                        [unroll]
                        for (int ky = 0 ; ky < ks.y; ++ky)
                            [unroll]
                            for (int kx = 0 ; kx < ks.x; ++kx)
                            {
                                const int2 pk = ks - 1 - int2(kx, ky);
                                const float w = Load1f(weights, uint4(pk, po.z, c));

                                const int2 ps = po - pk;
                                if (ValidPosition(input, ps))
                                    accumulator += w * Load1f(input, uint3(ps, c));
                            }

                    Store1f(output, po, accumulator);
                }
                else
                {
                    const uint2 convStrides = uint2(strideX, strideY);

                    float accumulator = 0;
                    [unroll]
                    for (int c = 0 ; c != weights.logicalSize.w; ++c)
                        [unroll]
                        for (int ky = 0 ; ky < ks.y; ++ky)
                            [unroll]
                            for (int kx = 0 ; kx < ks.x; ++kx)
                            {
                                const int2 pk = ks - 1 - int2(kx, ky);
                                const float w = Load1f(weights, uint4(pk, po.z, c));

                                const int2 stridedPs = po + uint2(kx, ky) - (ks/2) - (ks & 1);
                                if (any(stridedPs % convStrides)) // accesses must be aligned to convStrides
                                    continue;

                                const int3 ps = int3(stridedPs / convStrides, c);
                                if (ValidPosition(input, ps))
                                    accumulator += w * Load1f(input, ps);
                            }

                    Store1f(output, po, accumulator);
                }
            }
}


template<typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D(
    const uint beginX, const uint beginY,
    const uint endX, const uint endY,
    const uint dilationX, const uint dilationY,
    const uint strideX, const uint strideY,
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor1f<SB> bias,
    const Tensor3f<SO> output,
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

    for (uint m = 0 ; m != perThreadWork.z; ++m)
        for (uint y = 0 ; y != perThreadWork.y; ++y)
            for (uint x = 0 ; x != perThreadWork.x; ++x)
            {
                const int3 po = workOffset + int3(x, y, m);
                if (!ValidPosition(output, po))
                    continue;

                if (strideX == 1 && strideY == 1)
                {
                    float accumulator = Load1f(bias, po.z);
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
                else
                {
                    // TODO: this needs to be verified to generate sane ISA. It is trivial to translate to bit shifts and masks for power-of-2 strides
                    const uint2 convStrides = uint2(strideX, strideY);

                    float accumulator = Load1f(bias, po.z);
                    [unroll]
                    for (int c = 0 ; c != weights.logicalSize.w; ++c)
                        [unroll]
                        for (int ky = 0 ; ky < ks.y; ++ky)
                            [unroll]
                            for (int kx = 0 ; kx < ks.x; ++kx)
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



