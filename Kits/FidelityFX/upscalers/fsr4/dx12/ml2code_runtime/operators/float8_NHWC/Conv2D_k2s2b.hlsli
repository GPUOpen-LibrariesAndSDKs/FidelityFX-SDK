// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

#if WMMA_ENABLED

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"

groupshared uint inputLDS[(16 * 16)/2];
template<typename SI, typename WEIGHTS, typename SB, typename SO>
void Conv_8_16_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const WEIGHTS weights,
    const Tensor1f<SB> bias,
    const QuantizedTensor3f8_NHWC<SO> output,
    const int3 piBase,
    const int3 po,
    const uint3 groupID)
{
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix;
    const uint biasOffset = bias.OffsetOf(0);
    accumulatorMatrix.Load(bias.storage.buffer, biasOffset, 0, true);
    //accumulatorMatrix.Fill(0);


    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        const uint inputOffset = input.OffsetOf(piBase + int3(0, ky, 0));
        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > inputMatrix;
#if 1
        const uint waveOffset = WaveReadLaneFirst(inputOffset) + WaveGetLaneIndex()*4*4;
        const uint4 inputDwords = input.storage.Load4(waveOffset);
        //const uint4 inputDwords = input.storage.Load4(inputOffset);

        inputLDS[WaveGetLaneIndex()*4+0] = inputDwords.x;
        inputLDS[WaveGetLaneIndex()*4+1] = inputDwords.y;
        inputLDS[WaveGetLaneIndex()*4+2] = inputDwords.z;
        inputLDS[WaveGetLaneIndex()*4+3] = inputDwords.w;
        GroupMemoryBarrierWithGroupSync();

        AMD_GROUPSHARED_LOAD(inputMatrix, inputLDS, 0, 8, true);
		GroupMemoryBarrierWithGroupSync();
#else
        inputMatrix.Load(input.storage.buffer, inputOffset, 32, true);
#endif

        uint weightOffset = weights.threadGroupStorageByteOffset + 512 * ky;
        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > weightMatrix;
        weightMatrix.Load(weights.storage.buffer, weightOffset, 32, false);
        //weightMatrix.Fill(1.0f);

        accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);
        //break;
    }

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > convOutputsWmma;
    convOutputsWmma.CopySat(accumulatorMatrix);

    //ResetPadding(output, po, true, true);

    uint storeOffset = output.OffsetOf(po);
    convOutputsWmma.Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
}
// {
//     AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
//     const uint biasOffset = bias.OffsetOf(0);
//     accumulatorMatrix[0].Load(bias.storage.buffer, biasOffset, 0, true);
//     accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

//     [unroll]
//     for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
//     {
//         [unroll]
//         for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
//         {
//             const uint inputOffset = input.OffsetOf(piBase + int3(inputWaveOffset * 16 * 2, ky, 0));
//             AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > inputMatrix;
//             inputMatrix.Load(input.storage.buffer, inputOffset, 32, true);

//             uint weightOffset = weights.threadGroupStorageByteOffset + 512 * ky;
//             AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > weightMatrix;
//             weightMatrix.Load(weights.storage.buffer, weightOffset, 32, false);

//             accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
//         }
//     }

//     AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > convOutputsWmma[2];
//     [unroll]
//     for (uint matId = 0; matId < 2; ++matId)
//     {
//         convOutputsWmma[matId].CopySat(accumulatorMatrix[matId]);
//     }

//     uint storeOffset = output.OffsetOf(po);
//     convOutputsWmma[0].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
//     storeOffset += output.storageByteStrides.x * 16;
//     convOutputsWmma[1].Store(output.storage.buffer, storeOffset, output.storageByteStrides.x, true);
// }

// DOT2
// template<typename SI, typename SW, typename SB, typename SO>
// void Conv_8_16_k2s2b(
//     const Tensor3h_NHWC<SI> input,
//     const Tensor4h_NHWC<SW> weights,
//     const Tensor1f<SB> bias,
//     const QuantizedTensor3f8_NHWC<SO> output,
//     const int3 piBase,
//     const int3 po)
// {
//     const float rcpScale = 1.0f / output.quantizationScale;

//     half4 inputValues[2*4];
//     uint nInput = 0;

//     [unroll]
//     for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
//     {
//         [unroll]
//         for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
//         {
//             const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, 0));
//             const uint4 inputDwords = input.storage.Load4(inputOffset);

//             inputValues[nInput++] = Unpack4h(inputDwords.xy);
//             inputValues[nInput++] = Unpack4h(inputDwords.zw);
//         }
//     }

//     const uint numFeatures = 16;

//     float accumulators[16];
//     [unroll]
//     for (uint f = 0 ; f != numFeatures; f++)
//     {
//         accumulators[f] = 0;
//     }

//     //Reset input index
//     nInput = 0;
//     [unroll]
//     for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
//     {
//         [unroll]
//         for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
//         {
//             [unroll]
//             for (uint f = 0; f < numFeatures; f++)
//             {
//                 const uint inputIndex = 2 * (kx + 2 * ky);
//                 const uint weightsOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
//                 const uint4 weightDwords = weights.storage.Load4(weightsOffset);

//                 half4 ws = Unpack4h(weightDwords.xy);
//                 accumulators[f] = dot2add(ws.xy, inputValues[inputIndex].xy, accumulators[f]);
//                 accumulators[f] = dot2add(ws.zw, inputValues[inputIndex].zw, accumulators[f]);

//                 ws = Unpack4h(weightDwords.zw);
//                 accumulators[f] = dot2add(ws.xy, inputValues[inputIndex + 1].xy, accumulators[f]);
//                 accumulators[f] = dot2add(ws.zw, inputValues[inputIndex + 1].zw, accumulators[f]);
//             }
//         }
//     }

//     uint4 storageBytes;

//     float4 vs = float4(accumulators[0], accumulators[1], accumulators[2], accumulators[3]);
//     storageBytes.x = amd_pack_sat_fp8(float4(vs + Load4f(bias, 0)));

//     vs = float4(accumulators[4], accumulators[5], accumulators[6], accumulators[7]);
//     storageBytes.y = amd_pack_sat_fp8(float4(vs + Load4f(bias, 4)));


//     vs = float4(accumulators[8], accumulators[9], accumulators[10], accumulators[11]);
//     storageBytes.z = amd_pack_sat_fp8(float4(vs + Load4f(bias, 8)));

//     vs = float4(accumulators[12], accumulators[13], accumulators[14], accumulators[15]);
//     storageBytes.w = amd_pack_sat_fp8(float4(vs + Load4f(bias, 12)));

//     const uint byteAddress = output.OffsetOf(po);
//     output.storage.Store4(byteAddress, storageBytes);
// }

template<typename SI, typename WEIGHTS, typename SB, typename SO>
void Conv2D_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const WEIGHTS weights,
    const Tensor1f<SB> bias,
    const QuantizedTensor3f8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * int3(1,1,1);//perThreadWork;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;



    if (numFeatures == 16 && input.storageSize.z == 8)
    {

        const int3 poBase = workOffset;
        const int3 piBase = int3(poBase.xy*convStrides, 0);
        Conv_8_16_k2s2b(input, weights, bias, output, piBase, poBase, computeShaderParams.groupID);
    }
}

#else // #if WMMA_ENABLED
#error To use FP8 data type you need to provide WMMA_ENABLED=1 in hlsl_defines. There is no FP8 without WMMA.
#endif // #if WMMA_ENABLED
