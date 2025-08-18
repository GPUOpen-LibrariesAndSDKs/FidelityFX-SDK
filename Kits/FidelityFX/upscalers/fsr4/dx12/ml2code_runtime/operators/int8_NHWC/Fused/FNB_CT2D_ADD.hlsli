// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

#if WMMA_ENABLED
#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"
#endif

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FNB_CT2D_ADD(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float fnbOutputScale,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI> inputAdd,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const QuantizedTensor4i8_HWCN<SW3> ct2d_weights,
    const Tensor1h<SB3> ct2d_bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{

    _Static_assert(numFeatures == 64, "NumFeatures must be 64");
    _Static_assert(numGroups == 2, "NumGroups must be 2");

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

            int8_t4_packed vs[numFeatures/2/4];
            uint inputIndex = 0;
            [unroll]
            for (uint c = 0; c < weights0.logicalSize.z * 2; c += 16)
            {
                const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, c));
                const uint4 inputDwords = input.storage.Load4(inputOffset);
                vs[inputIndex++] = inputDwords.x;
                vs[inputIndex++] = inputDwords.y;
                vs[inputIndex++] = inputDwords.z;
                vs[inputIndex++] = inputDwords.w;
            }

            [unroll]
            for (uint f = 0; f < numFeatures/2; ++f)
            {
                inputIndex = 0;
                if (f >= numFeatures / 4)
                {
                    inputIndex = 4;
                }

                [unroll]
                for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                {
                    const uint weightsOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                    const uint4 weightsDwords = weights0.storage.Load4(weightsOffset);

                    conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.x, conv_accumulator[f]);
                    conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.y, conv_accumulator[f]);
                    conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.z, conv_accumulator[f]);
                    conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.w, conv_accumulator[f]);
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

    // Add first half
    uint4 packedVss[4];
    [unroll]
    for (uint i = 0; i < numFeatures/2/2; i+=4)
    {
        const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * weightReluScale + Load4hA(bias2, i);
        result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

        const int4 quantizedResult = round(result * (rcprFNBoutputQuantizationScale));
        packedVss[0][i/4] = pack_clamp_s8(quantizedResult);
    }

    [unroll]
    for (uint i = numFeatures/2/2; i < numFeatures/2; i+=4)
    {
        const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * weightReluScale + Load4hA(bias2, i);
        result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

        const int4 quantizedResult = round(result * (rcprFNBoutputQuantizationScale));
        packedVss[1][(i-numFeatures/2/2)/4] = pack_clamp_s8(quantizedResult);
    }

    // Second half
    [unroll]
    for (uint i = numFeatures/2; i < numFeatures-16; i+=4)
    {
        const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * weightReluScale + Load4hA(bias2, i);
        result += unpack_s8s32(conv_result[i/4]) * quantScale1;

        const int4 quantizedResult = round(result * (rcprFNBoutputQuantizationScale));
        packedVss[2][(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
    }

    [unroll]
    for (uint i = numFeatures-16; i < numFeatures; i+=4)
    {
        const int3 po = int3(poBase.xy, i);

        const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * weightReluScale + Load4hA(bias2, i);
        result += unpack_s8s32(conv_result[i/4]) * quantScale1;

        const int4 quantizedResult = round(result * (rcprFNBoutputQuantizationScale));
        packedVss[3][(i-(numFeatures-16))/4] = pack_clamp_s8(quantizedResult);
    }

    // CT2D + ADD
    const float ct2d_quantizationScale = fnbOutputScale * ct2d_weights.quantizationScale;
    const int2 localOffset[4] = { int2(0,0), int2(1,0), int2(0,1), int2(1,1) };

    //[unroll]
    for (int  i = 0; i < 4; ++i)
    {
        const int2 pk = localOffset[i];
        const uint2 poBase2D = computeShaderParams.dispatchThreadID.xy * 2 + pk;
        if (any(poBase2D >= output.logicalSize.xy))
            continue;//TODO Find a better solution that is still reliable, keep in mind calculations until here are needed for each of the four output pixels. dispatch numgroups * groupsize exceeds tensor dims by a bit
        float outputScale = 1.0 / quantOutputScale0;

        uint4 storeDwords = uint4(0,0,0,0);
        uint storageIndex = 0;

        [unroll]
        for (uint z = 0; z < 32;)
        {
            const int3 po = int3(poBase2D, z);

            int4 accumulator = 0;
            uint weightsOffset = ct2d_weights.OffsetOf(uint4(pk, po.z, 0));

            [unroll]
            for (uint c = 0 ; c != 64; c += 16)
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
            for (uint c = 0 ; c != 64; c += 16)
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
            for (uint c = 0 ; c != 64; c += 16)
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
            for (uint c = 0 ; c != 64; c += 16)
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
            result += unpack_s8s32(Load4i8A(inputAdd, po)) * inputAdd.quantizationScale;

            storeDwords[storageIndex++] = pack_clamp_s8(int4(round(result * outputScale)));
            z+=4;

            if (z%16 ==0)
            {
                output.storage.Store4(output.OffsetOf(int3(poBase2D, z-16)), storeDwords);
                storageIndex = 0;
                outputScale = 1.0 / quantOutputScale1;
            }
        }
    }
}

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FNB_CT2D_ADD(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float fnbOutputScale,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI> inputAdd,
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
            result += unpack_s8s32(Load4i8A(inputAdd, po)) * inputAdd.quantizationScale;

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

#if WMMA_ENABLED
template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FNB_CT2D_ADD(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float fnbOutputScale,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI> inputAdd,
    const QuantizedTensor4i8_HWNC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_HWNC<SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_HWNC<SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const QuantizedTensor4i8_HWCN< SW3> ct2d_weights,
    const QuantizedTensor1i32<SB3> ct2d_bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    _Static_assert(numFeatures == 64, "NumFeatures must be 64");
    _Static_assert(numGroups == 2, "NumGroups must be 2");

    const int3 workOffset = computeShaderParams.groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv0OutputsWmma[2][64/16];

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2][64/16];
    [unroll]
    for (uint f = 0; f < weights0.logicalSize.w; f += 16)
    {
        accumulatorMatrix[0][f / 16].Load(bias0.storage.buffer, bias0.OffsetOf(f), 0, true);
        accumulatorMatrix[1][f / 16].Copy(accumulatorMatrix[0][f / 16]);
    }

    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            for (uint f = 0; f < weights0.logicalSize.w; f += 16)
            {
                int currGroupId = f / 16;
                [unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {
                    [unroll]
                    for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                    {
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix;
                        inputMatrix.Fill(0);

                        int ThreadId = WaveGetLaneIndex();
                        int groupWmma = ThreadId / 16;
                        int threadWmma = ThreadId % 16;

                        int3 LoadCoordBase = int3(piBase + int3(kx + inputWaveOffset * 16 + threadWmma, ky, c + groupWmma * 8 + currGroupId * 16));
                        uint LoadOffset = input.OffsetOf(LoadCoordBase);
                        bool OutOfRange = LoadCoordBase.x >= input.logicalSize.x || LoadCoordBase.y >= input.logicalSize.y;
                        LoadOffset = OutOfRange ? 0x80000000 : LoadOffset;

                        uint2 InputBufferBytes = input.storage.Load2(LoadOffset);
                        inputMatrix.container.r[0] = InputBufferBytes[0];
                        inputMatrix.container.r[1] = InputBufferBytes[1];

                        uint weightOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                        weightMatrix.Load(weights0.storage.buffer, weightOffset, weights0.storageByteStrides.w, false);

                        accumulatorMatrix[inputWaveOffset][f / 16] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][f / 16]);
                    }
                }
            }
        }
    }

    [unroll]
    for (uint f = 0; f < weights0.logicalSize.w; f += 16)
    {
        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId][f/16].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId][f/16].Element(i));
                quantizedElement *= quantScale0 * weights0.quantizationScale;
                quantizedElement *= (1.0 / quantScale1);
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                accumulatorMatrix[matId][f / 16].SetElement(i, int(quantizedElement));
            }

            conv0OutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId][f / 16]);
        }
    }

    // Fuse split values
    {
        uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 32));
        conv0OutputsWmma[0][2].Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
        inputOffset = input.OffsetOf(piBase + int3(16 + 1, 1, 32));
        conv0OutputsWmma[1][2].Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);

        inputOffset = input.OffsetOf(piBase + int3(1, 1, 48));
        conv0OutputsWmma[0][3].Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
        inputOffset = input.OffsetOf(piBase + int3(16 + 1, 1, 48));
        conv0OutputsWmma[1][3].Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
    }

    // Second Convolution + Relu
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> conv1OutputsWmma[32 / 16][128/16];

    [unroll]
    for (uint f = 0; f < weights1.logicalSize.w; f += 16)
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias1.storage.buffer, bias1.OffsetOf(f), 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights1.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputMatrix = conv0OutputsWmma[inputWaveOffset][c/16];

                int weightOffset = weights1.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> weightMatrix;
                weightMatrix.Load(weights1.storage.buffer, weightOffset, weights1.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                quantizedElement *= quantScale1 * weights1.quantizationScale;
                quantizedElement *= 1.0 / activationScaleFactor;
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, 0.0f, 127.0f); //Dropping the lower half as ReLu implementation
                int quantizedResult = int(quantizedElement);

                accumulatorMatrix[matId].SetElement(i, quantizedResult);
            }

            conv1OutputsWmma[matId][f/16].Copy(accumulatorMatrix[matId]);
        }
    }

    // Third Convolution
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> conv2OutputsWmma[2][64/16];

    [unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        // NOTE: Bias loading
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16> biasMatrixF16;
        biasMatrixF16.Load(bias2.storage.buffer, f * 2, 0, true);

        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias2.storage.buffer, bias2.OffsetOf(f), 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights2.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputMatrix = conv1OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights2.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> weightMatrix;
                weightMatrix.Load(weights2.storage.buffer, weightOffset, weights2.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        if ((f/32) == 0)
        {
            [unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputsAdd;
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> inputsAddI32;
                const uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 0) + int3(matId * 16, 0, f));
                inputsAdd.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                inputsAddI32.Copy(inputsAdd);

                [unroll]
                for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                {
                    float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                    quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                    float residuals = float(inputsAddI32.Element(i)) * quantScale0;
                    quantizedElement += residuals;

                    quantizedElement *= 1.0 / fnbOutputScale;
                    quantizedElement = round(quantizedElement);
                    quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                    accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                }

                conv2OutputsWmma[matId][f/16].Copy(accumulatorMatrix[matId]);
            }
        }
        else
        {
            [unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> inputsAddI32;
                inputsAddI32.Copy(conv0OutputsWmma[matId][f/16]);

                [unroll]
                for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                {
                    float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                    quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                    float residuals = float(inputsAddI32.Element(i)) * quantScale1;
                    quantizedElement += residuals;

                    quantizedElement *= 1.0 / fnbOutputScale;
                    quantizedElement = round(quantizedElement);
                    quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                    accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                }

                conv2OutputsWmma[matId][f/16].Copy(accumulatorMatrix[matId]);
            }
        }
    }

    // Fourth convolution (x = 2, y = 2, half = 2, W = 1)
    [unroll]
    for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
    {
        [unroll]
        for (uint ky = 0; ky < ct2d_weights.logicalSize.y; ++ky)
        {
            [unroll]
            for (uint kx = 0; kx < ct2d_weights.logicalSize.x; ++kx)
            {
                [unroll]
                for (uint weightWOffset = 0; weightWOffset < (ct2d_weights.logicalSize.z / 16); ++weightWOffset)
                {
                    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> convOutputsWmma;

                    // NOTE: X = 2, Y = 2, Half = 2
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> accumulatorMatrix;
                    accumulatorMatrix.Load(ct2d_bias.storage.buffer, ct2d_bias.OffsetOf(weightWOffset * 16), 0, true);

                    [unroll]
                    for (uint inputZOffset = 0; inputZOffset < (ct2d_weights.logicalSize.w / 16); ++inputZOffset)
                    {
                        AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputMatrix = conv2OutputsWmma[inputWaveOffset][inputZOffset];

                        uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, weightWOffset * 16, inputZOffset * 16));
                        AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> weightMatrix;
                        weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                        accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);
                    }

                    // NOTE: Get skip connection
                    int skipOffset = inputAdd.OffsetOf(poBase + uint3(kx + inputWaveOffset * stride * 16, ky, weightWOffset * 16));
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> skipMatrix;
                    skipMatrix.Load(inputAdd.storage.buffer, skipOffset, stride * inputAdd.storageByteStrides.x, true);
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> skipMatrixI32;
                    skipMatrixI32.Copy(skipMatrix);

                    // NOTE: Quantize
                    [unroll]
                    for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                    {
                        float matElement = (float)accumulatorMatrix.Element(i);
                        matElement *= fnbOutputScale * ct2d_weights.quantizationScale;

                        float skipValue = (float)skipMatrixI32.Element(i) * inputAdd.quantizationScale;
                        matElement += skipValue;

                        if (weightWOffset == 0)
                        {
                            matElement *= 1.0 / quantOutputScale0;
                        }
                        else
                        {
                            matElement *= 1.0 / quantOutputScale1;
                        }

                        matElement = round(matElement);
                        matElement = clamp(matElement, -128.0f, 127.0f);

                        accumulatorMatrix.SetElement(i, int(matElement));
                    }

                    convOutputsWmma.Copy(accumulatorMatrix);

                    // NOTE: Store
                    uint storeOffset = output.OffsetOf(uint3(poBase) + uint3(kx + stride * 16 * inputWaveOffset, ky, 16 * weightWOffset));
                    convOutputsWmma.Store(output.storage.buffer, storeOffset, stride * output.storageByteStrides.x, true);
                }
            }
        }
    }
}

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FNB_CT2D_ADD(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float fnbOutputScale,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor3i8_NHWC<SI> inputAdd,
    const QuantizedTensor4i8_HWNC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_HWNC< SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_HWNC< SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const QuantizedTensor4i8_HWCN< SW3> ct2d_weights,
    const QuantizedTensor1i32<SB3> ct2d_bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    _Static_assert(numFeatures == 32, "NumFeatures must be 32");
    _Static_assert(numGroups == 1, "NumGroups must be 1");

    const int3 workOffset = computeShaderParams.groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv0OutputsWmma[2][32 / 16];

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2];
    accumulatorMatrix[0].Load(bias0.storage.buffer, bias0.OffsetOf(0), 0, true);
    accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

    //[unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
    {
        //[unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            for (uint f = 0; f < weights0.logicalSize.w; f += 16)
            {
                //[unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {
                    //[unroll]
                    for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                    {
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix;
                        inputMatrix.Fill(0);

                        int ThreadId = WaveGetLaneIndex();
                        int groupWmma = ThreadId / 16;
                        int threadWmma = ThreadId % 16;

                        int3 LoadCoordBase = int3(piBase + int3(kx + inputWaveOffset * 16 + threadWmma, ky, c + groupWmma * 8));
                        uint LoadOffset = input.OffsetOf(LoadCoordBase);
                        bool OutOfRange = LoadCoordBase.x >= input.logicalSize.x || LoadCoordBase.y >= input.logicalSize.y;
                        LoadOffset = OutOfRange ? 0x80000000 : LoadOffset;

                        uint2 InputBufferBytes = input.storage.Load2(LoadOffset);
                        inputMatrix.container.r[0] = InputBufferBytes[0];
                        inputMatrix.container.r[1] = InputBufferBytes[1];

                        uint weightOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                        weightMatrix.Load(weights0.storage.buffer, weightOffset, weights0.storageByteStrides.w, false);

                        accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
                    }
                }
            }
        }
    }

    for (uint f = 0; f < weights0.logicalSize.w; f += 16)
    {
        for (uint matId = 0; matId < 2; ++matId)
        {
            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                quantizedElement *= quantScale0 * weights0.quantizationScale;
                quantizedElement *= (1.0 / quantScale1);
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
            }

            conv0OutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId]);
        }
    }

    // Fuse split values
    {
        uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 16));
        conv0OutputsWmma[0][1].Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
        inputOffset = input.OffsetOf(piBase + int3(16 + 1, 1, 16));
        conv0OutputsWmma[1][1].Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
    }

    // Second Convolution + Relu
    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv1OutputsWmma[32 / 16][64 / 16];

    //[unroll]
    for (uint f = 0; f < weights1.logicalSize.w; f += 16)
    {
        // NOTE: Bias loading
        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > biasMatrixF16;
        biasMatrixF16.Load(bias1.storage.buffer, f * 2, 0, true);

        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias1.storage.buffer, bias1.OffsetOf(f), 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        //[unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
        {
            //[unroll]
            for (uint c = 0; c < weights1.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix = conv0OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights1.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                weightMatrix.Load(weights1.storage.buffer, weightOffset, weights1.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        //[unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                quantizedElement *= quantScale1 * weights1.quantizationScale;
                quantizedElement *= 1.0 / activationScaleFactor;
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, 0.0f, 127.0f); //Dropping the lower half as ReLu implementation
                int quantizedResult = int(quantizedElement);

                accumulatorMatrix[matId].SetElement(i, quantizedResult);
            }

            conv1OutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId]);
        }
    }

    // Third Convolution
    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv2OutputsWmma[2][32 / 16];

    //[unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        // NOTE: Bias loading
        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > biasMatrixF16;
        biasMatrixF16.Load(bias2.storage.buffer, f * 2, 0, true);

        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias2.storage.buffer, bias2.OffsetOf(f), 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        //[unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
        {
            //[unroll]
            for (uint c = 0; c < weights2.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix = conv1OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights2.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                weightMatrix.Load(weights2.storage.buffer, weightOffset, weights2.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        if ((f / 16) == 0)
        {
            //[unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputsAdd;
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > inputsAddI32;
                const uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 0) + int3(matId * 16, 0, f));
                inputsAdd.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                inputsAddI32.Copy(inputsAdd);

                for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                {
                    float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                    quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                    float residuals = float(inputsAddI32.Element(i)) * quantScale0;
                    quantizedElement += residuals;

                    quantizedElement *= 1.0 / fnbOutputScale;
                    quantizedElement = round(quantizedElement);
                    quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                    accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                }

                conv2OutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId]);
            }
        }
        else
        {
            //[unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > inputsAddI32;
                inputsAddI32.Copy(conv0OutputsWmma[matId][f / 16]);

                for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                {
                    float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                    quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                    float residuals = float(inputsAddI32.Element(i)) * quantScale1;
                    quantizedElement += residuals;

                    quantizedElement *= 1.0 / fnbOutputScale;
                    quantizedElement = round(quantizedElement);
                    quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                    accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                }

                conv2OutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId]);
            }
        }
    }

    // Fourth convolution (x = 2, y = 2, half = 2, W = 1)
    //[unroll]
    for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
    {
        //[unroll]
        for (uint ky = 0; ky < ct2d_weights.logicalSize.y; ++ky)
        {
            //[unroll]
            for (uint kx = 0; kx < ct2d_weights.logicalSize.x; ++kx)
            {
                //[unroll]
                for (uint weightWOffset = 0; weightWOffset < (ct2d_weights.logicalSize.z / 16); ++weightWOffset)
                {
                    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > convOutputsWmma;

                    // NOTE: X = 2, Y = 2, Half = 2
                    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix;
                    accumulatorMatrix.Load(ct2d_bias.storage.buffer, ct2d_bias.OffsetOf(weightWOffset * 16), 0, true);

                    //[unroll]
                    for (uint inputZOffset = 0; inputZOffset < (ct2d_weights.logicalSize.w / 16); ++inputZOffset)
                    {
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix = conv2OutputsWmma[inputWaveOffset][inputZOffset];

                        uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, weightWOffset * 16, inputZOffset * 16));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                        weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                        accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);
                    }

                    // NOTE: Get skip connection
                    int skipOffset = inputAdd.OffsetOf(poBase + uint3(kx + inputWaveOffset * stride * 16, ky, weightWOffset * 16));
                    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > skipMatrix;
                    skipMatrix.Load(inputAdd.storage.buffer, skipOffset, stride * inputAdd.storageByteStrides.x, true);
                    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > skipMatrixI32;
                    skipMatrixI32.Copy(skipMatrix);

                    // NOTE: Quantize
                    for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                    {
                        float matElement = (float) accumulatorMatrix.Element(i);
                        matElement *= fnbOutputScale * ct2d_weights.quantizationScale;

                        float skipValue = (float) skipMatrixI32.Element(i) * inputAdd.quantizationScale;
                        matElement += skipValue;
                        matElement *= 1.0 / output.quantizationScale;

                        matElement = round(matElement);
                        matElement = clamp(matElement, -128.0f, 127.0f);

                        accumulatorMatrix.SetElement(i, int(matElement));
                    }

                    convOutputsWmma.Copy(accumulatorMatrix);

                    // NOTE: Store
                    uint storeOffset = output.OffsetOf(uint3(poBase) + uint3(kx + stride * 16 * inputWaveOffset, ky, 16 * weightWOffset));
                    convOutputsWmma.Store(output.storage.buffer, storeOffset, stride * output.storageByteStrides.x, true);
                }
            }
        }
    }
}
#endif // #if WMMA_ENABLED
