// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

#if WMMA_ENABLED
#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"
#endif

template<typename SW>
half4 ct2d_iteration_4(
    const uint z,
    const int2 pk,
    const uint4 packedVs,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const float inputsScale)
{
    int4 accumulator = 0;
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w0 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.x = dot4add_i8packed(w0.x, packedVs.x, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.y, packedVs.y, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.z, packedVs.z, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.w, packedVs.w, accumulator.x);
    }

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w1 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.y = dot4add_i8packed(w1.x, packedVs.x, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.y, packedVs.y, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.z, packedVs.z, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.w, packedVs.w, accumulator.y);
    }

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w2 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.z = dot4add_i8packed(w2.x, packedVs.x, accumulator.z);
        accumulator.z = dot4add_i8packed(w2.y, packedVs.y, accumulator.z);
        accumulator.z = dot4add_i8packed(w2.z, packedVs.z, accumulator.z);
        accumulator.z = dot4add_i8packed(w2.w, packedVs.w, accumulator.z);
    }


    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w3 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.w = dot4add_i8packed(w3.x, packedVs.x, accumulator.w);
        accumulator.w = dot4add_i8packed(w3.y, packedVs.y, accumulator.w);
        accumulator.w = dot4add_i8packed(w3.z, packedVs.z, accumulator.w);
        accumulator.w = dot4add_i8packed(w3.w, packedVs.w, accumulator.w);
    }

    return half4(accumulator * inputsScale.xxxx);
}

template<typename SW, typename SB>
half2 ct2d_iteration_2(
    const uint z,
    const int2 pk,
    const uint4 packedVs,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const Tensor1h<SB> bias,
    const float inputsScale)
{
    int2 accumulator = 0;
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w0 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.x = dot4add_i8packed(w0.x, packedVs.x, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.y, packedVs.y, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.z, packedVs.z, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.w, packedVs.w, accumulator.x);
    }

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w1 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.y = dot4add_i8packed(w1.x, packedVs.x, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.y, packedVs.y, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.z, packedVs.z, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.w, packedVs.w, accumulator.y);
    }

    return half2(accumulator * inputsScale.xx + Load2hA(bias, z));
}

