// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

#if WMMA_ENABLED

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"

template<uint numFeatures, uint numGroups, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FNB_CT2D_ADD(
    const float quantScale0,
    const float quantScale1,
    const float activationScaleFactor,
    const float fnbOutputScale,
    const float quantOutputScale0, // If the output tensor requires only one quantization factor for its output, then quantOutputScale0 == quantOutputScale1
    const float quantOutputScale1,
    const Tensor3f8_NHWC<SI> input,
    const QuantizedTensor3f8_NHWC<SI> inputAdd,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC<SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC<SW2> weights2,
    const Tensor1f<SB2> bias2,
    const QuantizedTensor4f8_HWCN< SW3> ct2d_weights,
    const Tensor1f<SB3> ct2d_bias,
    const Tensor3f8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    _Static_assert(numFeatures == 64, "NumFeatures must be 64");
    _Static_assert(numGroups == 2, "NumGroups must be 2");

    const int3 threadWorkOffset = computeShaderParams.dispatchThreadID /** perThreadWork*/; //Dont multiply perThreadwork AND stride
    const int3 workOffset = computeShaderParams.groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poThreadBase = stride * threadWorkOffset;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv0OutputsWmma[2][64/16];

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2][64/16];
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
        accumulatorMatrix[0].Load(bias1.storage.buffer, bias1.OffsetOf(f), 0, true);
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
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv2OutputsWmma[2][64/16];

    [unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias2.storage.buffer, bias2.OffsetOf(f), 0, true);
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
                const uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 0) + int3(matId * 16, 0, f));
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

                conv2OutputsWmma[matId][f/16].CopySat(accumulatorMatrix[matId]);
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

                conv2OutputsWmma[matId][f/16].CopySat(accumulatorMatrix[matId]);
            }
        }
    }

    //ResetPadding(output, poThreadBase, true, true, int2(stride, stride));

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
                    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> convOutputsWmma;

                    // NOTE: X = 2, Y = 2, Half = 2
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix;
                    accumulatorMatrix.Load(ct2d_bias.storage.buffer, ct2d_bias.OffsetOf(weightWOffset * 16), 0, true);

                    [unroll]
                    for (uint inputZOffset = 0; inputZOffset < (ct2d_weights.logicalSize.w / 16); ++inputZOffset)
                    {
                        AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv2OutputsWmma[inputWaveOffset][inputZOffset];

                        uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, weightWOffset * 16, inputZOffset * 16));
                        AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
                        weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                        accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);
                    }

                    // NOTE: Get skip connection
                    int skipOffset = inputAdd.OffsetOf(poBase + uint3(kx + inputWaveOffset * stride * 16, ky, weightWOffset * 16));
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> skipMatrix;
                    skipMatrix.Load(inputAdd.storage.buffer, skipOffset, stride * inputAdd.storageByteStrides.x, true);
                    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> skipMatrixF32;
                    skipMatrixF32.Copy(skipMatrix);

                    // NOTE: Quantize
                    [unroll]
                    for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                    {
                        float matElement = (float)accumulatorMatrix.Element(i);
                        float skipValue = (float)skipMatrixF32.Element(i);
                        matElement += skipValue;

                        accumulatorMatrix.SetElement(i, matElement);
                    }

                    convOutputsWmma.CopySat(accumulatorMatrix);

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
    const Tensor3f8_NHWC<SI> input,
    const QuantizedTensor3f8_NHWC<SI> inputAdd,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC< SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC< SW2> weights2,
    const Tensor1f<SB2> bias2,
    const QuantizedTensor4f8_HWCN< SW3> ct2d_weights,
    const Tensor1f<SB3> ct2d_bias,
    const QuantizedTensor3f8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    _Static_assert(numFeatures == 32, "NumFeatures must be 32");
    _Static_assert(numGroups == 1, "NumGroups must be 1");

    const int3 threadWorkOffset = computeShaderParams.dispatchThreadID /** perThreadWork*/; //Dont multiply perThreadwork AND stride
    const int3 workOffset = computeShaderParams.groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poThreadBase = stride * threadWorkOffset;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv0OutputsWmma[2][32 / 16];

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
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

    for (uint f = 0; f < weights0.logicalSize.w; f += 16)
    {
        for (uint matId = 0; matId < 2; ++matId)
        {
            conv0OutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId]);
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
    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv1OutputsWmma[32 / 16][64 / 16];

    //[unroll]
    for (uint f = 0; f < weights1.logicalSize.w; f += 16)
    {
        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias1.storage.buffer, bias1.OffsetOf(f), 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        //[unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
        {
            //[unroll]
            for (uint c = 0; c < weights1.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix = conv0OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights1.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
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

        //[unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            conv1OutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId]);
        }
    }

    // Third Convolution
    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv2OutputsWmma[2][32 / 16];

    //[unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
        accumulatorMatrix[0].Load(bias2.storage.buffer, bias2.OffsetOf(f), 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        //[unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
        {
            //[unroll]
            for (uint c = 0; c < weights2.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix = conv1OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights2.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                weightMatrix.Load(weights2.storage.buffer, weightOffset, weights2.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        if ((f / 16) == 0)
        {
            //[unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputsAdd;
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > inputsAddF32;
                const uint inputOffset = input.OffsetOf(piBase + int3(1, 1, 0) + int3(matId * 16, 0, f));
                inputsAdd.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);
                inputsAddF32.Copy(inputsAdd);

                for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                {
                    float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                    float residuals = float(inputsAddF32.Element(i));
                    quantizedElement += residuals;

                    accumulatorMatrix[matId].SetElement(i, quantizedElement);
                }

                conv2OutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId]);
            }
        }
        else
        {
            //[unroll]
            for (uint matId = 0; matId < 2; ++matId)
            {
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > inputsAddF32;
                inputsAddF32.Copy(conv0OutputsWmma[matId][f / 16]);

                for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
                {
                    float quantizedElement = float(accumulatorMatrix[matId].Element(i));
                    float residuals = float(inputsAddF32.Element(i));
                    quantizedElement += residuals;

                    accumulatorMatrix[matId].SetElement(i, quantizedElement);
                }

                conv2OutputsWmma[matId][f / 16].CopySat(accumulatorMatrix[matId]);
            }
        }
    }

    //ResetPadding(output, poThreadBase, true, true, int2(stride, stride));

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
                    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > convOutputsWmma;

                    // NOTE: X = 2, Y = 2, Half = 2
                    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix;
                    accumulatorMatrix.Load(ct2d_bias.storage.buffer, ct2d_bias.OffsetOf(weightWOffset * 16), 0, true);

                    //[unroll]
                    for (uint inputZOffset = 0; inputZOffset < (ct2d_weights.logicalSize.w / 16); ++inputZOffset)
                    {
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix = conv2OutputsWmma[inputWaveOffset][inputZOffset];

                        uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, weightWOffset * 16, inputZOffset * 16));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                        weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                        accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);
                    }

                    // NOTE: Get skip connection
                    int skipOffset = inputAdd.OffsetOf(poBase + uint3(kx + inputWaveOffset * stride * 16, ky, weightWOffset * 16));
                    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > skipMatrix;
                    skipMatrix.Load(inputAdd.storage.buffer, skipOffset, stride * inputAdd.storageByteStrides.x, true);
                    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > skipMatrixF32;
                    skipMatrixF32.Copy(skipMatrix);

                    // NOTE: Quantize
                    for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                    {
                        float matElement = (float) accumulatorMatrix.Element(i);
                        float skipValue = (float) skipMatrixF32.Element(i);
                        matElement += skipValue;

                        accumulatorMatrix.SetElement(i, matElement);
                    }

                    convOutputsWmma.CopySat(accumulatorMatrix);

                    // NOTE: Store
                    uint storeOffset = output.OffsetOf(uint3(poBase) + uint3(kx + stride * 16 * inputWaveOffset, ky, 16 * weightWOffset));
                    convOutputsWmma.Store(output.storage.buffer, storeOffset, stride * output.storageByteStrides.x, true);
                }
            }
        }
    }
}

#else // #if WMMA_ENABLED
#error To use FP8 data type you need to provide WMMA_ENABLED=1 in hlsl_defines. There is no FP8 without WMMA.
#endif // #if WMMA_ENABLED
