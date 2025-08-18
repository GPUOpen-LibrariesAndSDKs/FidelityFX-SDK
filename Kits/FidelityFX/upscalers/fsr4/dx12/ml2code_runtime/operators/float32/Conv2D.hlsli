// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"


template<typename SI, typename SW, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor2f<SI> input,
    const Tensor2f<SW> weights,
    const Tensor2f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    if (strideX != 1 || strideY != 1)
        return; // not implemented

    const uint2 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int2 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int2 po = workOffset + int2(x, y);
            if (!ValidPosition(output, po))
                continue;

            const int2 pi = po - int2(beginX, beginY);

            float accumulator = 0;
            [unroll]
            for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
                [unroll]
                for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
                {
                    const uint2 pw = uint2(kx, ky); // position in weights
                    const float w = Load1f(weights, pw);

                    const int2 ps = pi + pw; // position in sample in  input
                    if (ValidPosition(input, ps))
                        accumulator += w * Load1f(input, ps);
                }

            Store1f(output, po, accumulator);
        }
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor1f<SB> bias,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);

    for (uint z = 0 ; z < perThreadWork.z ; z++)
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const int3 pi = int3(po.xy*convStrides, 0) - int3(beginX, beginY, 0);

                float accumulator = Load1f(bias, po.z);
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

template<typename SI, typename SW, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);

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

template<typename SI, typename SW, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    uint groups,
    const Tensor3f<SI> input,
    const Tensor4f<SW> weights,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
     if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);
    const uint numElementsInGroup = weights.logicalSize.z;

    for (uint m = 0 ; m != perThreadWork.z; ++m)
        for (uint y = 0 ; y != perThreadWork.y; ++y)
            for (uint x = 0 ; x != perThreadWork.x; ++x)
            {
                const int3 po = workOffset + int3(x, y, m);
                if (!ValidPosition(output, po))
                    continue;

                const uint thisGroup = po.z / numElementsInGroup;
                const uint groupOffset = thisGroup * numElementsInGroup;

                const int3 pi = int3(po.xy*convStrides, groupOffset) - int3(beginX, beginY, 0);

                float accumulator = 0;
                [unroll]
                for (uint c = 0; c != numElementsInGroup; ++c)
                    [unroll]
                    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
                        [unroll]
                        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
                        {
                            const uint4 pw = uint4(kx, ky, c, po.z);
                            const float w = Load1f(weights, pw);

                            const int3 ps = pi + pw.xyz;
                            if (ValidPosition(input, ps))
                                accumulator += w * Load1f(input, ps);
                        }

                Store1f(output, po, accumulator);
            }
}
