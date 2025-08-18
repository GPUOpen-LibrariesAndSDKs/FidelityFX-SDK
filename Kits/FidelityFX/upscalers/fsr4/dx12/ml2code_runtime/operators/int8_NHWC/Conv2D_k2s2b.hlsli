// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void Conv_8_16_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 po)
{
    const float rcpScale = 1.0f / output.quantizationScale;

    half4 inputValues[2*4];
    uint nInput = 0;

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, 0));
            const uint4 inputDwords = input.storage.Load4(inputOffset);

            inputValues[nInput++] = Unpack4h(inputDwords.xy);
            inputValues[nInput++] = Unpack4h(inputDwords.zw);
        }
    }

    const uint numFeatures = 16;

    float accumulators[16];
    [unroll]
    for (uint f = 0 ; f != numFeatures; f++)
    {
        accumulators[f] = 0;
    }

    [unroll]
    for (uint f = 0 ; f < numFeatures; f++)
    {
        nInput = 0;
        [unroll]
            for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
            {
                [unroll]
                for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
                {
                    const uint weightsOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                    const uint4 weightDwords = weights.storage.Load4(weightsOffset);

                    half4 ws = Unpack4h(weightDwords.xy);
                    accumulators[f] = dot2add(ws.xy, inputValues[nInput].xy, accumulators[f]);
                    accumulators[f] = dot2add(ws.zw, inputValues[nInput++].zw, accumulators[f]);

                    ws = Unpack4h(weightDwords.zw);
                    accumulators[f] = dot2add(ws.xy, inputValues[nInput].xy, accumulators[f]);
                    accumulators[f] = dot2add(ws.zw, inputValues[nInput++].zw, accumulators[f]);
                }
            }
    }

    const uint4 biasDwords = bias.storage.Load4(bias.OffsetOf(0));
    const uint4 biasDwords2 = bias.storage.Load4(bias.OffsetOf(8));

    uint4 storageBytes;

    float4 vs = float4(accumulators[0], accumulators[1], accumulators[2], accumulators[3]);
    int16_t4 quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.xy)) * rcpScale));
    storageBytes.x = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[4], accumulators[5], accumulators[6], accumulators[7]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.zw)) * rcpScale));
    storageBytes.y = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[8], accumulators[9], accumulators[10], accumulators[11]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.xy)) * rcpScale));
    storageBytes.z = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[12], accumulators[13], accumulators[14], accumulators[15]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.zw)) * rcpScale));
    storageBytes.w = pack_clamp_s8(quantized_vs);

    const uint byteAddress = output.OffsetOf(po);
    output.storage.Store4(byteAddress, storageBytes);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_8_16_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_HWNC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 po)
{
    const float rcpScale = 1.0f / output.quantizationScale;

    half4 inputValues[2*4];
    uint nInput = 0;

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, 0));
            const uint4 inputDwords = input.storage.Load4(inputOffset);

            inputValues[nInput++] = Unpack4h(inputDwords.xy);
            inputValues[nInput++] = Unpack4h(inputDwords.zw);
        }
    }

    const uint numFeatures = 16;

    float accumulators[16];
    [unroll]
    for (uint f = 0 ; f != numFeatures; f++)
    {
        accumulators[f] = 0;
    }

    //Reset input index
    nInput = 0;
    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            [unroll]
            for (uint f = 0; f < numFeatures; f++)
            {
                const uint inputIndex = 2 * (kx + 2 * ky);
                const uint weightsOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                const uint4 weightDwords = weights.storage.Load4(weightsOffset);

                half4 ws = Unpack4h(weightDwords.xy);
                accumulators[f] = dot2add(ws.xy, inputValues[inputIndex].xy, accumulators[f]);
                accumulators[f] = dot2add(ws.zw, inputValues[inputIndex].zw, accumulators[f]);

                ws = Unpack4h(weightDwords.zw);
                accumulators[f] = dot2add(ws.xy, inputValues[inputIndex + 1].xy, accumulators[f]);
                accumulators[f] = dot2add(ws.zw, inputValues[inputIndex + 1].zw, accumulators[f]);
            }
        }
    }

    const uint4 biasDwords = bias.storage.Load4(bias.OffsetOf(0));
    const uint4 biasDwords2 = bias.storage.Load4(bias.OffsetOf(8));

    uint4 storageBytes;

    float4 vs = float4(accumulators[0], accumulators[1], accumulators[2], accumulators[3]);
    int16_t4 quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.xy)) * rcpScale));
    storageBytes.x = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[4], accumulators[5], accumulators[6], accumulators[7]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.zw)) * rcpScale));
    storageBytes.y = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[8], accumulators[9], accumulators[10], accumulators[11]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.xy)) * rcpScale));
    storageBytes.z = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[12], accumulators[13], accumulators[14], accumulators[15]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.zw)) * rcpScale));
    storageBytes.w = pack_clamp_s8(quantized_vs);

    const uint byteAddress = output.OffsetOf(po);
    output.storage.Store4(byteAddress, storageBytes);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_10_16_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    const float rcpScale = 1.0f / output.quantizationScale;

    half2 inputValues[5*4];
    uint nInput = 0;

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {

            const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, 0));
            const uint4 inputDwords = input.storage.Load4(inputOffset);

            half4 halfValues = Unpack4h(inputDwords.xy);
            inputValues[nInput++] = halfValues.xy;
            inputValues[nInput++] = halfValues.zw;

            halfValues = Unpack4h(inputDwords.zw);
            inputValues[nInput++] = halfValues.xy;
            inputValues[nInput++] = halfValues.zw;

            inputValues[nInput++] = Unpack2h(input.storage.Load1(input.OffsetOf(piBase + int3(kx, ky, 8))));
        }
    }

    float accumulators[16];
    [unroll]
    for (uint f = 0 ; f != 16; f++)
    {
        accumulators[f] = 0;
    }
    [unroll]
    for (uint f = 0 ; f < numFeatures; f++)
    {
        nInput = 0;
        [unroll]
            for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
            {
                [unroll]
                for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
                {
                    const uint weightsOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                    const uint4 weightDwords = weights.storage.Load4(weightsOffset);

                    half4 ws = Unpack4h(weightDwords.xy);
                    accumulators[f] = dot2add(ws.xy, inputValues[nInput++], accumulators[f]);
                    accumulators[f] = dot2add(ws.zw, inputValues[nInput++], accumulators[f]);

                    ws = Unpack4h(weightDwords.zw);
                    accumulators[f] = dot2add(ws.xy, inputValues[nInput++], accumulators[f]);
                    accumulators[f] = dot2add(ws.zw, inputValues[nInput++], accumulators[f]);

                    ws.xy = Unpack2h(weights.storage.Load1(weights.OffsetOf(uint4(kx, ky, 8, f))));
                    accumulators[f] = dot2add(ws.xy, inputValues[nInput++], accumulators[f]);
                }
            }
    }

    const uint4 biasDwords = bias.storage.Load4(bias.OffsetOf(0));
    const uint4 biasDwords2 = bias.storage.Load4(bias.OffsetOf(8));

    const int3 po = poBase;
    uint4 storageBytes;

    float4 vs = float4(accumulators[0], accumulators[1], accumulators[2], accumulators[3]);
    int16_t4 quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.xy)) * rcpScale));
    storageBytes.x = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[4], accumulators[5], accumulators[6], accumulators[7]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.zw)) * rcpScale));
    storageBytes.y = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[8], accumulators[9], accumulators[10], accumulators[11]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.xy)) * rcpScale));
    storageBytes.z = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[12], accumulators[13], accumulators[14], accumulators[15]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.zw)) * rcpScale));
    storageBytes.w = pack_clamp_s8(quantized_vs);

    const uint byteAddress = output.OffsetOf(po);
    output.storage.Store4(byteAddress, storageBytes);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;

    if (numFeatures == 16 && input.storageSize.z == 8)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0);

                Conv_8_16_k2s2b(input, weights, bias, output, piBase, poBase);
            }

        return;
    }
    else if (numFeatures == 16 && input.logicalSize.z == 10)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0);
                Conv_10_16_k2s2b(input, weights, bias, output, piBase, poBase, numFeatures);
            }

        return;
    }
    else
    {
        struct Algorithm {
            float rcpScale;
            uint4 kernelSize;
            Tensor3h_NHWC<SI> input;
            Tensor4h_NHWC<SW> weights;
            Tensor1h<SB> bias;
            QuantizedTensor3i8_NHWC<SO> output;

            bool ValidInput(int3 p) { return ValidPosition(input, p); }
            bool ValidOutput(int3 p) { return ValidPosition(output, p); }

            half4 LoadBias4(int p) { return 0; }
            half4 LoadWeight4(int4 p) { return Load4hA(weights, p); }
            half4 LoadInput4(int3 p) { return Load4hA(input, p); }

            float Dot(half4 x, half4 y, float acc)
            {
                acc = dot2add(x.xy, y.xy, acc);
                return dot2add(x.zw, y.zw, acc);
            }

            void StoreOutput4(int3 p, float4 vs)
            {
                const int16_t4 quantized_vs = int16_t4(round((vs + Load4hA(bias, p.z)) * rcpScale));
                Store4i8A(output, p, pack_clamp_s8(quantized_vs));
            }
        };

        Algorithm algorithm = {
            1.0f / output.quantizationScale,
            weights.logicalSize,
            input, weights, bias, output
        };

        TemplatedConv2D_3413_NHWC_44<float4, half4, half4>(algorithm, 0, 0, 0, 0, 2, 2, output.threadGroupSliceStart, output.threadGroupSliceSize,
            computeShaderParams);
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k2s2b(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_HWNC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;

    if (numFeatures == 16 && input.storageSize.z == 8)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0);

                Conv_8_16_k2s2b(input, weights, bias, output, piBase, poBase);
            }

        return;
    }
    //else if (numFeatures == 16 && input.logicalSize.z == 10)
    //{
    //    for (uint y = 0 ; y < perThreadWork.y; y++)
    //        for (uint x = 0 ; x < perThreadWork.x; x++)
    //        {
    //            const int3 poBase = workOffset + int3(x, y, 0);
    //            if (!ValidPosition(output, poBase))
    //                continue;

    //            const int3 piBase = int3(poBase.xy*convStrides, 0);
    //            Conv_10_16_k2s2b(input, weights, bias, output, piBase, poBase, numFeatures);
    //        }

    //    return;
    //}
    else
    {
        struct Algorithm {
            float rcpScale;
            uint4 kernelSize;
            Tensor3h_NHWC<SI> input;
            Tensor4h_HWNC <SW>weights;
            Tensor1h<SB> bias;
            QuantizedTensor3i8_NHWC<SO> output;

            bool ValidInput(int3 p) { return ValidPosition(input, p); }
            bool ValidOutput(int3 p) { return ValidPosition(output, p); }

            half4 LoadBias4(int p) { return 0; }
            half4 LoadWeight4(int4 p) { return Load4hA(weights, p); }
            half4 LoadInput4(int3 p) { return Load4hA(input, p); }

            float Dot(half4 x, half4 y, float acc)
            {
                acc = dot2add(x.xy, y.xy, acc);
                return dot2add(x.zw, y.zw, acc);
            }

            void StoreOutput4(int3 p, float4 vs)
            {
                const int16_t4 quantized_vs = int16_t4(round((vs + Load4hA(bias, p.z)) * rcpScale));
                Store4i8A(output, p, pack_clamp_s8(quantized_vs));
            }
        };

        Algorithm algorithm = {
            1.0f / output.quantizationScale,
            weights.logicalSize,
            input, weights, bias, output
        };

        TemplatedConv2D_3413_NHWC_44<float4, half4, half4>(algorithm, 0, 0, 0, 0, 2, 2, output.threadGroupSliceStart, output.threadGroupSliceSize,
            computeShaderParams);
    }
}