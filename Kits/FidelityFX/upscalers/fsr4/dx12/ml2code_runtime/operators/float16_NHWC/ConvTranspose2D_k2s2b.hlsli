// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/operators/templates/ConvTranspose2D.hlsli"


template<typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = 2;
    const uint2 strides = 2;

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

            if (perThreadWork.z == 14)
            {
                uint z;
                for (z = 0; z < 12; z+=4)
                {
                    const int3 po = int3(poBase2D, z);

                    half4 accumulator = Load4hA(bias, po.z);

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

                const int3 po = int3(poBase2D, z);

                half2 accumulator = Load2hA(bias, po.z);

                uint inputOffset = input.OffsetOf(uint3(ps, 0));
                uint weightsOffset = weights.OffsetOf(uint4(pk, po.z, 0));
                [unroll]
                for (uint c = 0 ; c != weights.logicalSize.w; c += 2)
                {
                    const half2 vs = Load2hAOffset(input, inputOffset);
                    inputOffset += 2*input.storageByteStrides.z;

                    const half2 w0 = Load2hAOffset(weights, weightsOffset);
                    weightsOffset += weights.storageByteStrides.w;
                    const half2 w1 = Load2hAOffset(weights, weightsOffset);
                    weightsOffset += weights.storageByteStrides.w;

                    accumulator += w0 * vs.x +
                        w1 * vs.y;
                }

                Store2hA(output, po, accumulator);

                continue;
            }

            [unroll]
            for (uint z = 0; z < perThreadWork.z; z+=4)
            {
                const int3 po = int3(poBase2D, z);

                half4 accumulator = Load4hA(bias, po.z);

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


template<uint NumChannels, typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D_k2s2b_NHWC_HWCN_44h(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
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
            // if (!ValidPosition(input, int3(ps, 0)))
            //     continue;

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

                Store4hA(output, po, accumulator);
            }
        }
}

template<typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (weights.logicalSize.w == 16)
        return ConvTranspose2D_k2s2b_NHWC_HWCN_44h<16>(input, weights, bias, output, computeShaderParams);

    if (weights.logicalSize.w == 32)
        return ConvTranspose2D_k2s2b_NHWC_HWCN_44h<32>(input, weights, bias, output, computeShaderParams);

    if (weights.logicalSize.w == 48)
        return ConvTranspose2D_k2s2b_NHWC_HWCN_44h<48>(input, weights, bias, output, computeShaderParams);

    if (weights.logicalSize.w == 64)
        return ConvTranspose2D_k2s2b_NHWC_HWCN_44h<64>(input, weights, bias, output, computeShaderParams);
}

template<uint NumChannels, typename SW, typename SB>
half4 ct2d_iteration_4(
    const uint z,
    const int2 pk,
    const uint4 packedVss[NumChannels / 16],
    const QuantizedTensor4i8_HWCN<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const float outputScale)
{
    int4 accumulator = Load4i32(bias, z);
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

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

    return half4(accumulator * outputScale.xxxx);
}

template<uint NumChannels, typename SW, typename SB>
half4 ct2d_iteration_4(
    const uint z,
    const int2 pk,
    const uint4 packedVss[NumChannels / 16],
    const QuantizedTensor4i8_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const float inputsScale)
{
    int4 accumulator = 0;
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

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

    return half4(accumulator * inputsScale.xxxx + Load4hA(bias, z));
}


template<typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D(
    const uint beginX, const uint beginY,
    const uint endX, const uint endY,
    const uint dilationX, const uint dilationY,
    const uint strideX, const uint strideY,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (beginX || beginY || endX || endY || dilationX != 1 || dilationY != 1)
        return; // Padding and dilation not implemented

    if (strideX != 2 || strideY != 2 || weights.logicalSize.x != 2 || weights.logicalSize.y != 2)
        return; // only 2x2 kernels and 2x2 strides for now

      struct Algorithm {
        half biasScale;
        half inputScale;
        half weightsScale;

        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        QuantizedTensor1i32<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4i32(bias, p) * biasScale; }
        half4 LoadInput4(int3 p) { return unpack_s8s16(Load4i8A(input, p)) * inputScale; }
        half4 LoadWeight4(int4 p) { return unpack_s8s16(Load4i8A(weights, p)) * weightsScale; }
        half Dot(half4 ws, half4 vs, half acc) { return dot(ws, vs) + acc; }
        void StoreOutput4(int3 p, half4 vs) { Store4hA(output, p, vs + LoadBias4(p.z));}
    };

    Algorithm algorithm = {
        bias.quantizationScale,
        input.quantizationScale,
        weights.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConvTranspose2D_2x2_2x2_3413_NHWC_44<half4, half4, half4>(algorithm, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}

template<uint NumChannels, typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D_2x2_2x2_NHWC_HWCN_44i8(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint2 strides = uint2(2, 2);
    const float outputScale = input.quantizationScale * weights.quantizationScale;

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

            // Specialcase last ct2d
            if (perThreadWork.z == 12)
            {
                const half4 first = ct2d_iteration_4<NumChannels>(0, pk, packedVss, weights, bias, outputScale);
                const half4 second = ct2d_iteration_4<NumChannels>(4, pk, packedVss, weights, bias, outputScale);
                // Store4
                const uint storeOffset = output.OffsetOf(int3(poBase2D, 0));
                uint4 storeDwords;
                storeDwords.xy = Pack4h(first);
                storeDwords.zw = Pack4h(second);
                output.storage.Store4(storeOffset, storeDwords);

                const half4 result = ct2d_iteration_4<NumChannels>(8, pk, packedVss, weights, bias, outputScale);
                Store4hA(output, int3(poBase2D, 8), result);
            }
            else if (IsAligned(perThreadWork.z, 8))
            {
                [unroll]
                for (uint z = 0; z < perThreadWork.z; z += 8)
                {
                    const half4 first = ct2d_iteration_4<NumChannels>(z, pk, packedVss, weights, bias, outputScale);
                    const half4 second = ct2d_iteration_4<NumChannels>(z+4, pk, packedVss, weights, bias, outputScale);
                    // Store4
                    const uint storeOffset = output.OffsetOf(int3(poBase2D, z));
                    uint4 storeDwords;
                    storeDwords.xy = Pack4h(first);
                    storeDwords.zw = Pack4h(second);
                    output.storage.Store4(storeOffset, storeDwords);
                }
            }

        }
}

