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
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3f_NHWC<SI> input,
    const Tensor4f_NHWC<SW> weights,
    const Tensor1f<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        float outputScale;

        uint4 kernelSize;
        Tensor3f_NHWC<SI> input;
        Tensor4f_NHWC<SW> weights;
        Tensor1f<SB> bias;
        QuantizedTensor3i8_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        float4 LoadBias4(int p) { return Load4f(bias, p) ; }
        float4 LoadInput4(int3 p) { return Load4f(input, p); }
        float4 LoadWeight4(int4 p) { return Load4f(weights, p);  }
        float Dot(float4 ws, float4 vs, float acc) { return dot(ws, vs) + acc; }

        void StoreOutput4(int3 p, float4 vs)
        {
            const int4 quantized_vs = round(vs * outputScale);
            Store4i8A(output, p, pack_clamp_s8(quantized_vs));
        }
    };

    Algorithm algorithm = {
        1.0 / output.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<float4, float4, float4>(algorithm, beginY, beginX, endY, endX, strideY, strideX, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}
