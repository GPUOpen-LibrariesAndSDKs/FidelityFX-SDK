// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

#if WMMA_ENABLED
#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"
#endif

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    if (numFeatures == 32)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                    preloadedInputs[i/4] = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }
                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = Load4i32(bias0, i);
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
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
                    const int16_t4 quantized_vs = round(accumulator * quantScale0 * weights0.quantizationScale * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int relu_output[numFeatures*2];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 b = Load4i32(bias1, i);
                    relu_output[i]   = b.x;
                    relu_output[i+1] = b.y;
                    relu_output[i+2] = b.z;
                    relu_output[i+3] = b.w;
                }
                [unroll]
                for (uint i = 0; i < numFeatures*2; i++)
                {
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int16_t4 quantized_vs = round(vs * quantScale1 * weights1.quantizationScale * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_clamp_s8(Relu(quantized_vs));
                }
                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////

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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////

                uint4 storeDwords;
                [unroll]
                for (uint i = 0; i < numFeatures / 2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int16_t4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int16_t4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 64)
    {
        if (numGroups != 2)
            return;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = Load4i32(bias0, i);
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                    const int16_t4 quantized_vs = round(accumulator * quantScale0 * weights0.quantizationScale * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int relu_output[numFeatures*2];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 b = Load4i32(bias1, i);
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
                    const int16_t4 quantized_vs = round(vs * quantScale1 * weights1.quantizationScale * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_clamp_s8(Relu(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                // Add first half
                uint4 storeDwords;
                uint storageOffset = output.OffsetOf(poBase);
                [unroll]
                for (uint i = 0; i < numFeatures/2/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int16_t4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2/2; i < numFeatures/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int16_t4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[(i-numFeatures/2/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,16));
                output.storage.Store4(storageOffset, storeDwords);


                // Second half
                [unroll]
                for (uint i = numFeatures/2; i < numFeatures-16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int16_t4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32));
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures-16; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int16_t4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-(numFeatures-16))/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32+16));
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
}

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    if (numFeatures == 16)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                // The packed result from the first convolution after concat with the input
                int8_t4_packed conv_result[4] = {0,0,0,0};

                // accumulator for half the number of features (the ones the first conv works on)
                int3 pi = piBase;

                // // preloaded inputs
                int preloadedInputs[2];


                // Note: load 4 dwords on one go and split between two buffers
                // Load second part of input into the conv_result
                {
                    const int3 po = int3(poBase.xy, 0);
                    const uint inputOffset = input.OffsetOf(po);
                    const int4 inputDwords = input.storage.Load4(inputOffset);
                    preloadedInputs[0] = inputDwords.x;
                    preloadedInputs[1] = inputDwords.y;
                    conv_result[2] = inputDwords.z;
                    conv_result[3] = inputDwords.w;
                }

                int conv_accumulator[8] = {0,0,0,0,0,0,0,0};
                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < 8; i+=2)
                {
                    const int2 b = (0,0);
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[2];
                        uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 4)//8
                        {
                            const int3 ps = piBase + int3(kx, ky, c);
                            vs[inputIndex++] = Load4i8A(input, ps);
                        }

                        [unroll]
                        for (uint f = 0; f != 8; ++f)
                        {
                            inputIndex = 0;
                            for (uint c = 0; c != weights0.logicalSize.z; c += 4) // z == 8
                            {
                                int8_t4_packed weights = Load4i8A(weights0, uint4(kx, ky, c, f));
                                conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weights, conv_accumulator[f]);
                            }
                        }
                    }

                [unroll]
                for (uint f = 0; f != 8; f+=4)
                {
                    const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
                    const int16_t4 quantized_vs = round((accumulator * (quantScale0 * weights0.quantizationScale) + Load4hA(bias0, f)) * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int relu_output[32];
                [unroll]
                for (uint i = 0; i < 32; i+=4)
                {
                    const int4 b = 0;
                    relu_output[i]   = b.x;
                    relu_output[i+1] = b.y;
                    relu_output[i+2] = b.z;
                    relu_output[i+3] = b.w;
                }
                [unroll]
                for (uint i = 0; i < 32; i++)
                {
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    int4 weightsDwords = weights1.storage.Load4(weightsOffset); // load 16 values at once

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                }

                uint8_t4_packed relu_result[8];

                [unroll]
                for (uint i = 0; i < 32; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    float4 result = vs * weights1.quantizationScale * quantScale1 + Load4hA(bias1, i);
                    const int4 quantized_vs = round(result * (1.0/activationScaleFactor));//round((vs * (quantScale1 * weights1.quantizationScale)+ Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_clamp_s8(Relu(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int conv3_output[numFeatures];
                [unroll]
                for (uint i = 0; i < numFeatures; i++) { conv3_output[i] = 0; }

                [unroll]
                for (uint i = 0; i < numFeatures; i++)
                {
                    uint inputIndex = 0;
                    [unroll]
                    for (uint j = 0; j < 32; j+=16)
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords = uint4(0,0,0,0);
                [unroll]
                for (uint i = 0; i < 8; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }

                [unroll]
                for (uint i = 8; i < 16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }

                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }

    else if (numFeatures == 32 && numGroups == 1)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                    preloadedInputs[i/4] = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
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
                    const int4 quantized_vs = round((accumulator * (quantScale0 * weights0.quantizationScale) + Load4hA(bias0, f)) * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords;
                [unroll]
                for (uint i = 0; i < numFeatures / 2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 32 && numGroups == 2)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                    preloadedInputs[i/4] = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[numFeatures/2/4];
                        uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c < weights0.logicalSize.z; c += 16)
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
                                inputIndex = 2;
                            }

                            [unroll]
                            for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                            {
                                const uint weightsOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                                const uint2 weightsDwords = weights0.storage.Load2(weightsOffset);

                                conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.x, conv_accumulator[f]);
                                conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.y, conv_accumulator[f]);
                            }
                        }
                    }

                [unroll]
                for (uint f = 0; f != numFeatures/2; f+=4)
                {
                    const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
                    const int4 quantized_vs = round((accumulator * quantScale0 * weights0.quantizationScale + Load4hA(bias0, f))* (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords;
                [unroll]
                for (uint i = 0; i < numFeatures / 2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 48)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                // The packed result from the first convolution after concat with the input
                int8_t4_packed conv_result[numFeatures/4];

                // accumulator for half the number of features (the ones the first conv works on)
                int conv_accumulator[16];
                int3 pi = piBase;

                // Load second part of input into the conv_result
                [unroll]
                for (uint z = 16; z < 48; z+=16)
                {
                    const int3 po = int3(poBase.xy, z);
                    const uint inputOffset = input.OffsetOf(po);
                    const int4 inputDwords = input.storage.Load4(inputOffset);
                    conv_result[z/4]   = inputDwords.x;
                    conv_result[z/4+1] = inputDwords.y;
                    conv_result[z/4+2] = inputDwords.z;
                    conv_result[z/4+3] = inputDwords.w;
                }

                // preloaded inputs
                int preloadedInputs[4];
                [unroll]
                for (uint i = 0; i < 16; i+=16)
                {
                    const int3 po = int3(poBase.xy, i);
                    const uint inputOffset = input.OffsetOf(po);
                    const int4 inputDwords = input.storage.Load4(inputOffset);

                    preloadedInputs[i/4]   = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < 16; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[16/4];
                        uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 4)
                        {
                            const int3 ps = piBase + int3(kx, ky, c);
                            vs[inputIndex++] = Load4i8A(input, ps);
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
                        }

                        [unroll]
                        for (uint f = 0; f != 16; ++f)
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
                for (uint f = 0; f != 16; f+=4)
                {
                    const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
                    const int4 quantized_vs = round((accumulator * (quantScale0 * weights0.quantizationScale) + Load4hA(bias0, f)) * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    int4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,32,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[8], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[9], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[10], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[11], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];

                [unroll]
                for (uint i = 0; i < numFeatures*2/4; i+=1)
                {
                    relu_result[i] = 0;
                }
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords = uint4(0,0,0,0);
                [unroll]
                for (uint i = 0; i < 16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = 16; i < 32; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-16)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = 32; i < 48; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-32)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 32);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 64 && numGroups == 1)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[32/4];
                        uint inputIndex = 0;

                        //int8_t4_packed vs[16/4];
                        //uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 4)
                        {
                            const int3 ps = piBase + int3(kx, ky, c);
                            vs[inputIndex++] = Load4i8A(input, ps);
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
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
                    const int4 quantized_vs = round((accumulator * quantScale0 * weights0.quantizationScale + Load4hA(bias0, f))* (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i)) * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                // Add first half
                uint4 storeDwords;
                uint storageOffset = output.OffsetOf(poBase);
                [unroll]
                for (uint i = 0; i < numFeatures/2/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2/2; i < numFeatures/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[(i-numFeatures/2/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,16));
                output.storage.Store4(storageOffset, storeDwords);


                // Second half
                [unroll]
                for (uint i = numFeatures/2; i < numFeatures-16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32));
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures-16; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-(numFeatures-16))/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32+16));
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 64 && numGroups == 2)
    {
        if (numGroups != 2)
            return;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                    const int4 quantized_vs = round((accumulator * quantScale0 * weights0.quantizationScale + Load4hA(bias0, f))* (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i)) * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                // Add first half
                uint4 storeDwords;
                uint storageOffset = output.OffsetOf(poBase);
                [unroll]
                for (uint i = 0; i < numFeatures/2/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2/2; i < numFeatures/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale0));
                    storeDwords[(i-numFeatures/2/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,16));
                output.storage.Store4(storageOffset, storeDwords);


                // Second half
                [unroll]
                for (uint i = numFeatures/2; i < numFeatures-16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32));
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures-16; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/quantOutputScale1));
                    storeDwords[(i-(numFeatures-16))/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32+16));
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
}

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    if (numFeatures == 32)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                    preloadedInputs[i/4] = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = Load4i32(bias0, i);
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
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
                    const int16_t4 quantized_vs = round(accumulator * quantScale0 * weights0.quantizationScale * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int relu_output[numFeatures*2];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 b = Load4i32(bias1, i);
                    relu_output[i]   = b.x;
                    relu_output[i+1] = b.y;
                    relu_output[i+2] = b.z;
                    relu_output[i+3] = b.w;
                }
                [unroll]
                for (uint i = 0; i < numFeatures*2; i++)
                {
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int16_t4 quantized_vs = round(vs * quantScale1 * weights1.quantizationScale * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_clamp_s8(Relu(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords;
                [unroll]
                for (uint i = 0; i < numFeatures / 2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int16_t4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int16_t4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }

    else if (numFeatures == 64)
    {
        if (numGroups != 2)
            return;

        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = Load4i32(bias0, i);
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                    const int16_t4 quantized_vs = round(accumulator * quantScale0 * weights0.quantizationScale * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int relu_output[numFeatures*2];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 b = Load4i32(bias1, i);
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
                    const int16_t4 quantized_vs = round(vs * quantScale1 * weights1.quantizationScale * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_clamp_s8(Relu(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                // Add first half
                uint4 storeDwords;
                uint storageOffset = output.OffsetOf(poBase);
                [unroll]
                for (uint i = 0; i < numFeatures/2/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int16_t4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2/2; i < numFeatures/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int16_t4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,16));
                output.storage.Store4(storageOffset, storeDwords);


                // Second half
                [unroll]
                for (uint i = numFeatures/2; i < numFeatures-16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int16_t4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32));
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures-16; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4i32(bias2, i) * bias2.quantizationScale;
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int16_t4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-(numFeatures-16))/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32+16));
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
}

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    if(numFeatures == 16)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                // The packed result from the first convolution after concat with the input
                int8_t4_packed conv_result[4] = {0,0,0,0};

                // accumulator for half the number of features (the ones the first conv works on)
                int3 pi = piBase;

                // // preloaded inputs
                int preloadedInputs[2];


                // Note: load 4 dwords on one go and split between two buffers
                // Load second part of input into the conv_result
                {
                    const int3 po = int3(poBase.xy, 0);
                    const uint inputOffset = input.OffsetOf(po);
                    const int4 inputDwords = input.storage.Load4(inputOffset);
                    preloadedInputs[0] = inputDwords.x;
                    preloadedInputs[1] = inputDwords.y;
                    conv_result[2] = inputDwords.z;
                    conv_result[3] = inputDwords.w;
                }

                int conv_accumulator[8] = {0,0,0,0,0,0,0,0};

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < 8; i+=2)
                {
                    const int2 b = (0,0);
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[2];
                        uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 4)//8
                        {
                            const int3 ps = piBase + int3(kx, ky, c);
                            vs[inputIndex++] = Load4i8A(input, ps);
                        }

                        [unroll]
                        for (uint f = 0; f != 8; ++f)
                        {
                            inputIndex = 0;
                            for (uint c = 0; c != weights0.logicalSize.z; c += 4) // z == 8
                            {
                                int8_t4_packed weights = Load4i8A(weights0, uint4(kx, ky, c, f));
                                conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weights, conv_accumulator[f]);
                            }
                        }
                    }

                [unroll]
                for (uint f = 0; f != 8; f+=4)
                {
                    const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
                    const int16_t4 quantized_vs = round((accumulator * (quantScale0 * weights0.quantizationScale) + Load4hA(bias0, f)) * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int relu_output[32];
                [unroll]
                for (uint i = 0; i < 32; i+=4)
                {
                    const int4 b = 0;
                    relu_output[i]   = b.x;
                    relu_output[i+1] = b.y;
                    relu_output[i+2] = b.z;
                    relu_output[i+3] = b.w;
                }
                [unroll]
                for (uint i = 0; i < 32; i++)
                {
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    int4 weightsDwords = weights1.storage.Load4(weightsOffset); // load 16 values at once

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                }

                int8_t4_packed relu_result[8];
                [unroll]
                for (uint i = 0; i < 8; i+=1)
                {
                    relu_result[i] = 0;
                }

                [unroll]
                for (uint i = 0; i < 32; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * (quantScale1 * weights1.quantizationScale)+ Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                int conv3_output[numFeatures];
                [unroll]
                for (uint i = 0; i < numFeatures; i++) { conv3_output[i] = 0; }

                [unroll]
                for (uint i = 0; i < numFeatures; i++)
                {
                    uint inputIndex = 0;
                    [unroll]
                    for (uint j = 0; j < 32; j+=16)
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords = uint4(0,0,0,0);
                [unroll]
                for (uint i = 0; i < 8; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }

                [unroll]
                for (uint i = 8; i < 16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }

                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 32 && numGroups == 1)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                    preloadedInputs[i/4] = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
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
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords;
                [unroll]
                for (uint i = 0; i < numFeatures / 2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 32 && numGroups == 2)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                    preloadedInputs[i/4] = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[numFeatures/2/4];
                        uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c < weights0.logicalSize.z; c += 16)
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
                                inputIndex = 2;
                            }

                            [unroll]
                            for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                            {
                                const uint weightsOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                                const uint2 weightsDwords = weights0.storage.Load2(weightsOffset);

                                conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.x, conv_accumulator[f]);
                                conv_accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.y, conv_accumulator[f]);
                            }
                        }
                    }

                [unroll]
                for (uint f = 0; f != numFeatures/2; f+=4)
                {
                    const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
                    const int4 quantized_vs = round((accumulator * quantScale0 * weights0.quantizationScale + Load4hA(bias0, f))* (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords;
                [unroll]
                for (uint i = 0; i < numFeatures / 2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 48)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                // The packed result from the first convolution after concat with the input
                int8_t4_packed conv_result[numFeatures/4];

                // accumulator for half the number of features (the ones the first conv works on)
                int conv_accumulator[16];
                int3 pi = piBase;

                // Load second part of input into the conv_result
                [unroll]
                for (uint z = 16; z < 48; z+=16)
                {
                    const int3 po = int3(poBase.xy, z);
                    const uint inputOffset = input.OffsetOf(po);
                    const int4 inputDwords = input.storage.Load4(inputOffset);
                    conv_result[z/4]   = inputDwords.x;
                    conv_result[z/4+1] = inputDwords.y;
                    conv_result[z/4+2] = inputDwords.z;
                    conv_result[z/4+3] = inputDwords.w;
                }

                // preloaded inputs
                int preloadedInputs[4];
                [unroll]
                for (uint i = 0; i < 16; i+=16)
                {
                    const int3 po = int3(poBase.xy, i);
                    const uint inputOffset = input.OffsetOf(po);
                    const int4 inputDwords = input.storage.Load4(inputOffset);

                    preloadedInputs[i/4]   = inputDwords.x;
                    preloadedInputs[i/4+1] = inputDwords.y;
                    preloadedInputs[i/4+2] = inputDwords.z;
                    preloadedInputs[i/4+3] = inputDwords.w;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < 16; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[16/4];
                        uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 4)
                        {
                            const int3 ps = piBase + int3(kx, ky, c);
                            vs[inputIndex++] = Load4i8A(input, ps);
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
                        }

                        [unroll]
                        for (uint f = 0; f != 16; ++f)
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
                for (uint f = 0; f != 16; f+=4)
                {
                    const int4 accumulator = int4(conv_accumulator[f], conv_accumulator[f+1], conv_accumulator[f+2], conv_accumulator[f+3]);
                    const int4 quantized_vs = round((accumulator * (quantScale0 * weights0.quantizationScale) + Load4hA(bias0, f)) * (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
                    int4 weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,16,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[4], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[5], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[6], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[7], weightsDwords.w, relu_output[i]);

                    weightsOffset = weights1.OffsetOf(uint4(0,0,32,i));
                    weightsDwords = weights1.storage.Load4(weightsOffset);

                    relu_output[i] = dot4add_i8packed(conv_result[8], weightsDwords.x, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[9], weightsDwords.y, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[10], weightsDwords.z, relu_output[i]);
                    relu_output[i] = dot4add_i8packed(conv_result[11], weightsDwords.w, relu_output[i]);
                }

                int8_t4_packed relu_result[numFeatures*2/4];

                [unroll]
                for (uint i = 0; i < numFeatures*2/4; i+=1)
                {
                    relu_result[i] = 0;
                }
                [unroll]
                for (uint i = 0; i < numFeatures*2; i+=4)
                {
                    const int4 vs = int4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i))* (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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

                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                uint4 storeDwords = uint4(0,0,0,0);
                [unroll]
                for (uint i = 0; i < 16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                int3 po = int3(poBase.xy, 0);
                uint storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = 16; i < 32; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-16)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 16);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = 32; i < 48; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * weights2.quantizationScale * activationScaleFactor + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-32)/4] = pack_clamp_s8(quantizedResult);
                }
                po = int3(poBase.xy, 32);
                storageOffset = output.OffsetOf(po);
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
    else if (numFeatures == 64 && numGroups == 1)
    {
        for (uint y = 0; y < perThreadWork.y; y++)
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

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

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// First convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                [unroll]
                for (uint i = 0; i < numFeatures/2; i+=4)
                {
                    const int4 b = 0;
                    conv_accumulator[i]   = b.x;
                    conv_accumulator[i+1] = b.y;
                    conv_accumulator[i+2] = b.z;
                    conv_accumulator[i+3] = b.w;
                }
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                            continue;

                        int8_t4_packed vs[32/4];
                        uint inputIndex = 0;

                        //int8_t4_packed vs[16/4];
                        //uint inputIndex = 0;
                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 4)
                        {
                            const int3 ps = piBase + int3(kx, ky, c);
                            vs[inputIndex++] = Load4i8A(input, ps);
                            //vs[inputIndex++] = pack_clamp_s8(int16_t4(round(Load4hA(input, ps) * (1.0 /quantScale0))));
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
                    const int4 quantized_vs = round((accumulator * quantScale0 * weights0.quantizationScale + Load4hA(bias0, f))* (1.0 / quantScale1));
                    conv_result[f/4] = pack_clamp_s8(quantized_vs);
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Second Convolution + Relu ////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                    const int4 quantized_vs = round((vs * quantScale1 * weights1.quantizationScale + Load4hA(bias1, i)) * (1.0 / activationScaleFactor));
                    relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
                }

                ///////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////// Third Convolution ////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
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
                ///////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////// Add  + Store ///////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////
                // Add first half
                uint4 storeDwords;
                uint storageOffset = output.OffsetOf(poBase);
                [unroll]
                for (uint i = 0; i < numFeatures/2/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[i/4] = pack_clamp_s8(quantizedResult);
                }
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures/2/2; i < numFeatures/2; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(preloadedInputs[i/4]) * quantScale0;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,16));
                output.storage.Store4(storageOffset, storeDwords);


                // Second half
                [unroll]
                for (uint i = numFeatures/2; i < numFeatures-16; i+=4)
                {
                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-numFeatures/2)/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32));
                output.storage.Store4(storageOffset, storeDwords);

                [unroll]
                for (uint i = numFeatures-16; i < numFeatures; i+=4)
                {
                    const int3 po = int3(poBase.xy, i);

                    const int4 vs = int4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
                    float4 result = vs * (weights2.quantizationScale * activationScaleFactor) + Load4hA(bias2, i);
                    result += unpack_s8s32(conv_result[i/4]) * quantScale1;

                    const int4 quantizedResult = round(result * (1.0/output.quantizationScale));
                    storeDwords[(i-(numFeatures-16))/4] = pack_clamp_s8(quantizedResult);
                }
                storageOffset = output.OffsetOf(poBase + int3(0,0,32+16));
                output.storage.Store4(storageOffset, storeDwords);
            }
    }
}

