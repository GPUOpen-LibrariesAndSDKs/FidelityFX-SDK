// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"

template<uint NumChannels, typename SI, typename SI2, typename SW, typename SB, typename SO>
void ConvTranspose2D_Add_2x2_2x2_3413_NHWC_HWCN_44i8(
    const float quantScale0,
    const float quantScale1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI2> inputAdd,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (NumChannels != 64)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint2 strides = uint2(2, 2);
    const float inputScale = input.quantizationScale * weights.quantizationScale;

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

            uint inputOffset = input.OffsetOf(uint3(ps, 0));

            uint4 packedVss[NumChannels / 16];
            for (uint c16 = 0 ; c16 != NumChannels/16; c16 += 1)
            {
                packedVss[c16] = input.storage.Load4(inputOffset);
                inputOffset += 16;
            }

            uint4 storeDwords;
            uint storageIndex = 0;
            float outputScale = 1.0 / quantScale0;
            [unroll]
            for (uint z = 0; z != perThreadWork.z;)
            {
                const int3 po = int3(poBase2D, z);

                int4 accumulator = Load4i32(bias, po.z);
                uint weightsOffset = weights.OffsetOf(uint4(pk, po.z, 0));

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 16)
                {
                    const uint4 w0 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.x = dot4add_i8packed(w0.x, packedVs.x, accumulator.x);
                    accumulator.x = dot4add_i8packed(w0.y, packedVs.y, accumulator.x);
                    accumulator.x = dot4add_i8packed(w0.z, packedVs.z, accumulator.x);
                    accumulator.x = dot4add_i8packed(w0.w, packedVs.w, accumulator.x);
                }

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 16)
                {
                    const uint4 w1 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16*weights.storageByteStrides.w;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.y = dot4add_i8packed(w1.x, packedVs.x, accumulator.y);
                    accumulator.y = dot4add_i8packed(w1.y, packedVs.y, accumulator.y);
                    accumulator.y = dot4add_i8packed(w1.z, packedVs.z, accumulator.y);
                    accumulator.y = dot4add_i8packed(w1.w, packedVs.w, accumulator.y);
                }

                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 16)
                {
                    const uint4 w2 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16*weights.storageByteStrides.w;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.z = dot4add_i8packed(w2.x, packedVs.x, accumulator.z);
                    accumulator.z = dot4add_i8packed(w2.y, packedVs.y, accumulator.z);
                    accumulator.z = dot4add_i8packed(w2.z, packedVs.z, accumulator.z);
                    accumulator.z = dot4add_i8packed(w2.w, packedVs.w, accumulator.z);
                }


                [unroll]
                for (uint c = 0 ; c != NumChannels; c += 16)
                {
                    const uint4 w3 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16*weights.storageByteStrides.w;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.w = dot4add_i8packed(w3.x, packedVs.x, accumulator.w);
                    accumulator.w = dot4add_i8packed(w3.y, packedVs.y, accumulator.w);
                    accumulator.w = dot4add_i8packed(w3.z, packedVs.z, accumulator.w);
                    accumulator.w = dot4add_i8packed(w3.w, packedVs.w, accumulator.w);
                }

                float4 result = accumulator * inputScale;
                result += unpack_s8s32(Load4i8A(inputAdd, po)) * inputAdd.quantizationScale;

                storeDwords[storageIndex++] = pack_clamp_s8(int4(round(result * outputScale)));
                z+=4;

                if (z%16 ==0)
                {
                    output.storage.Store4(output.OffsetOf(int3(poBase2D, z-16)), storeDwords);
                    storageIndex = 0;
                    outputScale = 1.0 / quantScale1;
                }
            }
        }
}

