// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3f_NHWC<SI> input,
    const Tensor4f_NHWC<SW> weights,
    const Tensor1f<SB> bias,
    const QuantizedTensor3u8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        float rcpScale;
        uint4 kernelSize;
        Tensor3f_NHWC<SI> input;
        Tensor4f_NHWC<SW> weights;
        Tensor1f<SB> bias;
        QuantizedTensor3u8_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        float4 LoadBias4(int p) { return Load4f(bias, p); }
        float4 LoadWeight4(int4 p) { return Load4f(weights, p); }
        float4 LoadInput4(int3 p) { return Load4f(input, p); }

        float Dot(float4 x, float4 y, float acc) { return dot(x, y) + acc; }

        void StoreOutput4(int3 p, float4 vs)
        {
            const int16_t4 quantized_vs = int16_t4(round(vs * rcpScale));
            Store4u8A(output, p, pack_clamp_u8(quantized_vs));
        }
    };

    Algorithm algorithm = {
        1.0f / output.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<float4, float4, float4>(algorithm, beginY, beginX, endY, endX, strideY, strideX, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3u8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        float rcpScale;
        uint4 kernelSize;
        Tensor3h_NHWC<SI> input;
        Tensor4h_NHWC<SW> weights;
        Tensor1h<SB> bias;
        QuantizedTensor3u8_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p); }
        half4 LoadWeight4(int4 p) { return Load4hA(weights, p); }
        half4 LoadInput4(int3 p) { return Load4hA(input, p); }

        half Dot(half4 x, half4 y, half acc) { return dot(x, y) + acc; }

        void StoreOutput4(int3 p, half4 vs)
        {
            const int16_t4 quantized_vs = int16_t4(round(vs * rcpScale));
            Store4u8A(output, p, pack_clamp_u8(quantized_vs));
        }
    };

    Algorithm algorithm = {
        1.0f / output.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<half4, half4, half4>(algorithm, beginY, beginX, endY, endX, strideY, strideX, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}