template<int numOutputChannels, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void CNB_CT2D(
    const float rcprOutputScale0,
    const float inputScale1,
    const float rcprOutputScale1,
    const float inputScale2,
    const float cnbOutputScale,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const QuantizedTensor4i8_HWCN<SW3> ct2d_weights,
    const Tensor1h<SB3> ct2d_bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint numFeatures = weights0.logicalSize.w;
    const int3 workOffset = int3(computeShaderParams.dispatchThreadID.xy, 0);

    const int3 poBase = workOffset;
    const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

    // CNB
    int accumulator[16];
    int relu_output[32];
    [unroll]
    for (uint i = 0; i < 16; ++i) { accumulator[i] = 0; }
    [unroll]
    for (uint i = 0; i < 32; ++i) { relu_output[i] = 0; }

    // First convolution
    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                continue;

            int8_t4_packed vs[16/4];
            uint inputIndex = 0;
            [unroll]
            for (uint c = 0; c != weights0.logicalSize.z; c += 16)
            {
                const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, c));
                const uint4 inputDwords = input.storage.Load4(inputOffset);
                vs[inputIndex++] = inputDwords.x;
                vs[inputIndex++] = inputDwords.y;
                vs[inputIndex++] = inputDwords.z;
                vs[inputIndex++] = inputDwords.w;
            }

            [unroll]
            for (uint f = 0; f != numFeatures; ++f)
            {
                inputIndex = 0;
                for (uint c = 0; c != weights0.logicalSize.z; c += 16)
                {
                    const uint weightsOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                    const uint4 weightsDwords = weights0.storage.Load4(weightsOffset);

                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.x, accumulator[f]);
                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.y, accumulator[f]);
                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.z, accumulator[f]);
                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.w, accumulator[f]);
                }
            }
        }

    int8_t4_packed conv_result[16/4];
    [unroll]
    float inputscale = input.quantizationScale * weights0.quantizationScale;
    for (uint f = 0; f != numFeatures; f+=4)
    {
        const float4 conv_accumulator = float4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int4 quantized_vs = round((conv_accumulator * inputscale + Load4hA(bias0, f)) * rcprOutputScale0);
        conv_result[f/4] = pack_clamp_s8(quantized_vs);
    }

    // Second Convolution + Relu
    [unroll]
    for (uint i = 0; i < 32; i++)
    {
        const uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
        const uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

        relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
        relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
        relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
        relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);
    }

    int8_t4_packed relu_result[32/4];
    inputscale = inputScale1 * weights1.quantizationScale;
    [unroll]
    for (uint i = 0; i < 32; i+=4)
    {
        const float4 vs = float4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
        const int4 quantized_vs = round((vs * inputscale + Load4hA(bias1, i)) * rcprOutputScale1);
        relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
    }

    int conv3_output[16];
    [unroll]
    for (int i = 0; i < 16; ++i) { conv3_output[i] = 0; }

    // Third Convolution
    [unroll]
    for (uint i = 0; i < 16; i++)
    {
        uint weightsOffset = weights2.OffsetOf(uint4(0,0,0,i));
        uint4 weightsDwords = weights2.storage.Load4(weightsOffset);

        conv3_output[i] = dot4add_i8packed(relu_result[0], weightsDwords.x, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[1], weightsDwords.y, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[2], weightsDwords.z, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[3], weightsDwords.w, conv3_output[i]);

        weightsOffset = weights2.OffsetOf(uint4(0,0,16,i));
        weightsDwords = weights2.storage.Load4(weightsOffset);

        conv3_output[i] = dot4add_i8packed(relu_result[4], weightsDwords.x, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[5], weightsDwords.y, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[6], weightsDwords.z, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[7], weightsDwords.w, conv3_output[i]);
    }

    // Add
    const uint4 inputDwords = input.storage.Load4(input.OffsetOf(int3(poBase.xy, 0)));

    uint4 packedVss;
    inputscale = weights2.quantizationScale * inputScale2;
    const float outputRcprScale = 1.0f / cnbOutputScale;
    [unroll]
    for (uint i = 0; i < 16; i+=4)
    {
        const float4 vs = float4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * inputscale + Load4hA(bias2, i);
        result += unpack_s8s32(inputDwords[i/4]) * input.quantizationScale;

        packedVss[i/4] = pack_clamp_s8(int4(round(result * outputRcprScale)));
    }

    // CT2D
    const float ct2d_input_quantizationScale = cnbOutputScale * ct2d_weights.quantizationScale;
    const int2 localOffset[4] = { int2(0,0), int2(1,0), int2(0,1), int2(1,1) };

    if (numOutputChannels == 8)
    {
        uint4 biasDwords = ct2d_bias.storage.Load4(ct2d_bias.OffsetOf(0));

        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            const int2 pk = localOffset[i];
            const half4 first = ct2d_iteration_4(0, pk, packedVss, ct2d_weights, ct2d_input_quantizationScale) + Unpack4h(biasDwords.xy);
            const half4 second = ct2d_iteration_4(4, pk, packedVss, ct2d_weights, ct2d_input_quantizationScale) + Unpack4h(biasDwords.zw);

            // Store4
            uint4 storeDwords;
            storeDwords.xy = Pack4h(first);
            storeDwords.zw = Pack4h(second);

            const uint2 poBase2D = computeShaderParams.dispatchThreadID.xy * 2 + pk;
            const uint storeOffset = output.OffsetOf(int3(poBase2D, 0));
            output.storage.Store4(storeOffset, storeDwords);
        }
    }
    else if (numOutputChannels == 6)
    {
        const uint2 biasDwords = ct2d_bias.storage.Load2(ct2d_bias.OffsetOf(0));
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            const int2 pk = localOffset[i];
            const half4 first = ct2d_iteration_4(0, pk, packedVss, ct2d_weights, ct2d_input_quantizationScale) + Unpack4h(biasDwords.xy);
            const half2 second = ct2d_iteration_2(4, pk, packedVss, ct2d_weights, ct2d_bias, ct2d_input_quantizationScale);

            // Store4
            uint3 storeDwords;
            storeDwords.xy = Pack4h(first);
            storeDwords.z  = Pack2h(second);

            const uint2 poBase2D = computeShaderParams.dispatchThreadID.xy * 2 + pk;
            const uint storeOffset = output.OffsetOf(int3(poBase2D, 0));
            output.storage.Store3(storeOffset, storeDwords);
        }
    }

}

