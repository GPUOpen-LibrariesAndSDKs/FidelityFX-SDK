// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"

#if WMMA_ENABLED
#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"
#endif

template<typename SI, typename SW, typename SB, typename SO>
void Conv_16_32(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    int accumulator[32];
    [unroll]
    for (uint f = 0 ; f != 32; f+=4)
    {
        const int4 b = Load4i32(bias, f);
        accumulator[f] = b.x;
        accumulator[f+1] = b.y;
        accumulator[f+2] = b.z;
        accumulator[f+3] = b.w;
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {

            int8_t4_packed vs[4];
            const int3 inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 0)));
            const uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = inputDwords.x;
            vs[1] = inputDwords.y;
            vs[2] = inputDwords.z;
            vs[3] = inputDwords.w;

            [unroll]
            for (uint f = 0; f < numFeatures; ++f )
            {
                const uint4 weightsOffeset = weights.OffsetOf(uint4(kx, ky, 0, f));
                const uint4 weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[0], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[1], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[2], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[3], accumulator[f]);
            }
        }

    // Store output
    uint storeIndex = output.OffsetOf(poBase);
    uint4 storeDwords;
    [unroll]
    for (uint f = 0; f < numFeatures / 2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round(acc * weights.quantizationScale * input.quantizationScale * (1.0 / quantFactor0));

        storeDwords[f/4] = pack_clamp_s8(result);
    }
    output.storage.Store4(storeIndex, storeDwords);

    [unroll]
    for (uint f = numFeatures / 2; f < numFeatures; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round(acc * weights.quantizationScale * input.quantizationScale * (1.0 / quantFactor1));

        storeDwords[(f-numFeatures/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,16));
    output.storage.Store4(storeIndex, storeDwords);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_16_32(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    int accumulator[32];
    [unroll]
    for (uint f = 0 ; f != 32; f+=4)
    {
        const int4 b = 0;
        accumulator[f] = b.x;
        accumulator[f+1] = b.y;
        accumulator[f+2] = b.z;
        accumulator[f+3] = b.w;
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {

            int8_t4_packed vs[4];
            const int3 inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 0)));
            const uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = inputDwords.x;
            vs[1] = inputDwords.y;
            vs[2] = inputDwords.z;
            vs[3] = inputDwords.w;

            [unroll]
            for (uint f = 0; f < numFeatures; ++f )
            {
                const uint4 weightsOffeset = weights.OffsetOf(uint4(kx, ky, 0, f));
                const uint4 weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[0], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[1], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[2], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[3], accumulator[f]);
            }
        }

    // Store output
    uint storeIndex = output.OffsetOf(poBase);
    uint4 storeDwords;
    [unroll]
    for (uint f = 0; f < numFeatures / 2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor0));

        storeDwords[f/4] = pack_clamp_s8(result);
    }
    output.storage.Store4(storeIndex, storeDwords);

    [unroll]
    for (uint f = numFeatures / 2; f < numFeatures; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-numFeatures/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,16));
    output.storage.Store4(storeIndex, storeDwords);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_16_48(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    int accumulator[48];
    [unroll]
    for (uint f = 0 ; f != 48; f+=4)
    {
        const int4 b = 0;
        accumulator[f] = b.x;
        accumulator[f+1] = b.y;
        accumulator[f+2] = b.z;
        accumulator[f+3] = b.w;
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {

            int8_t4_packed vs[4];
            const int3 inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 0)));
            const uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = inputDwords.x;
            vs[1] = inputDwords.y;
            vs[2] = inputDwords.z;
            vs[3] = inputDwords.w;

            [unroll]
            for (uint f = 0; f < numFeatures; ++f )
            {
                const uint4 weightsOffeset = weights.OffsetOf(uint4(kx, ky, 0, f));
                const uint4 weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[0], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[1], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[2], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[3], accumulator[f]);
            }
        }

    // Store output
    uint storeIndex = output.OffsetOf(poBase);
    uint4 storeDwords;
    [unroll]
    for (uint f = 0; f < 16; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor0));

        storeDwords[f/4] = pack_clamp_s8(result);
    }
    output.storage.Store4(storeIndex, storeDwords);

    [unroll]
    for (uint f = 16; f < 32; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-16)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,16));
    output.storage.Store4(storeIndex, storeDwords);

        [unroll]
    for (uint f = 32; f < 48; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-32)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32));
    output.storage.Store4(storeIndex, storeDwords);
}


