// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

#if WMMA_ENABLED

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC< SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC< SW2> weights2,
    const Tensor1f<SB2> bias2,
    Tensor3f8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (numFeatures == 32 && numGroups == 1)
    {
        const int X = 2;
        const int Y = 4;

        const int3 workOffset = computeShaderParams.groupID * int3(16*X, Y, 1);

        const int3 poBase = workOffset;
        const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

        // Preload weights
        AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix[3][3];
        AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix1[4][2];
        AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix2[2][4];
        {
            uint weightOffset = weights0.threadGroupStorageByteOffset;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx)
                {
                    weightMatrix[ky+1][kx+1].Load(weights0.storage.buffer, weightOffset, 16, false);
                    weightOffset += 256;
                }
        }
        {
            uint weightOffset = weights1.threadGroupStorageByteOffset;
            for (uint i = 0; i < 4; ++i)
            {
                // N[i*16:i*16+16] | C[:16]
                weightMatrix1[i][0].Load(weights1.storage.buffer, weightOffset, 32, false);
                //                   C[16:]
                weightMatrix1[i][1].Load(weights1.storage.buffer, weightOffset+16, 32, false);

                weightOffset += 256 * 2;
            }
        }
        {
            uint weightOffset = weights2.threadGroupStorageByteOffset;
            for (uint i = 0; i < 2; ++i)
            {
                weightMatrix2[i][0].Load(weights2.storage.buffer, weightOffset,    64, false);
                weightMatrix2[i][1].Load(weights2.storage.buffer, weightOffset+16, 64, false);
                weightMatrix2[i][2].Load(weights2.storage.buffer, weightOffset+32, 64, false);
                weightMatrix2[i][3].Load(weights2.storage.buffer, weightOffset+48, 64, false);

                weightOffset += 256 * 4;
            }
        }

        // Load input [16:32]
        AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv3x3_output[2][X][Y];
        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
            {
                uint inputOffset = input.OffsetOf(poBase + int3(xmat*16, ymat, 16));
                conv3x3_output[1][xmat][ymat].Load(input.storage.buffer, inputOffset, 32, true);
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
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                    conv3x3_output[0][xmat][ymat].CopySat(accumulator3x3[xmat][ymat]);


                                                                                    // N (output_channels), X, Y
        AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1x1_expand_output[4][X][Y];
        AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1x1_contract_output[2][X][Y];

        // 1x1 expand + activation + quantize
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator[4][X][Y];
            for (int i = 0; i < 4; i++)
            {
                accumulator[i][0][0].Load(bias1.storage.buffer, bias1.threadGroupStorageByteOffset + i*(16*4), 0, true);
                for (int ymat = 0; ymat < Y; ++ymat)
                    for (int xmat = 0; xmat < X; ++xmat)
                        accumulator[i][xmat][ymat].Copy(accumulator[i][0][0]);
            }

            for (int i = 0; i < 4; ++i)
            {
                for (int ymat = 0; ymat < Y; ++ymat)
                    for (int xmat = 0; xmat < X; ++xmat)
                    {
                        for (int c = 0; c < 2; ++c)
                        {
                            accumulator[i][xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix1[i][c], conv3x3_output[c][xmat][ymat], accumulator[i][xmat][ymat]);
                        }
                    }
            }


            // Relu
            for (int c = 0; c < 4; ++c)
            {
                for (int ymat = 0; ymat < Y; ++ymat)
                    for (int xmat = 0; xmat < X; ++xmat)
                    {
                        for (uint i = 0; i < accumulator[c][xmat][ymat].Length(); i++)
                        {
                            float elem = accumulator[c][xmat][ymat].Element(i);
                            accumulator[c][xmat][ymat].SetElement(i, max(0.0f, elem));
                        }

                        // Quantize
                        conv1x1_expand_output[c][xmat][ymat].CopySat(accumulator[c][xmat][ymat]);
                    }
            }
        }

        // Load Residual connection
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> fp8Residual[2][X][Y];
        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
            {
                uint inputOffset = input.OffsetOf(poBase + int3(xmat*16, ymat, 0));

                fp8Residual[0][xmat][ymat].Load(input.storage.buffer, inputOffset, 32, true);
                fp8Residual[1][xmat][ymat].Copy(conv3x3_output[1][xmat][ymat]);
            }


        // 1x1 contract + residual add + quantize
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator[2][X][Y];
            for (int i = 0; i < 2; i++)
            {
                accumulator[i][0][0].Load(bias2.storage.buffer, bias2.threadGroupStorageByteOffset + i*(16*4), 0, true);
                for (int ymat = 0; ymat < Y; ++ymat)
                    for (int xmat = 0; xmat < X; ++xmat)
                        accumulator[i][xmat][ymat].Copy(accumulator[i][0][0]);
            }

            for (int i = 0; i < 2; ++i)
            {
                for (int ymat = 0; ymat < Y; ++ymat)
                    for (int xmat = 0; xmat < X; ++xmat)
                    {
                        for (int c = 0; c < 4; ++c)
                        {
                            accumulator[i][xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix2[i][c], conv1x1_expand_output[c][xmat][ymat], accumulator[i][xmat][ymat]);
                        }
                    }
            }

            // Residual connection
            for (int c = 0; c < 2; ++c)
                for (int ymat = 0; ymat < Y; ++ymat)
                    for (int xmat = 0; xmat < X; ++xmat)
                    {
                        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> fp32Residual;
                        fp32Residual.Copy(fp8Residual[c][xmat][ymat]);

                        for (int i = 0; i < accumulator[c][xmat][ymat].Length(); i++)
                        {
                            float elem = accumulator[c][xmat][ymat].Element(i);
                            accumulator[c][xmat][ymat].SetElement(i, elem + fp32Residual.Element(i));
                        }

                        // Quantize
                        conv1x1_contract_output[c][xmat][ymat].CopySat(accumulator[c][xmat][ymat]);
                    }
        }

        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
                for (int i = 0; i < 2; ++i)
                {
                    uint storeOffset = output.OffsetOf(poBase + int3(xmat*16, ymat, i*16));
                    conv1x1_contract_output[i][xmat][ymat].Store(output.storage.buffer, storeOffset, 32, true);
                }
        
    }
    else if (numFeatures == 64 && numGroups == 2)
    {
        const int3 workOffset = output.threadGroupSliceStart;
        const int3 poBase = workOffset;
        const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv0OutputsWmma[2][64/16];
        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2][64 / 16];
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
                            AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                            int inputOffset = input.OffsetOf(piBase + int3(kx + inputWaveOffset * 16, ky, c + f));
                            inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);

                            uint weightOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                            AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
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
                conv0OutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId][f / 16]);
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
        AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1OutputsWmma[32 / 16][128/16];

        [unroll]
        for (uint f = 0; f < weights1.logicalSize.w; f += 16)
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
            accumulatorMatrix[0].Load(bias1.storage.buffer, bias1.OffsetOf(f), 0 , true);
            accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

            [unroll]
            for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
            {
                [unroll]
                for (uint c = 0; c < weights1.logicalSize.z; c += 16)
                {
                    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv0OutputsWmma[inputWaveOffset][c/16];

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
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> outputMats[2][64/16];

        [unroll]
        for (uint f = 0; f < weights2.logicalSize.w; f += 16)
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
            accumulatorMatrix[0].Load(bias2.storage.buffer, bias2.OffsetOf(f), 0 , true);
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

            if ((f/32) == 0)
            {
                [unroll]
                for (uint matId = 0; matId < 2; ++matId)
                {
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputsAdd;
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> inputsAddF32;
                    const uint inputOffset = input.OffsetOf(poBase + int3(matId * 16, 0, f));
                    inputsAdd.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                    inputsAddF32.Copy(inputsAdd);

                    [unroll]
                    for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                    {
                        float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                        float residuals = float(inputsAddF32.Element(i));
                        quantizedElement += residuals;

                        accumulatorMatrix[matId].SetElement(i, quantizedElement);
                    }

                    outputMats[matId][f/16].CopySat(accumulatorMatrix[matId]);
                }
            }
            else
            {
                [unroll]
                for (uint matId = 0; matId < 2; ++matId)
                {
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> inputsAddF32;
                    inputsAddF32.Copy(conv0OutputsWmma[matId][f/16]);

                    [unroll]
                    for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                    {
                        float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                        float residuals = float(inputsAddF32.Element(i));
                        quantizedElement += residuals;

                        accumulatorMatrix[matId].SetElement(i, quantizedElement);
                    }

                    outputMats[matId][f/16].CopySat(accumulatorMatrix[matId]);
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

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FasterNetBlock(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const Tensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC< SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC< SW2> weights2,
    const Tensor1f<SB2> bias2,
    QuantizedTensor3f8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const int X = 2;
    const int Y = 4;

    const int3 workOffset = computeShaderParams.groupID * int3(16*X, Y, 1);

    const int3 poBase = workOffset;
    const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

    // Preload weights
    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix[3][3];
    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix1[4][2];
    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix2[2][4];
    {
        uint weightOffset = weights0.threadGroupStorageByteOffset;
        for (int ky = -1; ky <= 1; ++ky)
            for (int kx = -1; kx <= 1; ++kx)
            {
                weightMatrix[ky+1][kx+1].Load(weights0.storage.buffer, weightOffset, 16, false);
                weightOffset += 256;
            }
    }
    {
        uint weightOffset = weights1.threadGroupStorageByteOffset;
        for (uint i = 0; i < 4; ++i)
        {
            // N[i*16:i*16+16] | C[:16]
            weightMatrix1[i][0].Load(weights1.storage.buffer, weightOffset, 32, false);
            //                   C[16:]
            weightMatrix1[i][1].Load(weights1.storage.buffer, weightOffset+16, 32, false);

            weightOffset += 256 * 2;
        }
    }
    {
        uint weightOffset = weights2.threadGroupStorageByteOffset;
        for (uint i = 0; i < 2; ++i)
        {
            weightMatrix2[i][0].Load(weights2.storage.buffer, weightOffset,    64, false);
            weightMatrix2[i][1].Load(weights2.storage.buffer, weightOffset+16, 64, false);
            weightMatrix2[i][2].Load(weights2.storage.buffer, weightOffset+32, 64, false);
            weightMatrix2[i][3].Load(weights2.storage.buffer, weightOffset+48, 64, false);

            weightOffset += 256 * 4;
        }
    }

    // Load input [16:32]
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv3x3_output[2][X][Y];
    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
        {
            uint inputOffset = input.OffsetOf(poBase + int3(xmat*16, ymat, 16));
            conv3x3_output[1][xmat][ymat].Load(input.storage.buffer, inputOffset, 32, true);
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
        for (int ymat = 0; ymat < Y; ++ymat)
            for (int xmat = 0; xmat < X; ++xmat)
                conv3x3_output[0][xmat][ymat].CopySat(accumulator3x3[xmat][ymat]);


                                                                                // N (output_channels), X, Y
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1x1_expand_output[4][X][Y];
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1x1_contract_output[2][X][Y];

    // 1x1 expand + activation + quantize
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator[4][X][Y];
        for (int i = 0; i < 4; i++)
        {
            accumulator[i][0][0].Load(bias1.storage.buffer, bias1.threadGroupStorageByteOffset + i*(16*4), 0, true);
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                    accumulator[i][xmat][ymat].Copy(accumulator[i][0][0]);
        }

        for (int i = 0; i < 4; ++i)
        {
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    for (int c = 0; c < 2; ++c)
                    {
                        accumulator[i][xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix1[i][c], conv3x3_output[c][xmat][ymat], accumulator[i][xmat][ymat]);
                    }
                }
        }


        // Relu
        for (int c = 0; c < 4; ++c)
        {
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    for (uint i = 0; i < accumulator[c][xmat][ymat].Length(); i++)
                    {
                        float elem = accumulator[c][xmat][ymat].Element(i);
                        accumulator[c][xmat][ymat].SetElement(i, max(0.0f, elem));
                    }

                    // Quantize
                    conv1x1_expand_output[c][xmat][ymat].CopySat(accumulator[c][xmat][ymat]);
                }
        }
    }

    // Load Residual connection
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> fp8Residual[2][X][Y];
    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
        {
            uint inputOffset = input.OffsetOf(poBase + int3(xmat*16, ymat, 0));

            fp8Residual[0][xmat][ymat].Load(input.storage.buffer, inputOffset, 32, true);
            fp8Residual[1][xmat][ymat].Copy(conv3x3_output[1][xmat][ymat]);
        }


    // 1x1 contract + residual add + quantize
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulator[2][X][Y];
        for (int i = 0; i < 2; i++)
        {
            accumulator[i][0][0].Load(bias2.storage.buffer, bias2.threadGroupStorageByteOffset + i*(16*4), 0, true);
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                    accumulator[i][xmat][ymat].Copy(accumulator[i][0][0]);
        }

        for (int i = 0; i < 2; ++i)
        {
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    for (int c = 0; c < 4; ++c)
                    {
                        accumulator[i][xmat][ymat] = AmdWaveMatrixMultiply(weightMatrix2[i][c], conv1x1_expand_output[c][xmat][ymat], accumulator[i][xmat][ymat]);
                    }
                }
        }

        // Residual connection
        for (int c = 0; c < 2; ++c)
            for (int ymat = 0; ymat < Y; ++ymat)
                for (int xmat = 0; xmat < X; ++xmat)
                {
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> fp32Residual;
                    fp32Residual.Copy(fp8Residual[c][xmat][ymat]);

                    for (int i = 0; i < accumulator[c][xmat][ymat].Length(); i++)
                    {
                        float elem = accumulator[c][xmat][ymat].Element(i);
                        accumulator[c][xmat][ymat].SetElement(i, elem + fp32Residual.Element(i));
                    }

                    // Quantize
                    conv1x1_contract_output[c][xmat][ymat].CopySat(accumulator[c][xmat][ymat]);
                }
    }

    for (int ymat = 0; ymat < Y; ++ymat)
        for (int xmat = 0; xmat < X; ++xmat)
            for (int i = 0; i < 2; ++i)
            {
                uint storeOffset = output.OffsetOf(poBase + int3(xmat*16, ymat, i*16));
                conv1x1_contract_output[i][xmat][ymat].Store(output.storage.buffer, storeOffset, 32, true);
            }
    
}

#else // #if WMMA_ENABLED
#error To use FP8 data type you need to provide WMMA_ENABLED=1 in hlsl_defines. There is no FP8 without WMMA.
#endif // #if WMMA_ENABLED
