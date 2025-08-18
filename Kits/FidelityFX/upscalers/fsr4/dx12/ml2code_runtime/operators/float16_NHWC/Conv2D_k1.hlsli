// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SO>
void InnerConv4_16_32_k1(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    half accumulators[32];
    for (uint f = 0 ; f != 32; ++f)
        accumulators[f] = 0;

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            half4 vs[4];
            for (uint c = 0; c != weights.logicalSize.z; c += 4)
            {
                int3 ps = piBase + uint3(kx, ky, c);
                vs[c >> 2] = Load4hA(input, ps);
            }

            for (uint f = 0; f != numFeatures; ++f )
            {
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f);
                    const half4 ws = Load4hA(weights, pw);

                    accumulators[f] += dot(ws, vs[c >> 2]);
                }
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]));
    }
}

template<typename SI, typename SW, typename SO>
void InnerConv4_k1(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    if (input.logicalSize.z == 16 && numFeatures == 32)
        return InnerConv4_16_32_k1(piBase, poBase, numFeatures, input, weights, output);

    half4 accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = 0;

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
                accumulator[f % 4] += PipelinedDot(input.OffsetOf(piBase + uint3(kx, ky, 0)), input, weights.OffsetOf(uint4(kx, ky, 0, f)), weights, weights.logicalSize.z);

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, half4(accumulator[0], accumulator[1], accumulator[2], accumulator[3]));
        }
    }
}

template<typename SI, typename SW, typename SO>
void Conv2D_k1(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
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

    if (!IsAligned(numFeatures, 4))
        return;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            const int3 piBase = int3(poBase.xy*convStrides, 0);

            if (IsAligned(numFeatures, 4))
                InnerConv4_k1(piBase, poBase, numFeatures, input, weights, output);
        }
}