template<typename SI, typename SW, typename SB, typename SO>
void Conv_8_16_k2s2b(
    const float quantFactor0,
    const float quantFactor1,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 po,
    const uint numFeatures)
{
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
    int16_t4 quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.xy)) * (1.0/quantFactor0)));
    storageBytes.x = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[4], accumulators[5], accumulators[6], accumulators[7]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords.zw)) * (1.0/quantFactor0)));
    storageBytes.y = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[8], accumulators[9], accumulators[10], accumulators[11]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.xy)) * (1.0/quantFactor1)));
    storageBytes.z = pack_clamp_s8(quantized_vs);

    vs = float4(accumulators[12], accumulators[13], accumulators[14], accumulators[15]);
    quantized_vs = int16_t4(round((vs + Unpack4h(biasDwords2.zw)) * (1.0/quantFactor1)));
    storageBytes.w = pack_clamp_s8(quantized_vs);

    const uint byteAddress = output.OffsetOf(po);
    output.storage.Store4(byteAddress, storageBytes);
}



template<typename SI, typename SW, typename SB, typename SO>
void Conv_32_64(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    int accumulator[64];
    [unroll]
    for (uint f = 0 ; f < 64; f+=4)
    {
        const int4 b = Load4i32(bias, f);
        accumulator[f] = b.x;
        accumulator[f+1] = b.y;
        accumulator[f+2] = b.z;
        accumulator[f+3] = b.w;
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            int8_t4_packed vs[8];
            int3 inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 0)));
            uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = inputDwords.x;
            vs[1] = inputDwords.y;
            vs[2] = inputDwords.z;
            vs[3] = inputDwords.w;

            inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 16)));
            inputDwords = input.storage.Load4(inputOffset);
            vs[4] = inputDwords.x;
            vs[5] = inputDwords.y;
            vs[6] = inputDwords.z;
            vs[7] = inputDwords.w;

            [unroll]
            for (uint f = 0; f < numFeatures; ++f)
            {
                uint4 weightsOffeset = weights.OffsetOf(uint4(kx, ky, 0, f));
                uint4 weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[0], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[1], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[2], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[3], accumulator[f]);

                weightsOffeset = weights.OffsetOf(uint4(kx, ky, 16, f));
                weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[4], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[5], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[6], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[7], accumulator[f]);
            }
        }

    // Store first half output
    uint4 storeDwords;
    [unroll]
    for (uint f = 0; f < numFeatures/2/2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round(acc * weights.quantizationScale * input.quantizationScale * (1.0 / quantFactor0));

        storeDwords[f/4] = pack_clamp_s8(result);
    }
    uint storeIndex = output.OffsetOf(poBase);
    output.storage.Store4(storeIndex, storeDwords);
    [unroll]
    for (uint f = numFeatures/2/2; f < numFeatures/2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round(acc * weights.quantizationScale * input.quantizationScale * (1.0 / quantFactor0));

        storeDwords[(f-numFeatures/2/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,16));
    output.storage.Store4(storeIndex, storeDwords);

    // Store second half output
    [unroll]
    for (uint f = numFeatures/2; f < numFeatures-16; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round(acc * weights.quantizationScale * input.quantizationScale * (1.0 / quantFactor1));

        storeDwords[(f-numFeatures/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32));
    output.storage.Store4(storeIndex, storeDwords);

    [unroll]
    for (uint f = numFeatures-16; f < numFeatures; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round(acc * weights.quantizationScale * input.quantizationScale * (1.0 / quantFactor1));

        storeDwords[(f-(numFeatures-16))/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32+16));
    output.storage.Store4(storeIndex, storeDwords);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_32_64(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    int accumulator[64];
    [unroll]
    for (uint f = 0 ; f < 64; f+=4)
    {
        const int4 b = 0;
        accumulator[f] = b.x;
        accumulator[f+1] = b.y;
        accumulator[f+2] = b.z;
        accumulator[f+3] = b.w;
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            int8_t4_packed vs[8];
            int3 inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 0)));
            uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = inputDwords.x;
            vs[1] = inputDwords.y;
            vs[2] = inputDwords.z;
            vs[3] = inputDwords.w;

            inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 16)));
            inputDwords = input.storage.Load4(inputOffset);
            vs[4] = inputDwords.x;
            vs[5] = inputDwords.y;
            vs[6] = inputDwords.z;
            vs[7] = inputDwords.w;

            [unroll]
            for (uint f = 0; f < numFeatures; ++f)
            {
                uint4 weightsOffeset = weights.OffsetOf(uint4(kx, ky, 0, f));
                uint4 weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[0], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[1], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[2], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[3], accumulator[f]);

                weightsOffeset = weights.OffsetOf(uint4(kx, ky, 16, f));
                weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[4], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[5], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[6], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[7], accumulator[f]);
            }
        }

    // Store first half output
    uint4 storeDwords;
    [unroll]
    for (uint f = 0; f < numFeatures/2/2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor0));

        storeDwords[f/4] = pack_clamp_s8(result);
    }
    uint storeIndex = output.OffsetOf(poBase);
    output.storage.Store4(storeIndex, storeDwords);
    [unroll]
    for (uint f = numFeatures/2/2; f < numFeatures/2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor0));

        storeDwords[(f-numFeatures/2/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,16));
    output.storage.Store4(storeIndex, storeDwords);

    // Store second half output
    [unroll]
    for (uint f = numFeatures/2; f < numFeatures-16; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-numFeatures/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32));
    output.storage.Store4(storeIndex, storeDwords);

    [unroll]
    for (uint f = numFeatures-16; f < numFeatures; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-(numFeatures-16))/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32+16));
    output.storage.Store4(storeIndex, storeDwords);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv_48_64(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    int accumulator[64];
    [unroll]
    for (uint f = 0 ; f < 64; f+=4)
    {
        const int4 b = 0;
        accumulator[f] = b.x;
        accumulator[f+1] = b.y;
        accumulator[f+2] = b.z;
        accumulator[f+3] = b.w;
    }

    [unroll]
    for (uint ky = 0; ky < weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights.logicalSize.x; ++kx)
        {
            int8_t4_packed vs[12];
            int3 inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 0)));
            uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = inputDwords.x;
            vs[1] = inputDwords.y;
            vs[2] = inputDwords.z;
            vs[3] = inputDwords.w;

            inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 16)));
            inputDwords = input.storage.Load4(inputOffset);
            vs[4] = inputDwords.x;
            vs[5] = inputDwords.y;
            vs[6] = inputDwords.z;
            vs[7] = inputDwords.w;

            inputOffset = input.OffsetOf(int3(piBase + int3(kx, ky, 32)));
            inputDwords = input.storage.Load4(inputOffset);
            vs[8] = inputDwords.x;
            vs[9] = inputDwords.y;
            vs[10] = inputDwords.z;
            vs[11] = inputDwords.w;
            [unroll]
            for (uint f = 0; f < numFeatures; ++f)
            {
                uint4 weightsOffeset = weights.OffsetOf(uint4(kx, ky, 0, f));
                uint4 weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[0], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[1], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[2], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[3], accumulator[f]);

                weightsOffeset = weights.OffsetOf(uint4(kx, ky, 16, f));
                weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[4], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[5], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[6], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[7], accumulator[f]);

                weightsOffeset = weights.OffsetOf(uint4(kx, ky, 32, f));
                weightsDwords = weights.storage.Load4(weightsOffeset);

                accumulator[f] = dot4add_i8packed(weightsDwords.x, vs[8], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.y, vs[9], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.z, vs[10], accumulator[f]);
                accumulator[f] = dot4add_i8packed(weightsDwords.w, vs[11], accumulator[f]);
            }
        }

    // Store first half output
    uint4 storeDwords;
    [unroll]
    for (uint f = 0; f < numFeatures/2/2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor0));

        storeDwords[f/4] = pack_clamp_s8(result);
    }
    uint storeIndex = output.OffsetOf(poBase);
    output.storage.Store4(storeIndex, storeDwords);
    [unroll]
    for (uint f = numFeatures/2/2; f < numFeatures/2; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor0));

        storeDwords[(f-numFeatures/2/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,16));
    output.storage.Store4(storeIndex, storeDwords);

    // Store second half output
    [unroll]
    for (uint f = numFeatures/2; f < numFeatures-16; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-numFeatures/2)/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32));
    output.storage.Store4(storeIndex, storeDwords);

    [unroll]
    for (uint f = numFeatures-16; f < numFeatures; f+=4)
    {
        int4 acc = int4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int16_t4 result = round((acc * weights.quantizationScale * input.quantizationScale + Load4hA(bias, f)) * (1.0 / quantFactor1));

        storeDwords[(f-(numFeatures-16))/4] = pack_clamp_s8(result);
    }
    storeIndex = output.OffsetOf(poBase + int3(0,0,32+16));
    output.storage.Store4(storeIndex, storeDwords);
}