#if WMMA_ENABLED
template<int numOutputChannels, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void CNB_CT2D(
    const float rcprOutputScale0,
    const float inputScale1,
    const float rcprOutputScale1,
    const float inputScale2,
    const float cnbOutputScale,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWNC<SW0> weights0,
    const QuantizedTensor1i32<SB0> bias0,
    const QuantizedTensor4i8_HWNC< SW1> weights1,
    const QuantizedTensor1i32<SB1> bias1,
    const QuantizedTensor4i8_HWNC< SW2> weights2,
    const QuantizedTensor1i32<SB2> bias2,
    const QuantizedTensor4i8_HWCN< SW3> ct2d_weights,
    const QuantizedTensor1i32<SB3> ct2d_bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const int3 workOffset = computeShaderParams.groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > conv0OutputsWmma[2];

    // BIAS
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2];
    uint byteAddress = bias0.OffsetOf(0);
    accumulatorMatrix[0].Load(bias0.storage.buffer, byteAddress, 0, true);
    accumulatorMatrix[1].Copy(accumulatorMatrix[0]);//Load(bias0.storage.buffer, byteAddress, 0, true);

    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            [unroll]
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

    //TODO Generalize to cases other than 16 channels
    [unroll]
    for (uint matId = 0; matId < 2; ++matId)
    {
        [unroll]
        for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
        {
            int matElement = accumulatorMatrix[matId].Element(i);
            float quantizedElement = float(matElement);
            quantizedElement = quantizedElement * input.quantizationScale * weights0.quantizationScale;
            quantizedElement *= rcprOutputScale0;
            quantizedElement = round(quantizedElement);
            quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);
            int quantizedResult = int(quantizedElement);

            accumulatorMatrix[matId].SetElement(i, quantizedResult);
        }

        conv0OutputsWmma[matId].Copy(accumulatorMatrix[matId]);
    }

    // Second Convolution + Relu
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> conv1OutputsWmma[32 / 16][2];

    [unroll]
    for (uint f = 0; f < weights1.logicalSize.w; f += 16)
    {
        // BIAS
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> accumulatorMatrix[2];
        uint byteAddress = bias1.OffsetOf(f);
        accumulatorMatrix[0].Load(bias1.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);//Load(bias1.storage.buffer, byteAddress, 0, true);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights1.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> inputMatrix = conv0OutputsWmma[inputWaveOffset];

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
                int matElement = accumulatorMatrix[matId].Element(i);
                float quantizedElement = float(matElement);
                quantizedElement *= inputScale1 * weights1.quantizationScale;
                quantizedElement *= rcprOutputScale1;
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, 0.0f, 127.0f); //Dropping the lower half as ReLu implementation
                int quantizedResult = int(quantizedElement);

                accumulatorMatrix[matId].SetElement(i, quantizedResult);
            }

            conv1OutputsWmma[matId][f/16].Copy(accumulatorMatrix[matId]);
        }
    }

    // Third Convolution
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> outputMats[2];
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16> residualMats[2];
    {
        int loadOffset = input.OffsetOf(int3(piBase.xy + int2(1, 1), 0));
        residualMats[0].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
        loadOffset = input.OffsetOf(int3(piBase.xy + int2(16 + 1, 1), 0));
        residualMats[1].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
    }

    [unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        // BIAS
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> accumulatorMatrix[2];
        uint byteAddress = bias2.OffsetOf(f);
        accumulatorMatrix[0].Load(bias2.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);//Load(bias2.storage.buffer, byteAddress, 0, true);

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

        const float rcprOutputScale2 = 1.0 / cnbOutputScale;

        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16> residualI32;
            residualI32.Copy(residualMats[matId]);

            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                quantizedElement *= inputScale2 * weights2.quantizationScale;
                float residual = float(residualI32.Element(i)) * input.quantizationScale;
                quantizedElement += residual;
                quantizedElement *= rcprOutputScale2;
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);

                accumulatorMatrix[matId].SetElement(i, int(quantizedElement));
            }

            outputMats[matId].Copy(accumulatorMatrix[matId]);
        }
    }

    // Fourth convolution (x = 2, y = 2, half = 2, W = 1)
    float4 ctBias[2];
    ctBias[0] = Load4i32(ct2d_bias, 0) * ct2d_bias.quantizationScale;
    ctBias[1] = Load4i32(ct2d_bias, 4) * ct2d_bias.quantizationScale;

    for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
    {
        for (uint ky = 0; ky < ct2d_weights.logicalSize.y; ++ky)
        {
            uint kx = 0;
            {
                // Note: Biases init to zero, add later
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix;
                accumulatorMatrix.Fill(0);

                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix = outputMats[inputWaveOffset];

                uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, 0, 0));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);

                half result[8];
                [unroll]
                for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                {
                    float matElement = (float) accumulatorMatrix.Element(i);
                    matElement *= cnbOutputScale * ct2d_weights.quantizationScale;
                    matElement += ctBias[i/4][i%4];

                    result[i] = matElement;
                }

                // Store4
                uint4 storeDwords;
                uint storeOffset = output.OffsetOf(int3(poBase) + int3(WaveGetLaneIndex()/16,ky,0) + int3(stride * inputWaveOffset * 16, 0, 0) + int3(stride * (WaveGetLaneIndex()%16), 0, 0));

                storeDwords.xy = Pack4h(half4(result[0], result[1], result[2], result[3]));
                storeDwords.zw = Pack4h(half4(result[4], result[5], result[6], result[7]));
                output.storage.Store4(storeOffset, storeDwords);
            }
        }
    }
}

