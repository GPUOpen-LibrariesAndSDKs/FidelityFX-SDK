// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

#if WMMA_ENABLED

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"

// channels_storage * output_channels
groupshared uint skipOutputLDS[32*4*4];
template<typename SI,
    typename Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SW0, typename Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SB0,
    typename Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SW1, typename Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SB1,
    typename Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SW2, typename Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SB2,
    typename Conv2D_k2s2b_SW, typename Conv2D_k2s2b_BIAS,
    typename SO0, typename SO1>
void Fused_FusedConv2D_DW_Conv2D_PW_Relu_Conv2D_Add_FusedConv2D_k2s2b_QuantizedOutput(
    // FusedConv2D_DW_Conv2D_PW_Relu_Conv2D_Add factors.
    const float Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_rcprOutputScale0,
    const float Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_inputScale1,
    const float Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_rcprOutputScale1,
    const float Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_inputScale2,

    // FusedConv2D_k2s2b_QuantizedOutput factors.
    const float Conv2D_k2s2b_quantFactor0,
    const float Conv2D_k2s2b_quantFactor1,

    const QuantizedTensor3f8_NHWC<SI> input,

    // FusedConv2D_DW_Conv2D_PW_Relu_Conv2D_Add params.
    const QuantizedTensor4f8_HWNC<Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SW0> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0,
    const Tensor1f<Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SB0> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias0,
    const QuantizedTensor4f8_HWNC<Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SW1> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1,
    const Tensor1f<Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SB1> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias1,
    const QuantizedTensor4f8_HWNC<Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SW2> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2,
    const Tensor1f<Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_SB2> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias2,

    // FusedConv2D_k2s2b_QuantizedOutput params.
    const QuantizedTensor4f8_HWNC<Conv2D_k2s2b_SW> Conv2D_k2s2b_weights,
    const Conv2D_k2s2b_BIAS FusedConv2D_k2s2b_bias,

    QuantizedTensor3f8_NHWC<SO0> Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output,
    Tensor3f8_NHWC<SO1> Conv2D_k2s2b_output,
    const ComputeShaderParams computeShaderParams)
{
    const int3 workOffset = computeShaderParams.groupID * int3(32, 4, 1);
    const uint numFeatures = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0.logicalSize.w;

    const int3 poBase = workOffset;
    const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv0OutputsWmma[2][4];
                                                                                                        // [LEFT/RIGHT][4] batch of 4 in Y
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2][4];
    uint byteAddress = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias0.OffsetOf(0);
    accumulatorMatrix[0][0].Load(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias0.storage.buffer, byteAddress, 0, true);
    accumulatorMatrix[1][0].Copy(accumulatorMatrix[0][0]);

    [unroll]
    for (uint Y = 1; Y < 4; ++Y)
    {
        accumulatorMatrix[0][Y].Copy(accumulatorMatrix[0][0]);
        accumulatorMatrix[1][Y].Copy(accumulatorMatrix[0][0]);
    }


    [unroll]
    for (uint ky = 0; ky < Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0.logicalSize.x; ++kx)
        {
            [unroll]
            for (uint Y = 0; Y < 4; ++Y)
            {
                [unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {

                    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                    int inputOffset = input.OffsetOf(piBase + int3(kx + inputWaveOffset * 16, ky + Y, 0));
                    inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);

                    uint weightOffset = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0.OffsetOf(uint4(kx, ky, 0, 0));
                    AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                    weightMatrix.Load(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0.storage.buffer, weightOffset, Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights0.storageByteStrides.w, false);


                    accumulatorMatrix[inputWaveOffset][Y] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][Y]);
                }
            }
        }
    }

    [unroll]
    for (uint Y = 0; Y < 4; ++Y)
    {
        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            conv0OutputsWmma[matId][Y].CopySat(accumulatorMatrix[matId][Y]);
        }
    }

    // Second Convolution + Relu
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1OutputsWmma[32 / 16][2][4];

    // const uint weightOffset = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1.OffsetOf(int4(0, 0, 0, WaveGetLaneIndex())); // 32 lanes, 32 output channels
    // const uint4 weightsDwords = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1.storage.Load4(weightOffset);

    // uint weightIndex = 4 * WaveGetLaneIndex();
    // for (int i = 0; i < 4; ++i)
    //     weightsLDS[weightIndex++] = weightsDwords[i];

    [unroll]
    for (uint f = 0; f < Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1.logicalSize.w; f += 16)
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2][4];
        uint byteAddress = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias1.OffsetOf(f);
        accumulatorMatrix[0][0].Load(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias1.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1][0].Copy(accumulatorMatrix[0][0]);

        [unroll]
        for (uint Y = 1; Y < 4; ++Y)
        {
            accumulatorMatrix[0][Y].Copy(accumulatorMatrix[0][0]);
            accumulatorMatrix[1][Y].Copy(accumulatorMatrix[0][0]);
        }

        AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
        // AMD_GROUPSHARED_LOAD(weightMatrix, weightsLDS, f/16 * (4 * 32 / 2), 4, false);
        int weightOffset = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1.OffsetOf(int4(0, 0, 0, f));
        weightMatrix.Load(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1.storage.buffer, weightOffset, Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights1.storageByteStrides.w, false);

        [unroll]
        for (uint Y = 0; Y < 4; ++Y)
        {
            [unroll]
            for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv0OutputsWmma[inputWaveOffset][Y];
                accumulatorMatrix[inputWaveOffset][Y] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][Y]);

                // Relu()
                [unroll]
                for (uint i = 0; i < accumulatorMatrix[inputWaveOffset][Y].Length(); i++)
                {
                    float elem = accumulatorMatrix[inputWaveOffset][Y].Element(i);
                    accumulatorMatrix[inputWaveOffset][Y].SetElement(i, max(elem, 0.0f));
                }
            }

            [unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                conv1OutputsWmma[matId][f/16][Y].CopySat(accumulatorMatrix[matId][Y]);
            }
        }
    }

    // Third Convolution
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> outputMats[2][4];
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> residualMats[2][4];
    [unroll]
    for (uint Y = 0; Y < 4; ++Y)
    {
        int loadOffset = input.OffsetOf(int3(piBase.xy + int2(1, 1 + Y), 0));
        residualMats[0][Y].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
        loadOffset = input.OffsetOf(int3(piBase.xy + int2(16 + 1, 1 + Y), 0));
        residualMats[1][Y].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
    }

    // Weights loading in LDS
    {
                                                                 // C                   // N
        // const uint weightOffset = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2.OffsetOf(int4(0, 0, (WaveGetLaneIndex()%2)*16, WaveGetLaneIndex() / 2));
        // const uint4 weightsDwords = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2.storage.Load4(weightOffset);

        // uint weightIndex = 4 * WaveGetLaneIndex();
        // for (int i = 0; i < 4; ++i)
        //     weightsLDS[weightIndex++] = weightsDwords[i];

        const uint f = 0;
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2][4];
        uint byteAddress = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias2.OffsetOf(f);
        accumulatorMatrix[0][0].Load(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_bias2.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1][0].Copy(accumulatorMatrix[0][0]);

        [unroll]
        for (uint Y = 1; Y < 4; ++Y)
        {
            accumulatorMatrix[0][Y].Copy(accumulatorMatrix[0][0]);
            accumulatorMatrix[1][Y].Copy(accumulatorMatrix[0][0]);
        }

        [unroll]
        for (uint Y = 0; Y < 4; ++Y)
        {
            [unroll]
            for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
            {
                [unroll]
                for (uint c = 0; c < Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2.logicalSize.z; c += 16)
                {
                    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv1OutputsWmma[inputWaveOffset][c / 16][Y];

                    AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
                    //AMD_GROUPSHARED_LOAD(weightMatrix, weightsLDS, c/16 * 4, 8, false);
                    int weightOffset = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2.OffsetOf(int4(0, 0, c, f));
                    weightMatrix.Load(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2.storage.buffer, weightOffset, Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_weights2.storageByteStrides.w, false);

                    accumulatorMatrix[inputWaveOffset][Y] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][Y]);
                }
            }

            [unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> residualF32;
                residualF32.Copy(residualMats[matId][Y]);

                [unroll]
                for (uint i = 0; i < accumulatorMatrix[matId][Y].Length(); i++)
                {
                    float accumulatorValue = float(accumulatorMatrix[matId][Y].Element(i));
                    float residual = float(residualF32.Element(i));
                    accumulatorValue += residual;

                    accumulatorMatrix[matId][Y].SetElement(i, accumulatorValue);
                }

                outputMats[matId][Y].CopySat(accumulatorMatrix[matId][Y]);
            }
        }
    }

    [unroll]
    for (uint Y = 0; Y < 4; ++Y)
    {
        uint storeOffset = Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output.OffsetOf(poBase + int3(0,Y,0));
        outputMats[0][Y].Store(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output.storage.buffer, storeOffset, Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output.storageByteStrides.x, true);
        storeOffset += Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output.storageByteStrides.x * 16;
        outputMats[1][Y].Store(Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output.storage.buffer, storeOffset, Conv2D_DW_Conv2D_PW_Relu_Conv2D_Add_output.storageByteStrides.x, true);
    }

    // Store in LDS
    [unroll]
    for (uint Y = 0; Y < 4; ++Y)
    {
        AMD_GROUPSHARED_STORE(outputMats[0][Y], skipOutputLDS, Y*(16*4*2) + 0,    4, true);
        AMD_GROUPSHARED_STORE(outputMats[1][Y], skipOutputLDS, Y*(16*4*2) + 16*4, 4, true);
    }

    // Downscale
    {
                                                                                            //[Z-first/Z-second] [TOP-BOTTOM]
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16>accumulatorMatrix[2][2];
        const uint biasOffset = FusedConv2D_k2s2b_bias.OffsetOf(0);
        accumulatorMatrix[0][0].Load(FusedConv2D_k2s2b_bias.storage.buffer, biasOffset, 0, true);
        accumulatorMatrix[0][1].Copy(accumulatorMatrix[0][0]);

        const uint biasOffset2 = FusedConv2D_k2s2b_bias.OffsetOf(16);
        accumulatorMatrix[1][0].Load(FusedConv2D_k2s2b_bias.storage.buffer, biasOffset2, 0, true);
        accumulatorMatrix[1][1].Copy(accumulatorMatrix[1][0]);

        [unroll]
        for (uint Y = 0; Y < 2; ++Y)
        {
            [unroll]
            for (uint ky = 0; ky < Conv2D_k2s2b_weights.logicalSize.y; ++ky)
            {
                [unroll]
                for (uint kx = 0; kx < Conv2D_k2s2b_weights.logicalSize.x; ++kx)
                {
                    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix;
                    AMD_GROUPSHARED_LOAD(inputMatrix, skipOutputLDS, Y*(16*4*2*2)+ ky*(16*4*2) + kx*4, 8, true);

                    [unroll]
                    for (uint f = 0; f < Conv2D_k2s2b_weights.logicalSize.w; f += 16)
                    {
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                        const uint weightOffset = Conv2D_k2s2b_weights.OffsetOf(uint4(kx, ky, 0, f));
                        weightMatrix.Load(Conv2D_k2s2b_weights.storage.buffer, weightOffset, Conv2D_k2s2b_weights.storageByteStrides.w, false);

                        accumulatorMatrix[f/16][Y] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[f/16][Y]);
                    }
                }
            }
        }

        // Quantize
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> outputMats[2][2];
        [unroll]
        for (uint Y = 0; Y < 2; ++Y)
        {
            [unroll]
            for (uint f = 0; f < Conv2D_k2s2b_weights.logicalSize.w; f += 16)
            {
                outputMats[f/16][Y].CopySat(accumulatorMatrix[f/16][Y]);
            }

        }


        // Store downscale
        [unroll]
        for (uint Y = 0; Y < 2; ++Y)
        {
            uint storeOffset = Conv2D_k2s2b_output.OffsetOf(poBase / int3(2,2,1) + int3(0,Y,0));
            outputMats[0][Y].Store(Conv2D_k2s2b_output.storage.buffer, storeOffset, Conv2D_k2s2b_output.storageByteStrides.x, true);

            storeOffset = Conv2D_k2s2b_output.OffsetOf(poBase / int3(2,2,1) + int3(0,Y,16));
            outputMats[1][Y].Store(Conv2D_k2s2b_output.storage.buffer, storeOffset, Conv2D_k2s2b_output.storageByteStrides.x, true);
        }
    }
}

#else // #if WMMA_ENABLED
#error To use FP8 data type you need to provide WMMA_ENABLED=1 in hlsl_defines. There is no FP8 without WMMA.
#endif // #if WMMA_ENABLED