template<typename SI, typename SW, typename SO, typename BIAS>
void FusedConv2D_k2s2b_QuantizedOutput(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const BIAS bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;

    if (numFeatures == 48 && input.logicalSize.z == 16)
    {
        // Unpadded Case
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(0, 0, 0);

                if (IsAligned(numFeatures, 4))
                    Conv_16_48(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);
            }
    }

    if (numFeatures == 32 && input.logicalSize.z == 16)
    {
        // Unpadded Case
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(0, 0, 0);

                if (IsAligned(numFeatures, 4))
                    Conv_16_32(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);
            }
    }
    else if (numFeatures == 64 && input.logicalSize.z == 32)
    {
        // Unpadded Case
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(0, 0, 0);

                if (IsAligned(numFeatures, 4))
                    Conv_32_64(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);
            }
    }
    else if (numFeatures == 64 && input.logicalSize.z == 48)
    {
        // Unpadded Case
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(0, 0, 0);

                if (IsAligned(numFeatures, 4))
                    Conv_48_64(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);
            }
    }
}


template<typename SI, typename SW, typename SO, typename BIAS>
void FusedConv2D_k2s2b_QuantizedOutput(// fp16 weights + input but quantized output
    const float quantFactor0,
    const float quantFactor1,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const BIAS bias,
    const Tensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;
    {
        // Unpadded Case
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(0, 0, 0);

                if (IsAligned(numFeatures, 4))
                    Conv_8_16_k2s2b(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);
            }
    }
}

