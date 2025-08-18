// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

#if WMMA_ENABLED

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"

template<typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void ConvNextBlock(
    const float rcprOutputScale0,
    const float inputScale1,
    const float rcprOutputScale1,
    const float inputScale2,
    const QuantizedTensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC<SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC<SW2> weights2,
    const Tensor1f<SB2> bias2,
    const QuantizedTensor3f8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const int X = 1;
    const int Y = 8;

    const int3 workOffset = computeShaderParams.groupID * int3(16*X, Y, 1);
    const uint numFeatures = weights0.logicalSize.w;

    const int3 poBase = workOffset;
    const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

    // Preload weights
    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix[3][3];
    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix1[2];
    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix2[2];
    {
        uint weightOffset = weights0.threadGroupStorageByteOffset;
        for (int ky = 0; ky < 3; ++ky)
            for (int kx = 0; kx < 3; ++kx)
            {
                weightMatrix[ky][kx].Load(weights0.storage.buffer, weightOffset, 16, false);
                weightOffset += 256;
            }
    }
    {
        uint weightOffset = weights1.threadGroupStorageByteOffset;
        for (uint f = 0; f < 2; ++f)
        {
            weightMatrix1[f].Load(weights1.storage.buffer, weightOffset, 16, false);
            weightOffset += 256;
        }
    }
    {
        uint weightOffset = weights2.threadGroupStorageByteOffset;
        weightMatrix2[0].Load(weights2.storage.buffer, weightOffset, 32, false);
        weightMatrix2[1].Load(weights2.storage.buffer, weightOffset+16, 32, false);
        // weightMatrix2[0].Load(weights2.storage.buffer, weightOffset, 16, false);
        // weightMatrix2[1].Load(weights2.storage.buffer, weightOffset+256, 16, false);
    }


    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator3x3[X][Y];
    accumulator3x3[0][0].Load(bias0.storage.buffer, bias0.threadGroupStorageByteOffset, 0, true);
    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
            accumulator3x3[xmat][ymat].Copy(accumulator3x3[0][0]);

    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
    {
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            for (int ymat = 0; ymat < Y; ++ymat)
            {
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                    int inputOffset = input.OffsetOf(piBase + int3(kx + xmat * 16, ky + ymat, 0));
                    inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);

                    accumulator3x3[xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix[ky][kx], inputMatrix, accumulator3x3[xmat][ymat]);
                }

            }
        }
    }

    // Quantize
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv3x3_output[X][Y];
    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
            conv3x3_output[xmat][ymat].CopySat(accumulator3x3[xmat][ymat]);

                                                                                        // N (output_channels), X, Y
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1x1_expand_output[2][X][Y];
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1x1_contract_output[X][Y];

    // 1x1 expand + activation + quantize
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator[32/16][X][Y];
        const uint bias1Offset = bias1.threadGroupStorageByteOffset;
        accumulator[0][0][0].Load(bias1.storage.buffer, bias1Offset, 0, true);
        accumulator[1][0][0].Load(bias1.storage.buffer, bias1Offset+16*4, 0, true);
        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
            {
                accumulator[0][xmat][ymat].Copy(accumulator[0][0][0]);
                accumulator[1][xmat][ymat].Copy(accumulator[1][0][0]);
            }

        for (int C = 0; C < 2; ++C)
        {
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    accumulator[C][xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix1[C], conv3x3_output[xmat][ymat], accumulator[C][xmat][ymat]);
                }
        }


        // Relu
        for (int C = 0; C < 2; ++C)
        {
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    for (uint i = 0; i < accumulator[C][xmat][ymat].Length(); i++)
                    {
                        float elem = accumulator[C][xmat][ymat].Element(i);
                        accumulator[C][xmat][ymat].SetElement(i, max(0.0f, elem));
                    }

                    // Quantize
                    conv1x1_expand_output[C][xmat][ymat].CopySat(accumulator[C][xmat][ymat]);
                }
        }
    }

    // Load Residual connection
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> fp8Residual[X][Y];
    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
        {
            int inputOffset = input.OffsetOf(int3(piBase.xy + int2(1 + 16*xmat, 1 + ymat), 0));
            fp8Residual[xmat][ymat].Load(input.storage.buffer, inputOffset, 16, true);
        }

    // 1x1 contract + residual add + quantize
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator[X][Y];
        accumulator[0][0].Load(bias2.storage.buffer, bias2.threadGroupStorageByteOffset, 0, true);
        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
                accumulator[xmat][ymat].Copy(accumulator[0][0]);


        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
            {
                accumulator[xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix2[0], conv1x1_expand_output[0][xmat][ymat], accumulator[xmat][ymat]);
            }

        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
            {
                accumulator[xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix2[1], conv1x1_expand_output[1][xmat][ymat], accumulator[xmat][ymat]);
            }

        // Residual connection
        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
            {
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> fp32Residual;
                fp32Residual.Copy(fp8Residual[xmat][ymat]);

                for (int i = 0; i < accumulator[xmat][ymat].Length(); i++)
                {
                    float elem = accumulator[xmat][ymat].Element(i);
                    accumulator[xmat][ymat].SetElement(i, elem + fp32Residual.Element(i));
                }

                // Quantize
                conv1x1_contract_output[xmat][ymat].CopySat(accumulator[xmat][ymat]);
            }
    }

    // Store output
    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
        {
            const uint outputOffset = output.OffsetOf(poBase + int3(16*xmat,ymat,0));
            conv1x1_contract_output[xmat][ymat].Store(output.storage.buffer, outputOffset, 16, true);
        }
}

#else // #if WMMA_ENABLED
#error To use FP8 data type you need to provide WMMA_ENABLED=1 in hlsl_defines. There is no FP8 without WMMA.
#endif // #if WMMA_ENABLED