template<int numOutputChannels, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void CNB_CT2D(
    const float rcprOutputScale0,
    const float inputScale1,
    const float rcprOutputScale1,
    const float inputScale2,
    const float cnbOutputScale,
    const QuantizedTensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC< SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC< SW2> weights2,
    const Tensor1f<SB2> bias2,
    const QuantizedTensor4f8_HWCN< SW3> ct2d_weights,
    const Tensor1f<SB3> ct2d_bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const int3 workOffset = computeShaderParams.groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv0OutputsWmma[2];

    // BIAS
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
    uint byteAddress = bias0.OffsetOf(0);
    accumulatorMatrix[0].Load(bias0.storage.buffer, byteAddress, 0, true);
    accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            [unroll]
            for (uint f = 0; f < weights0.logicalSize.w; f += 16)
            {
                [unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {
                    [unroll]
                    for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                    {
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                        int inputOffset = input.OffsetOf(piBase + int3(kx + inputWaveOffset * 16, ky, c));
                        inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);

                        uint weightOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                        weightMatrix.Load(weights0.storage.buffer, weightOffset, weights0.storageByteStrides.w, false);

                        accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
                    }
                }
            }
        }
    }

    [unroll]
    for (uint matId = 0; matId < 2; ++matId)
    {
        conv0OutputsWmma[matId].CopySat(accumulatorMatrix[matId]);
    }

    // Second Convolution + Relu
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1OutputsWmma[32 / 16][2];

    [unroll]
    for (uint f = 0; f < weights1.logicalSize.w; f += 16)
    {
        // BIAS
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
        uint byteAddress = bias1.OffsetOf(f);
        accumulatorMatrix[0].Load(bias1.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights1.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv0OutputsWmma[inputWaveOffset];

                int weightOffset = weights1.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
                weightMatrix.Load(weights1.storage.buffer, weightOffset, weights1.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }

            // Relu()
            [unroll]
            for (uint i = 0; i < accumulatorMatrix[inputWaveOffset].Length(); i++)
            {
                float elem = accumulatorMatrix[inputWaveOffset].Element(i);
                accumulatorMatrix[inputWaveOffset].SetElement(i, max(elem, 0.0f));
            }
        }


        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            conv1OutputsWmma[matId][f/16].CopySat(accumulatorMatrix[matId]);
        }
    }

    // Third Convolution
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> outputMats[2];
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> residualMats[2];
    {
        int loadOffset = input.OffsetOf(int3(piBase.xy + int2(1, 1), 0));
        residualMats[0].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
        loadOffset = input.OffsetOf(int3(piBase.xy + int2(16 + 1, 1), 0));
        residualMats[1].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
    }

    [unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        // BIAS
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
        uint byteAddress = bias2.OffsetOf(f);
        accumulatorMatrix[0].Load(bias2.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights2.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv1OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights2.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
                weightMatrix.Load(weights2.storage.buffer, weightOffset, weights2.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> residualF32;
            residualF32.Copy(residualMats[matId]);

            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
            {
                float accumulatorValue = float(accumulatorMatrix[matId].Element(i));
                float residual = float(residualF32.Element(i));
                accumulatorValue += residual;

                accumulatorMatrix[matId].SetElement(i, accumulatorValue);
            }

            outputMats[matId].CopySat(accumulatorMatrix[matId]);
        }
    }

    // Fourth convolution (x = 2, y = 2, half = 2, W = 1)
    float4 ctBias[2];
    ctBias[0] = Load4f(ct2d_bias, 0);
    ctBias[1] = Load4f(ct2d_bias, 4);

    for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
    {
        for (uint ky = 0; ky < ct2d_weights.logicalSize.y; ++ky)
        {
            uint kx = 0;
            {
                // Note: Biases init to zero, add later
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix;
                accumulatorMatrix.Fill(0);

                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix = outputMats[inputWaveOffset];

                uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, 0, 0));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);

                half result[8];
                [unroll]
                for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                {
                    float matElement = (float)accumulatorMatrix.Element(i);
                    matElement += ctBias[i/4][i%4];

                    result[i] = matElement;
                }

                // Store4
                uint4 storeDwords;
                uint storeOffset = output.OffsetOf(int3(poBase) + int3(WaveGetLaneIndex()/16,ky,0) + int3(stride * inputWaveOffset * 16, 0, 0) + int3(stride * (WaveGetLaneIndex()%16), 0, 0));

                storeDwords.xy = Pack4h(half4(result[0], result[1], result[2], result[3]));
                storeDwords.zw = Pack4h(half4(result[4], result[5], result[6], result[7]));
                output.storage.Store4(storeOffset, storeDwords);
            }
        }
    }
}
#endif // #if WMMA_ENABLED
