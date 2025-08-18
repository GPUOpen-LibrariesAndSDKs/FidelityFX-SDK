// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"

template<typename SI, typename SI2, typename SW, typename SB, typename SO>
void FusedConvTranspose2D_Add(
    const uint beginX, const uint beginY,
    const uint endX, const uint endY,
    const uint dilationX, const uint dilationY,
    const uint strideX, const uint strideY,
    const Tensor3h_NHWC<SI> input,
    const Tensor3h_NHWC<SI2> inputAdd,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint4 pads = uint4(beginX, beginY, endX, endY);
    if (any(pads != 0))
        return; // not implemented

    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    if (strideX !=2 || strideY !=2)
        return; // not implemented

    if (any(weights.logicalSize.xy != 2))
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint2 strides = uint2(strideX, strideY);

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int2 poBase2D = workOffset.xy + int2(x, y);
            // if (!ValidPosition(output, uint3(poBase2D, 0)))
            //     continue;

            const int2 offsetPoBase2D = poBase2D - 1;
            const int2 poBaseMisalignment = offsetPoBase2D & 0x1; // poBase % stride;
            const int2 kxy = poBaseMisalignment;
            const int2 ps = (poBase2D + kxy) / 2;
            const int2 pk = 1 - kxy;
            // if (!ValidPosition(input, int3(ps, 0)))
            //     continue;

            [unroll]
            for (uint z = 0; z < perThreadWork.z; z+=4)
            {
                const int3 po = int3(poBase2D, z);

                half4 accumulator = Load4hA(bias, po.z);

                accumulator += Load4hA(inputAdd, po);

                uint inputOffset = input.OffsetOf(uint3(ps, 0));
                uint weightsOffset = weights.OffsetOf(uint4(pk, po.z, 0));
                [unroll]
                for (uint c = 0 ; c != weights.logicalSize.w; c += 4)
                {
                    const half4 vs = Load4hAOffset(input, inputOffset);
                    inputOffset += 4*input.storageByteStrides.z;

                    const half4 w0 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += weights.storageByteStrides.w;
                    const half4 w1 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += weights.storageByteStrides.w;
                    const half4 w2 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += weights.storageByteStrides.w;
                    const half4 w3 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += weights.storageByteStrides.w;

                    accumulator += w0 * vs.x +
                        w1 * vs.y +
                        w2 * vs.z +
                        w3 * vs.w;
                }

                Store4hA(output, po, accumulator);
            }
        }
}

template<typename SI, typename SI2, typename SW, typename SB, typename SO>
void FusedConvTranspose2D_k2s2b_Add(
    const Tensor3h_NHWC<SI> input,
    const Tensor3h_NHWC<SI2> add_input,
    const Tensor4h_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint NumChannels = 64;
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint2 strides = uint2(2, 2);

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int2 poBase2D = workOffset.xy + int2(x, y);
            if (!ValidPosition(output, uint3(poBase2D, 0)))
                continue;

            const int2 offsetPoBase2D = poBase2D - 1;
            const int2 poBaseMisalignment = offsetPoBase2D & 0x1; // poBase % stride;
            const int2 kxy = poBaseMisalignment;
            const int2 ps = (poBase2D + kxy) / 2;
            const int2 pk = 1 - kxy;

            uint inputOffset = input.OffsetOf(uint3(ps, 0));

            half4 vss[NumChannels / 4];
            for (uint c = 0 ; c != NumChannels; c += 4)
            {
                vss[c/4] = Load4hAOffset(input, inputOffset);
                inputOffset += 4*input.storageByteStrides.z;
            }

            [unroll]
            for (uint z = 0; z != perThreadWork.z; z+=4)
            {
                const int3 po = int3(poBase2D, z);

                half4 accumulator = Load4hA(bias, po.z);
                uint weightsOffset = weights.OffsetOf(uint4(pk, po.z, 0));

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 4)
                {
                    const half4 w0 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += 4*weights.storageByteStrides.w;
                    accumulator.x += dot(w0, vss[c / 4]);
                }

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 4)
                {
                    const half4 w1 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += 4*weights.storageByteStrides.w;
                    accumulator.y += dot(w1, vss[c / 4]);
                }

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 4)
                {
                    const half4 w2 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += 4*weights.storageByteStrides.w;
                    accumulator.z += dot(w2, vss[c / 4]);
                }

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 4)
                {
                    const half4 w3 = Load4hAOffset(weights, weightsOffset);
                    weightsOffset += 4*weights.storageByteStrides.w;
                    accumulator.w += dot(w3, vss[c / 4]);
                }

                Store4hA(output, po, half4(accumulator) + Load4hA(add_input, po));
            }
        }
}