template<uint NumChannels, typename SI, typename SI2, typename SW, typename SB, typename SO>
void ConvTranspose2D_Add_2x2_2x2_3413_NHWC_HWCN_44i8(
    const float quantScale0,
    const float quantScale1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI2> inputAdd,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (NumChannels % 16 != 0)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint2 strides = uint2(2, 2);
    const float inputScale = input.quantizationScale * weights.quantizationScale;

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

            uint inputOffset = input.OffsetOf(uint3(ps, 0));

            uint4 packedVss[NumChannels / 16];
            for (uint c16 = 0 ; c16 != NumChannels/16; c16 += 1)
            {
                packedVss[c16] = input.storage.Load4(inputOffset);
                inputOffset += 16;
            }

            uint4 storeDwords;
            uint storageIndex = 0;
            const float outScale0 = 1.0 / quantScale0;
            const float outScale1 = 1.0 / quantScale1;
            [unroll]
            for (uint z = 0; z != perThreadWork.z;)
            {
                const int3 po = int3(poBase2D, z);

                int4 accumulator = 0;
                uint weightsOffset = weights.OffsetOf(uint4(pk, po.z, 0));

                [unroll]
                for (uint c = 0; c != NumChannels; c += 16)
                {
                    const uint4 w0 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.x = dot4add_i8packed(w0.x, packedVs.x, accumulator.x);
                    accumulator.x = dot4add_i8packed(w0.y, packedVs.y, accumulator.x);
                    accumulator.x = dot4add_i8packed(w0.z, packedVs.z, accumulator.x);
                    accumulator.x = dot4add_i8packed(w0.w, packedVs.w, accumulator.x);
                }

                [unroll]
                for (uint c = 0; c != NumChannels; c += 16)
                {
                    const uint4 w1 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16 * weights.storageByteStrides.w;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.y = dot4add_i8packed(w1.x, packedVs.x, accumulator.y);
                    accumulator.y = dot4add_i8packed(w1.y, packedVs.y, accumulator.y);
                    accumulator.y = dot4add_i8packed(w1.z, packedVs.z, accumulator.y);
                    accumulator.y = dot4add_i8packed(w1.w, packedVs.w, accumulator.y);
                }

                [unroll]
                for (uint c = 0; c != NumChannels; c += 16)
                {
                    const uint4 w2 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16 * weights.storageByteStrides.w;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.z = dot4add_i8packed(w2.x, packedVs.x, accumulator.z);
                    accumulator.z = dot4add_i8packed(w2.y, packedVs.y, accumulator.z);
                    accumulator.z = dot4add_i8packed(w2.z, packedVs.z, accumulator.z);
                    accumulator.z = dot4add_i8packed(w2.w, packedVs.w, accumulator.z);
                }


                [unroll]
                for (uint c = 0; c != NumChannels; c += 16)
                {
                    const uint4 w3 = weights.storage.Load4(weightsOffset);
                    weightsOffset += 16 * weights.storageByteStrides.w;

                    const uint4 packedVs = packedVss[c / 16];
                    accumulator.w = dot4add_i8packed(w3.x, packedVs.x, accumulator.w);
                    accumulator.w = dot4add_i8packed(w3.y, packedVs.y, accumulator.w);
                    accumulator.w = dot4add_i8packed(w3.z, packedVs.z, accumulator.w);
                    accumulator.w = dot4add_i8packed(w3.w, packedVs.w, accumulator.w);
                }

                float4 result = accumulator * inputScale + Load4hA(bias, po.z);
                result += unpack_s8s32(Load4i8A(inputAdd, po)) * inputAdd.quantizationScale;

                const float outScale = z < perThreadWork.z / 2 ? outScale0 : outScale1;
                storeDwords[storageIndex++] = pack_clamp_s8(int4(round(result * outScale)));
                z+=4;

                if (z%16 ==0)
                {
                    output.storage.Store4(output.OffsetOf(int3(poBase2D, z-16)), storeDwords);
                    storageIndex = 0;
                }
            }
        }
}

template<typename SI, typename SI2, typename SW, typename SO, typename BIAS>
void FusedConvTranspose2D_k2s2b_Add_QuantizedOutput(
    const float quantScale0,
    const float quantScale1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI2> inputAdd,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const BIAS bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    
    if (weights.logicalSize.w == 32)
        return ConvTranspose2D_Add_2x2_2x2_3413_NHWC_HWCN_44i8<32> (quantScale0, quantScale1, input, inputAdd, weights, bias, output, computeShaderParams);
    
    if (weights.logicalSize.w == 64)
        return ConvTranspose2D_Add_2x2_2x2_3413_NHWC_HWCN_44i8 < 64 > (quantScale0, quantScale1, input, inputAdd, weights, bias, output, computeShaderParams);
}