#if WMMA_ENABLED
template<typename SI, typename SW, typename SB, typename SO>
void Conv_16_32(
    const float quantFactor0,
    const float quantFactor1,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWNC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    const float quantFactors[2] = { 1.f / quantFactor0, 1.f / quantFactor1 };

    // Convolution
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > convOutputsWmma[2][32 / 16];

    // BIAS
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2][32 / 16];
    // bias - 0-15
    const uint biasOffset = bias.OffsetOf(0);
    accumulatorMatrix[0][0].Load(bias.storage.buffer, biasOffset, 0, true);
    accumulatorMatrix[1][0].Copy(accumulatorMatrix[0][0]);

    // bias - 16-31
    const uint biasOffset2 = bias.OffsetOf(16);
    accumulatorMatrix[0][1].Load(bias.storage.buffer, biasOffset2, 0, true);
    accumulatorMatrix[1][1].Copy(accumulatorMatrix[0][1]);


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
                    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix;
                    inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x * 2, true);

                    uint weightOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                    AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
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
            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId][f / 16].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId][f / 16].Element(i));
                quantizedElement *= input.quantizationScale * weights.quantizationScale;
                quantizedElement *= (quantFactors[f / 16]);
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);
                accumulatorMatrix[matId][f / 16].SetElement(i, int(quantizedElement));
            }

            convOutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId][f / 16]);
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
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_HWNC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3i8_NHWC<SO> output,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    const float quantFactors[2] = { 1.0f / quantFactor0, 1.0f / quantFactor1 };

    // Convolution
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > convOutputsWmma[2][64 / 16];

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, 16, 16 > accumulatorMatrix[2][64 / 16];
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
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > inputMatrix;
                        inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x * 2, true);

                        uint weightOffset = weights.OffsetOf(uint4(kx, ky, c, f));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, 16, 16 > weightMatrix;
                        weightMatrix.Load(weights.storage.buffer, weightOffset, weights.storageByteStrides.w, false);

                        accumulatorMatrix[inputWaveOffset][f / 16] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset][f / 16]);
                    }
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
            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId][f / 16].Length(); i++)
            {
                float quantizedElement = float(accumulatorMatrix[matId][f / 16].Element(i));
                quantizedElement *= input.quantizationScale * weights.quantizationScale;
                quantizedElement *= (quantFactors[f / 32]);
                quantizedElement = round(quantizedElement);
                quantizedElement = clamp(quantizedElement, -128.0f, 127.0f);
                accumulatorMatrix[matId][f / 16].SetElement(i, int(quantizedElement));
            }

            convOutputsWmma[matId][f / 16].Copy(accumulatorMatrix[matId][f / 16]);
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
    const QuantizedTensor3i8_NHWC< SI> input,
    const QuantizedTensor4i8_HWNC< SW> weights,
    const BIAS bias,
    const Tensor3i8_NHWC< SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(2, 2);
    const uint numFeatures = weights.logicalSize.w;

    if (numFeatures == 32 && input.logicalSize.z == 16)
    {
#ifdef WMMA_DOWNCALE_1
        workOffset = computeShaderParams.groupID * int3(32, 1, 1);
#endif
        // Unpadded Case
        [unroll]
        for (uint y = 0; y < perThreadWork.y; y++)
            [unroll]
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
#ifndef WMMA_DOWNSCALE_1
                if (!ValidPosition(output, poBase))
                    continue;
#endif

                const int3 piBase = int3(poBase.xy * convStrides, 0) - int3(0, 0, 0);

#ifndef WMMA_DOWNSCALE_1
                if (IsAligned(numFeatures, 4))
#endif
                    Conv_16_32(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);

            }
    }
    else if (numFeatures == 64 && input.logicalSize.z == 32)
    {
#ifdef WMMA_DOWNSCALE_2
        workOffset = computeShaderParams.groupID * int3(32, 1, 1);
#endif
        // Unpadded Case
        [unroll]
        for (uint y = 0; y < perThreadWork.y; y++)
            [unroll]
            for (uint x = 0; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
#ifndef WMMA_DOWNSCALE_2
                if (!ValidPosition(output, poBase))
                    continue;
#endif

                const int3 piBase = int3(poBase.xy * convStrides, 0) - int3(0, 0, 0);

#ifndef WMMA_DOWNSCALE_2
                if (IsAligned(numFeatures, 4))
#endif
                    Conv_32_64(quantFactor0, quantFactor1, input, weights, bias, output, piBase, poBase, numFeatures);
            }
    }
}
#endif // #if WMMA_ENABLED
