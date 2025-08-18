// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"


template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1gb(
    uint groups,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        Tensor1h<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p); }
        half4 LoadInput4(int3 p) { return half4(unpack_s8s32(Load4i8A(input, p)) * input.quantizationScale); }
        half4 LoadWeight4(int4 p) { return half4(unpack_s8s32(Load4i8A(weights, p)) * weights.quantizationScale);  }
        float Dot(half4 ws, half4 vs, float acc)
        {
            acc = dot2add(ws.xy, vs.xy, acc);
            return dot2add(ws.zw, vs.zw, acc);
        }

        void StoreOutput4(int3 p, half4 vs)
        {
            Store4hA(output, p, vs);
        }
    };

    Algorithm algorithm = {
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedGroupedConv2D_3413_NHWC_44<float4, float4, float4>(algorithm, 1, 1, 1, 1, 1, 1, groups, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams.numThreads, computeShaderParams.groupThreadID);
}
