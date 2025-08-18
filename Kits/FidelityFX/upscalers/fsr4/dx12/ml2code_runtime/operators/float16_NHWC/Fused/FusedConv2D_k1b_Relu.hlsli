// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void InnerConvRelu4Padded(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    half accumulator[4] = {0, 0, 0, 0};

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if ((f % 2) == 0)
        {
            half2 bs = Load2hA(bias, f);
            accumulator[f % 4] = bs.x;
            accumulator[(f + 1) % 4] = bs.y;
        }

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
            {
                if (ValidPosition(input, int3(piBase.xy + uint2(kx, ky), 0)))
                {
                    [unroll]
                    for (uint c = 0; c != weights.logicalSize.z; c += 4)
                    {
                        const uint4 pw = uint4(kx, ky, c, f); // position in weights
                        int3 ps = piBase + pw.xyz;

                        const half4 ws = Load4hA(weights, pw);
                        const half4 vs = Load4hA(input, ps);

                        accumulator[f % 4] += dot(ws, vs);
                    }
                }
            }


        f++;
        if (f && !(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, Relu(half4(accumulator[0], accumulator[1], accumulator[2], accumulator[3])));
        }
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void InnerConvRelu4(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    half accumulator[4] = {0, 0, 0, 0};

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if ((f % 2) == 0)
        {
            half2 bs = Load2hA(bias, f);
            accumulator[f % 4] = bs.x;
            accumulator[(f + 1) % 4] = bs.y;
        }

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
                [unroll]
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f); // position in weights
                    int3 ps = piBase + pw.xyz;

                    const half4 ws = Load4hA(weights, pw);
                    const half4 vs = Load4hA(input, ps);

                    accumulator[f % 4] += dot(ws, vs);
                }

        f++;
        if (f && !(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, Relu(half4(accumulator[0], accumulator[1], accumulator[2], accumulator[3])));
        }
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void FusedConv2D_k1b_Relu(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    // each thread produces all channels
    if (computeShaderParams.numThreads.z != 1)
        return;

    // All channels are present in input
    if (input.threadGroupSliceSize.z != weights.logicalSize.z)
        return;

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = 1;
    const uint numFeatures = weights.logicalSize.w;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            const int3 piBase = int3(poBase.xy*convStrides, 0);

            if (IsAligned(numFeatures, 4))
                InnerConvRelu4(piBase, poBase, numFeatures, input, weights, bias, output);
        }
}

