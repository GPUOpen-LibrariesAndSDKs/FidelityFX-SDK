// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1b(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    // WARNING: Slow path!!!
    struct Algorithm {
        float inputScale;
        float weightsScale;
        float outputScale;

        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        Tensor1h<SB> bias;
        QuantizedTensor3i8_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p); }
        half4 LoadInput4(int3 p) { return half4(unpack_s8s32(Load4i8A(input, p)) * inputScale); }
        half4 LoadWeight4(int4 p) { return half4(unpack_s8s32(Load4i8A(weights, p)) * weightsScale); }

        float Dot(half4 ws, half4 vs, float acc)
        {
            acc = dot2add(ws.xy, vs.xy, acc);
            return dot2add(ws.zw, vs.zw, acc);
        }

        void StoreOutput4(int3 p, float4 vs)
        {
            const int4 quantized_vs = round(vs * outputScale);
            Store4i8A(output, p, pack_clamp_s8(quantized_vs));
        }
    };

    Algorithm algorithm = {
        input.quantizationScale,
        weights.quantizationScale,
        1.0 / output.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<float4, half4, half4>(algorithm,1, 1, 1, 1, 1, 1, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}

// Version with bias as quantized tensor 1D i32.
template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1b(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    // WARNING: Slow path!!!
    struct Algorithm {
        float inputScale;
        float weightsScale;
        float biasScale;
        float outputScale;

        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        QuantizedTensor1i32<SB> bias;
        QuantizedTensor3i8_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        float4 LoadBias4(int p) { return Load4i32(bias, p) * biasScale; }
        half4 LoadInput4(int3 p) { return half4(unpack_s8s32(Load4i8A(input, p)) * inputScale); }
        half4 LoadWeight4(int4 p) { return half4(unpack_s8s32(Load4i8A(weights, p)) * weightsScale); }

        float Dot(half4 ws, half4 vs, float acc)
        {
            acc = dot2add(ws.xy, vs.xy, acc);
            return dot2add(ws.zw, vs.zw, acc);
        }

        void StoreOutput4(int3 p, float4 vs)
        {
            const int4 quantized_vs = round(vs * outputScale);
            Store4i8A(output, p, pack_clamp_s8(quantized_vs));
        }
    };

    Algorithm algorithm = {
        input.quantizationScale,
        weights.quantizationScale,
        bias.quantizationScale,
        1.0 / output.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<float4, half4, half4>(algorithm, 1, 1, 1, 1, 1, 1,
        output.threadGroupSliceStart, output.threadGroupSliceSize, computeShaderParams);
}