template<uint NumChannels, typename SW, typename SB>
half2 ct2d_iteration_2(
    const uint z,
    const int2 pk,
    const uint4 packedVss[NumChannels / 16],
    const QuantizedTensor4i8_HWCN<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const float inputsScale)
{
    int2 accumulator = Load2i32(bias, z);
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

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

    return half2(accumulator * inputsScale.xx);
}

template<uint NumChannels, typename SW, typename SB>
half2 ct2d_iteration_2(
    const uint z,
    const int2 pk,
    const uint4 packedVss[NumChannels / 16],
    const QuantizedTensor4i8_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const float inputsScale)
{
    int2 accumulator = 0;
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

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

    return half2(accumulator * inputsScale.xx + Load2hA(bias, z));
}

template<uint NumChannels, typename SI, typename SW, typename SB, typename SO>
void ConvTranspose2D_2x2_2x2_NHWC_HWCN_44i8(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = weights.logicalSize.xy; // kernel size
    const uint2 strides = uint2(2, 2);
    const float outputScale = input.quantizationScale * weights.quantizationScale;

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

            if (perThreadWork.z == 14)
            {
                const half4 first = ct2d_iteration_4<NumChannels>(0, pk, packedVss, weights, bias, outputScale);
                const half4 second = ct2d_iteration_4<NumChannels>(4, pk, packedVss, weights, bias, outputScale);
                // Store4
                uint storeOffset = output.OffsetOf(int3(poBase2D, 0));
                uint4 storeDwords;
                storeDwords.xy = Pack4h(first);
                storeDwords.zw = Pack4h(second);
                output.storage.Store4(storeOffset, storeDwords);

                storeOffset = output.OffsetOf(int3(poBase2D, 8));
                {
                    const half4 result = ct2d_iteration_4<NumChannels>(8, pk, packedVss, weights, bias, outputScale);
                    storeDwords.xy = Pack4h(result);
                    //Store4hA(output, int3(poBase2D, 8), result);
                }

                {
                    const half2 result = ct2d_iteration_2<NumChannels>(12, pk, packedVss, weights, bias, outputScale);
                    storeDwords.z = Pack2h(result.xy);
                    //Store2hA(output, int3(poBase2D, 12), result.xy);
                }
                output.storage.Store3(storeOffset, storeDwords.xyz);
            }
            else if (perThreadWork.z == 12)
            {
                const half4 first = ct2d_iteration_4<NumChannels>(0, pk, packedVss, weights, bias, outputScale);
                const half4 second = ct2d_iteration_4<NumChannels>(4, pk, packedVss, weights, bias, outputScale);
                // Store4
                const uint storeOffset = output.OffsetOf(int3(poBase2D, 0));
                uint4 storeDwords;
                storeDwords.xy = Pack4h(first);
                storeDwords.zw = Pack4h(second);
                output.storage.Store4(storeOffset, storeDwords);

                const half4 result = ct2d_iteration_4<NumChannels>(8, pk, packedVss, weights, bias, outputScale);
                Store4hA(output, int3(poBase2D, 8), result);
            }
            else if (IsAligned(perThreadWork.z, 8))
            {
                [unroll]
                for (uint z = 0; z < perThreadWork.z; z += 8)
                {
                    const half4 first = ct2d_iteration_4<NumChannels>(z, pk, packedVss, weights, bias, outputScale);
                    const half4 second = ct2d_iteration_4<NumChannels>(z+4, pk, packedVss, weights, bias, outputScale);
                    // Store4
                    const uint storeOffset = output.OffsetOf(int3(poBase2D, z));
                    uint4 storeDwords;
                    storeDwords.xy = Pack4h(first);
                    storeDwords.zw = Pack4h(second);
                    output.storage.Store4(storeOffset, storeDwords);
                }
            }

        }
}

template<typename SI, typename SW, typename SO, typename BIAS>
void ConvTranspose2D_k2s2b(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const BIAS bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (weights.logicalSize.w == 16)
        return ConvTranspose2D_2x2_2x2_NHWC_HWCN_44i8<16>(input, weights, bias, output, computeShaderParams);

    if (weights.logicalSize.w == 32)
        return ConvTranspose2D_2x2_2x2_NHWC_HWCN_44i8<32>(input, weights, bias, output, computeShaderParams);
    if (weights.logicalSize.w == 48)
        return ConvTranspose2D_2x2_2x2_NHWC_HWCN_44i8<48>(input, weights, bias, output, computeShaderParams);
        
    if (weights.logicalSize.w == 64)
        return ConvTranspose2D_2x2_2x2_NHWC_HWCN_44i8<64>(input, weights, bias, output, computeShaderParams);
}