#if WMMA_ENABLED
template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWNC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_HWNC< SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_HWNC< SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);

    int3 workOffset = 0;

    if (numFeatures == 32)
    {
        workOffset = output.threadGroupSliceStart;
    }
    else if (numFeatures == 64 && numGroups == 2)
    {
        workOffset = output.threadGroupSliceStart;
    }
    else
    {
        workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    }


    if (numFeatures == 32 && numGroups == 1)
    {
        [unroll]
        for (uint y = 0; y < perThreadWork.y; y++)
        {
            [unroll]
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv0OutputsWmma[2][32 / 16];

                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2];
                accumulatorMatrix[0].Load(bias0.storage.buffer, bias0.OffsetOf(0), 0, true);
                accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

                [unroll]
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                {
                    [unroll]
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        for (uint f = 0; f < weights0.logicalSize.w; f += 16)
                        {
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

                [unroll]
                for (uint f = 0; f < weights0.logicalSize.w; f += 16)
                {
                    [unroll]
                    for (uint matId = 0; matId < 2; ++matId)
                    {
                        [unroll]
                        for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                        {
                            float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                            quantizedElement *= quantScale0 * weights0.quantizationScale;
                            quantizedElement *= (1.0 / quantScale1);
                            quantizedElement = round(quantizedElement);
                            quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                            accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                        }

                        conv0OutputsWmma[matId][0].Copy(accumulatorMatrix[matId]);
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
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> conv1OutputsWmma[32 / 16][4];

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
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> outputMats[2][2];

                [unroll]
                for (uint f = 0; f < weights2.logicalSize.w; f += 16)
                {
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

                    if ((f/16) == 0)
                    {
                        [unroll]
                        for (uint matId = 0; matId < 2; ++matId)
                        {
                            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputs;
                            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> inputsI32;
                            const uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 0) + int3(matId * 16, 0, 0));
                            inputs.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                            inputsI32.Copy(inputs);

                            [unroll]
                            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                            {
                                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                                quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                                float residuals = float(inputsI32.Element(i)) * quantScale0;
                                quantizedElement += residuals;

                                quantizedElement *= (1.0/quantOutputScale0);
                                quantizedElement = round(quantizedElement);
                                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                            }

                            outputMats[matId][0].Copy(accumulatorMatrix[matId]);
                        }
                    }
                    else
                    {
                        [unroll]
                        for (uint matId = 0; matId < 2; ++matId)
                        {
                            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> inputsAdd;
                            inputsAdd.Copy(conv0OutputsWmma[matId][1]);

                            [unroll]
                            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                            {
                                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                                quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                                float residuals = float(inputsAdd.Element(i)) * quantScale1;
                                quantizedElement += residuals;

                                quantizedElement *= (1.0/quantOutputScale1);
                                quantizedElement = round(quantizedElement);
                                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                            }

                            outputMats[matId][1].Copy(accumulatorMatrix[matId]);
                        }
                    }
                }

                uint storeOffset = output.OffsetOf(poBase);
                outputMats[0][0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
                outputMats[0][1].Store(output.storage.buffer, storeOffset + 16, output.storageByteStrides.x, true);
                storeOffset += 16 * output.storageByteStrides.x;
                outputMats[1][0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
                outputMats[1][1].Store(output.storage.buffer, storeOffset + 16, output.storageByteStrides.x, true);
            }
        }
    }
    else if (numFeatures == 64 && numGroups == 2)
    {
        [unroll]
        for (uint y = 0; y < perThreadWork.y; y++)
        {
            [unroll]
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv0OutputsWmma[2][64/16];

                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2][64 / 16];
                [unroll]
                for (uint f = 0; f < weights0.logicalSize.w; f += 16)
                {
                    accumulatorMatrix[0][f / 16].Load(bias0.storage.buffer, bias0.OffsetOf(f), 0 , true);
                    accumulatorMatrix[1][f / 16].Copy(accumulatorMatrix[0][f / 16]);
                }

                [unroll]
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                {
                    [unroll]
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        [unroll]
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
                        for (uint i = 0; i < accumulatorMatrix[matId][f / 16].Length(); i++)
                        {
                            float quantizedElement = float(accumulatorMatrix[matId][f / 16].Element(i));
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
                    accumulatorMatrix[0].Load(bias1.storage.buffer, bias1.OffsetOf(f), 0 , true);
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
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> outputMats[2][64/16];

                [unroll]
                for (uint f = 0; f < weights2.logicalSize.w; f += 16)
                {
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> accumulatorMatrix[2];
                    accumulatorMatrix[0].Load(bias2.storage.buffer, bias2.OffsetOf(f), 0 , true);
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
                            const uint inputOffset = input.OffsetOf(poBase + int3(matId * 16, 0, f));
                            inputsAdd.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                            inputsAddI32.Copy(inputsAdd);

                            [unroll]
                            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                            {
                                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                                quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                                float residuals = float(inputsAddI32.Element(i)) * quantScale0;
                                quantizedElement += residuals;

                                quantizedElement *= 1.0/quantOutputScale0;
                                quantizedElement = round(quantizedElement);
                                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                            }

                            outputMats[matId][f/16].Copy(accumulatorMatrix[matId]);
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

                                quantizedElement *= 1.0/quantOutputScale1;
                                quantizedElement = round(quantizedElement);
                                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                            }

                            outputMats[matId][f/16].Copy(accumulatorMatrix[matId]);
                        }
                    }
                }

                uint storeOffset = output.OffsetOf(poBase);
                outputMats[0][0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
                outputMats[0][1].Store(output.storage.buffer, storeOffset + 16, output.storageByteStrides.x, true);
                outputMats[0][2].Store(output.storage.buffer, storeOffset + 32, output.storageByteStrides.x, true);
                outputMats[0][3].Store(output.storage.buffer, storeOffset + 48, output.storageByteStrides.x, true);
                // NOTE: We have to do this because megafused can sometimes not be divisible by 32
                // in MLSR. This is a bit hacky, but it solves the problem for now
                //if ((poBase.x + 32) <= output.logicalSize.x)
                {
                    storeOffset += 16 * output.storageByteStrides.x;
                    outputMats[1][0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
                    outputMats[1][1].Store(output.storage.buffer, storeOffset + 16, output.storageByteStrides.x, true);
                    outputMats[1][2].Store(output.storage.buffer, storeOffset + 32, output.storageByteStrides.x, true);
                    outputMats[1][3].Store(output.storage.buffer, storeOffset + 48, output.storageByteStrides.x, true);
                }
            }
        }
    }
}

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const Tensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWNC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_HWNC< SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_HWNC< SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);

    int3 workOffset = 0;
    if (numFeatures == 32)
    {
        workOffset = output.threadGroupSliceStart;
    }
    else
    {
        workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    }


    if (numFeatures == 32 && numGroups == 1)
    {
        [unroll]
        for (uint y = 0; y < perThreadWork.y; y++)
        {
            [unroll]
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                //if (!ValidPosition(output, poBase))
                //    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv0OutputsWmma[2][32 / 16];

                //TODO Generalize to more than 16
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2];
                accumulatorMatrix[0].Load(bias0.storage.buffer, bias0.OffsetOf(0), 0, true);
                accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

                [unroll]
                for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
                {
                    [unroll]
                    for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
                    {
                        for (uint f = 0; f < weights0.logicalSize.w; f += 16)
                        {
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

                [unroll]
                for (uint f = 0; f < weights0.logicalSize.w; f += 16)
                {
                    [unroll]
                    for (uint matId = 0; matId < 2; ++matId)
                    {
                        [unroll]
                        for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                        {
                            float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                            quantizedElement *= quantScale0 * weights0.quantizationScale;
                            quantizedElement *= (1.0 / quantScale1);
                            quantizedElement = round(quantizedElement);
                            quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                            accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                        }

                        conv0OutputsWmma[matId][0].Copy(accumulatorMatrix[matId]);
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
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> conv1OutputsWmma[32 / 16][4];

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
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> outputMats[2][2];

                [unroll]
                for (uint f = 0; f < weights2.logicalSize.w; f += 16)
                {
                    // NOTE: Bias loading
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

                    if ((f/16) == 0)
                    {
                        [unroll]
                        for (uint matId = 0; matId < 2; ++matId)
                        {
                            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputs;
                            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> inputsI32;
                            const uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 0) + int3(matId * 16, 0, 0));
                            inputs.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                            inputsI32.Copy(inputs);

                            [unroll]
                            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                            {
                                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                                quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                                float residuals = float(inputsI32.Element(i)) * quantScale0;
                                quantizedElement += residuals;

                                quantizedElement *= (1.0 / output.quantizationScale);
                                quantizedElement = round(quantizedElement);
                                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                            }

                            outputMats[matId][0].Copy(accumulatorMatrix[matId]);
                        }
                    }
                    else
                    {
                        [unroll]
                        for (uint matId = 0; matId < 2; ++matId)
                        {
                            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> inputsAdd;
                            inputsAdd.Copy(conv0OutputsWmma[matId][1]);

                            [unroll]
                            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                            {
                                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                                quantizedElement *= weights2.quantizationScale * activationScaleFactor;

                                float residuals = float(inputsAdd.Element(i)) * quantScale1;
                                quantizedElement += residuals;

                                quantizedElement *= (1.0/output.quantizationScale);
                                quantizedElement = round(quantizedElement);
                                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
                            }

                            outputMats[matId][1].Copy(accumulatorMatrix[matId]);
                        }
                    }
                }

                uint storeOffset = output.OffsetOf(poBase);
                outputMats[0][0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
                outputMats[0][1].Store(output.storage.buffer, storeOffset + 16, output.storageByteStrides.x, true);
                storeOffset += 16 * output.storageByteStrides.x;
                outputMats[1][0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
                outputMats[1][1].Store(output.storage.buffer, storeOffset + 16, output.storageByteStrides.x, true);
            }
        }
    }

}
#endif // #if WMMA_ENABLED
