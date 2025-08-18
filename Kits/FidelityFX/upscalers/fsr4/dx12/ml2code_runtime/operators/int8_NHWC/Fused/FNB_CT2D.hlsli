// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FNB_CT2D(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float fnbOutputScale,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const QuantizedTensor4i8_HWCN<SW3> ct2d_weights,
    const Tensor1h<SB3> ct2d_bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    _Static_assert(numFeatures == 32, "NumFeatures must be 32");
    _Static_assert(numGroups == 1, "NumGroups must be 1");

    const int3 workOffset = int3(computeShaderParams.dispatchThreadID.xy, 0);

    const int3 poBase = workOffset;
    const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

    // FNB
    // The packed result from the first convolution after concat with the input
    int8_t4_packed conv_result[numFeatures/4];

    // accumulator for half the number of features (the ones the first conv works on)
    int conv_accumulator[numFeatures/2];
    int3 pi = piBase;

    // Load second part of input into the conv_result
    [unroll]
    for (uint z = numFeatures / 2; z < numFeatures; z+=16)
    {
        const int3 po = int3(poBase.xy, z);
        const uint inputOffset = input.OffsetOf(po);
        const uint4 inputDwords = input.storage.Load4(inputOffset);
        conv_result[z/4]   = inputDwords.x;
        conv_result[z/4+1] = inputDwords.y;
        conv_result[z/4+2] = inputDwords.z;
        conv_result[z/4+3] = inputDwords.w;
    }

    // preloaded inputs
    int preloadedInputs[numFeatures/4/2];
    [unroll]
    for (uint i = 0; i < numFeatures/2; i+=16)
    {
        const int3 po = int3(poBase.xy, i);
        const uint inputOffset = input.OffsetOf(po);
        const uint4 inputDwords = input.storage.Load4(inputOffset);

        preloadedInputs[i/4]   = inputDwords.x;
        preloadedInputs[i/4+1] = inputDwords.y;
        preloadedInputs[i/4+2] = inputDwords.z;
        preloadedInputs[i/4+3] = inputDwords.w;
    }

    // First convolution
    [unroll]
    for (uint i = 0; i < numFeatures/2; i+=4)
    {
        const int4 b = 0;
        conv_accumulator[i]   = b.x;
        conv_accumulator[i+1] = b.y;
        conv_accumulator[i+2] = b.z;
        conv_accumulator[i+3] = b.w;
    }
    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                continue;

            int8_t4_packed vs[numFeatures/4];
            uint inputIndex = 0;
            [unroll]
            for (uint c = 0; c != weights0.logicalSize.z; c += 4)
            {
                const int3 ps = piBase + int3(kx, ky, c);
                vs[inputIndex++] = Load4i8A(input, ps);
            }

            [unroll]
            for (uint f = 0; f != numFeatures/2; ++f)
            {
                inputIndex = 0;
                for (uint c = 0; c != weights0.logicalSize.z; c += 4)
                {
                    int8_t4_packed weights = Load4i8A(weights0, uint4(kx, ky, c, f));
                    conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weights, conv_accumulator[f]);
                }
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures/2; f+=4)
    {
        const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
        const int4 quantized_vs = round((accumulator * (quantScale0 * weights0.quantizationScale) + Load4hA(bias0, f))* (1.0 / quantScale1));
        conv_result[f/4] = pack_clamp_s8(quantized_vs);
    }

    // Second Convolution + Relu
    int relu_output[numFeatures*2];
    [unroll]
    for (uint i = 0; i < numFeatures*2; i+=4)
    {
        const int4 b = 0;
        relu_output[i]   = b.x;
        relu_output[i+1] = b.y;
        relu_output[i+2] = b.z;
        relu_output[i+3] = b.w;
    }
    [unroll]
    for (uint i = 0; i < numFeatures*2; i++)
    {
        uint inputIndex = 0;
        [unroll]
        for (uint j = 0; j < numFeatures; j+=16)
        {
            const uint weightsOffset = weights1.OffsetOf(uint4(0,0,j,i));
            const uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

            relu_output[i] = dot4add_i8packed(conv_result[inputIndex++], weightsDwords.x, relu_output[i]);
            relu_output[i] = dot4add_i8packed(conv_result[inputIndex++], weightsDwords.y, relu_output[i]);
            relu_output[i] = dot4add_i8packed(conv_result[inputIndex++], weightsDwords.z, relu_output[i]);
            relu_output[i] = dot4add_i8packed(conv_result[inputIndex++], weightsDwords.w, relu_output[i]);
        }
    }

    int8_t4_packed relu_result[numFeatures*2/4];
    [unroll]
    for (uint i = 0; i < numFeatures*2; i+=4)
    {
        const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
        const int4 quantized_vs = round((vs * (quantScale1 * weights1.quantizationScale) + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
        relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
    }

    // Third Convolution
    int conv3_output[numFeatures];
    [unroll]
    for (uint i = 0; i < numFeatures; i++) { conv3_output[i] = 0; }

    [unroll]
    for (uint i = 0; i < numFeatures; i++)
    {
        uint inputIndex = 0;
        [unroll]
        for (uint j = 0; j < numFeatures*2; j+=16)
        {
            const uint weightsOffset = weights2.OffsetOf(uint4(0,0,j,i));
            const uint4 weightsDwords = weights2.storage.Load4(weightsOffset);

            conv3_output[i] = dot4add_i8packed(relu_result[inputIndex+0], weightsDwords.x, conv3_output[i]);
            conv3_output[i] = dot4add_i8packed(relu_result[inputIndex+1], weightsDwords.y, conv3_output[i]);
            conv3_output[i] = dot4add_i8packed(relu_result[inputIndex+2], weightsDwords.z, conv3_output[i]);
            conv3_output[i] = dot4add_i8packed(relu_result[inputIndex+3], weightsDwords.w, conv3_output[i]);

            inputIndex += 4;
        }
    }

    const float rcprFNBoutputQuantizationScale = 1.0 / fnbOutputScale;
    const float weightReluScale = weights2.quantizationScale * activationScaleFactor;

    uint4 packedVss[2];
    // Add
    [unroll]
    for (uint i = 0; i < numFeatures / 2; i+=4)
    {
        const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * weightReluScale + Load4hA(bias2, i);
        result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

        const int4 quantizedResult = round(result * rcprFNBoutputQuantizationScale);
        packedVss[0][i/4] = pack_clamp_s8(quantizedResult);
    }

    [unroll]
    for (uint i = numFeatures/2; i < numFeatures; i+=4)
    {
        const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * weightReluScale + Load4hA(bias2, i);
        result += unpack_s8s32(conv_result[i/4]) * quantScale1;

        const int4 quantizedResult = round(result * rcprFNBoutputQuantizationScale);
        packedVss[1][(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
    }

    // CT2D + ADD
    const float ct2d_quantizationScale = fnbOutputScale * ct2d_weights.quantizationScale;
    const int2 localOffset[4] = { int2(0,0), int2(1,0), int2(0,1), int2(1,1) };
    const float outputScale = 1.0 / output.quantizationScale;

    [unroll]
    for (int  i = 0; i < 4; ++i)
    {
        const int2 pk = localOffset[i];
        const uint2 poBase2D = computeShaderParams.dispatchThreadID.xy * 2 + pk;

        uint4 storeDwords = uint4(0,0,0,0);
        uint storageIndex = 0;

        [unroll]
        for (uint z = 0; z < 16;)
        {
            const int3 po = int3(poBase2D, z);

            int4 accumulator = 0;
            uint weightsOffset = ct2d_weights.OffsetOf(uint4(pk, po.z, 0));

            [unroll]
            for (uint c = 0 ; c != 32; c += 16)
            {
                const uint4 w0 = ct2d_weights.storage.Load4(weightsOffset);
                weightsOffset += 16;

                const uint4 packedVs = packedVss[c / 16];
                accumulator.x = dot4add_i8packed(w0.x, packedVs.x, accumulator.x);
                accumulator.x = dot4add_i8packed(w0.y, packedVs.y, accumulator.x);
                accumulator.x = dot4add_i8packed(w0.z, packedVs.z, accumulator.x);
                accumulator.x = dot4add_i8packed(w0.w, packedVs.w, accumulator.x);
            }

            [unroll]
            for (uint c = 0 ; c != 32; c += 16)
            {
                const uint4 w1 = ct2d_weights.storage.Load4(weightsOffset);
                weightsOffset += 16;

                const uint4 packedVs = packedVss[c / 16];
                accumulator.y = dot4add_i8packed(w1.x, packedVs.x, accumulator.y);
                accumulator.y = dot4add_i8packed(w1.y, packedVs.y, accumulator.y);
                accumulator.y = dot4add_i8packed(w1.z, packedVs.z, accumulator.y);
                accumulator.y = dot4add_i8packed(w1.w, packedVs.w, accumulator.y);
            }

            [unroll]
            for (uint c = 0 ; c != 32; c += 16)
            {
                const uint4 w2 = ct2d_weights.storage.Load4(weightsOffset);
                weightsOffset += 16;

                const uint4 packedVs = packedVss[c / 16];
                accumulator.z = dot4add_i8packed(w2.x, packedVs.x, accumulator.z);
                accumulator.z = dot4add_i8packed(w2.y, packedVs.y, accumulator.z);
                accumulator.z = dot4add_i8packed(w2.z, packedVs.z, accumulator.z);
                accumulator.z = dot4add_i8packed(w2.w, packedVs.w, accumulator.z);
            }


            [unroll]
            for (uint c = 0 ; c != 32; c += 16)
            {
                const uint4 w3 = ct2d_weights.storage.Load4(weightsOffset);
                weightsOffset += 16;

                const uint4 packedVs = packedVss[c / 16];
                accumulator.w = dot4add_i8packed(w3.x, packedVs.x, accumulator.w);
                accumulator.w = dot4add_i8packed(w3.y, packedVs.y, accumulator.w);
                accumulator.w = dot4add_i8packed(w3.z, packedVs.z, accumulator.w);
                accumulator.w = dot4add_i8packed(w3.w, packedVs.w, accumulator.w);
            }

            float4 result = accumulator * ct2d_quantizationScale + Load4hA(ct2d_bias, po.z);

            storeDwords[storageIndex++] = pack_clamp_s8(int4(round(result * outputScale)));
            z+=4;

            if (z%16 == 0)
            {
                output.storage.Store4(output.OffsetOf(int3(poBase2D, z-16)), storeDwords);
                storageIndex = 0;
            }
        }
    }
}

