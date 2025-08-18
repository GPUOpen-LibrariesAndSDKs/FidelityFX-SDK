// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3b(
    const Tensor3h_NHWC< SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        half weightsScale;

        uint4 kernelSize;
        Tensor3h_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        Tensor1h<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p); }
        half4 LoadInput4(int3 p) { return Load4hA(input, p + int3(1, 1, 0)); }
        half4 LoadWeight4(int4 p) { return unpack_s8s16(Load4i8A(weights, p)) * weightsScale;  }

        half Dot(half4 ws, half4 vs, half acc)
        {
            return dot(ws, vs) + acc;
        }

        void StoreOutput4(int3 p, half4 vs)
        {
            Store4hA(output, p, vs);
        }
    };

    Algorithm algorithm = {
        weights.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedConv2D_3413_NHWC_44<half4, half4, half4>(algorithm, 1, 1, 1, 1, 1, 1, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}