// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void InnerConv2D_k3p1b(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    float4 accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = Load4hA(bias, f);

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
            {
                int3 psBase = int3(piBase.xy + uint2(kx, ky), 0);
                if (!ValidPosition(input, psBase, false))
                    continue;

                accumulator[f % 4] += PipelinedDot(input.OffsetOf(psBase), input, weights.OffsetOf(uint4(kx, ky, 0, f)), weights, weights.logicalSize.z);
            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, half4(accumulator));
        }
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1b(
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
    const uint numFeatures = weights.logicalSize.w;

    const bool needSliceBounds = NeedSliceBounds(perThreadWork, computeShaderParams.numThreads, output.threadGroupSliceSize);
    const bool needGlobalBounds = NeedGlobalBounds(output.threadGroupSliceStart, output.threadGroupSliceSize, output.logicalSize);

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase, needSliceBounds, needGlobalBounds))
                continue;

            const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

            if (IsAligned(numFeatures, 4))
                InnerConv2D_k3p1b(piBase, poBase, numFeatures, input, weights, bias, output);
        }
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1b(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        half inputScale;
        half weightsScale;

        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        Tensor1h<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p) ; }
        half4 LoadInput4(int3 p) { return unpack_s8s16(Load4i8A(input, p)) * inputScale; }
        half4 LoadWeight4(int4 p) { return unpack_s8s16(Load4i8A(weights, p)) * weightsScale;  }

        half Dot(half4 ws, half4 vs, half acc)
        {
            return dot(ws, vs) + acc;
        }

        void StoreOutput4(int3 p, half4 vs)
        {
            Store4hA(output, p, vs);
        }
    };

    Algorithm algorithm = {
        input.quantizationScale,
        weights.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<half4, half4, half4>(algorithm, 1, 1, 1, 1, 1, 1, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1b(
    const Tensor3h_NHWC< SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        half weightsScale;

        uint4 kernelSize;
        Tensor3h_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        Tensor1h<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p); }
        half4 LoadInput4(int3 p) { return Load4hA(input, p); }
        half4 LoadWeight4(int4 p) { return unpack_s8s16(Load4i8A(weights, p)) * weightsScale;  }

        half Dot(half4 ws, half4 vs, half acc)
        {
            return dot(ws, vs) + acc;
        }

        void StoreOutput4(int3 p, half4 vs)
        {
            Store4hA(output, p, vs);
        }
    };

    Algorithm algorithm = {
        weights.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<half4, half4, half4>(algorithm, 1, 1, 1, 1, 1, 1, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}