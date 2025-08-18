// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

#if WMMA_ENABLED

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"

template<typename SI, typename SW, typename SB, typename SO>
void Conv_16_32(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW> weights,
    const Tensor1f<SB> bias,
    const Tensor3f8_NHWC<SO> output,
    const int3 piBase,
    const int3 poThreadBase,
    const int3 poBase,
    const uint numFeatures)
{
    // Convolution
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > convOutputsWmma[2][32 / 16];
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2][32 / 16];
    // bias - 0-15
    const uint biasOffset = bias.OffsetOf(0);
    accumulatorMatrix[0][0].Load(bias.storage.buffer, biasOffset, 0, true);
    accumulatorMatrix[1][0].Copy(accumulatorMatrix[0][0]);

    // bias - 16-31
    const uint biasOffset2 = bias.OffsetOf(16);
    accumulatorMatrix[0][1].Load(bias.storage.buffer, biasOffset2, 0, true);
    accumulatorMatrix[1][1].Copy(accumulatorMatrix[0][1]);

    //ResetPadding(output, poThreadBase, true, true);

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            [unroll]
            for (uint f = 0; f < weights.logicalSize.w; f += 16)
            {
                [unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {
                    const uint inputOffset = input.OffsetOf(piBase + int3(kx + inputWaveOffset * 16 * 2, ky, 0));
                    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                    inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x * 2, true);

                    uint weightOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                    AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                    weightMatrix.Load(weights.storage.buffer, weightOffset, weights.storageByteStrides.w, false);

                    accumulatorMatrix[inputWaveOffset][f / 16] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][f / 16]);
                }
            }
        }
    }

    //TODO Merge to end of previous loop?
    [unroll]
    for (uint f = 0; f < weights.logicalSize.w; f += 16)
    {
        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            convOutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId][f / 16]);
        }

        uint storeOffset = output.OffsetOf(poBase + int3(0,0,f));
        convOutputsWmma[0][f / 16].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
        storeOffset += output.storageByteStrides.x * 16;
        convOutputsWmma[1][f / 16].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_32_64(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW> weights,
    const Tensor1f<SB> bias,
    const Tensor3f8_NHWC<SO> output,
    const int3 piBase,
    const int3 poThreadBase,
    const int3 poBase,
    const uint numFeatures)
{
    // Convolution
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > convOutputsWmma[2][64 / 16];

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2][64 / 16];
    [unroll]
    for (uint f = 0; f < weights.logicalSize.w; f += 16)
    {
        accumulatorMatrix[0][f / 16].Load(bias.storage.buffer, bias.OffsetOf(f), 0, true);
        accumulatorMatrix[1][f / 16].Copy(accumulatorMatrix[0][f / 16]);
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            [unroll]
            for (uint f = 0; f < weights.logicalSize.w; f += 16)
            {
                [unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {
                    [unroll]
                    for (uint c = 0; c < weights.logicalSize.z; c += 16)
                    {
                        const uint inputOffset = input.OffsetOf(piBase + int3(kx + inputWaveOffset * 16 * 2, ky, c));
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                        inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x * 2, true);

                        uint weightOffset = weights.OffsetOf(uint4(kx, ky, c, f));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                        weightMatrix.Load(weights.storage.buffer, weightOffset, weights.storageByteStrides.w, false);

                        accumulatorMatrix[inputWaveOffset][f / 16] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][f / 16]);
                    }
                }
            }
        }
    }

    //ResetPadding(output, poThreadBase, true, true);

    //TODO Merge to end of previous loop?
    [unroll]
    for (uint f = 0; f < weights.logicalSize.w; f += 16)
    {
        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            convOutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId][f / 16]);
        }

        uint storeOffset = output.OffsetOf(poBase + int3(0,0,f));
        convOutputsWmma[0][f / 16].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
        storeOffset += output.storageByteStrides.x * 16;
        convOutputsWmma[1][f / 16].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
    }
}



template<typename SI, typename SW, typename SO, typename BIAS>
void FusedConv2D_k2s2b_QuantizedOutput(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3f8_NHWC< SI> input,
    const QuantizedTensor4f8_HWNC< SW> weights,
    const BIAS bias,
    const Tensor3f8_NHWC< SO> output,
    const ComputeShaderParams computeShaderParams)
{
    int3 threadWorkOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID;
    int3 workOffset = output.threadGroupSliceStart;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;

    if (numFeatures == 32 && input.logicalSize.z == 16)
    {
        const int3 poThreadBase = threadWorkOffset;
        const int3 poBase = workOffset;

        const int3 piBase = int3(poBase.xy * convStrides, 0) - int3(0, 0, 0);
        Conv_16_32(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poThreadBase, poBase, numFeatures);
    }
    else if (numFeatures == 64 && input.logicalSize.z == 32)
    {
        const int3 poThreadBase = threadWorkOffset;
        const int3 poBase = workOffset;

        const int3 piBase = int3(poBase.xy * convStrides, 0) - int3(0, 0, 0);
        Conv_32_64(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poThreadBase, poBase, numFeatures);
    }
}

#else // #if WMMA_ENABLED
#error To use FP8 data type you need to provide WMMA_ENABLED=1 in hlsl_defines. There is no FP8 without WMMA.
#endif // #if WMMA_ENABLED
