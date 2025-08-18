// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SB, typename SO>
void FusedConv2D_k1b_Relu(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint strideY, uint strideX,
    uint dilationY, uint dilationX,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{

    struct Algorithm 
    {
        float inputScale;
        float weightsScale;
        float outputScale;

        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        Tensor1h<SB> bias;
        QuantizedTensor3i8_NHWC<SO> output;
    
        ComputeShaderParams computeShaderParams;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        int4 LoadBias4(int p) { return int4(0,0,0,0); }
        int8_t4_packed LoadInput4(int3 p) { return Load4i8A(input, p);}
        int8_t4_packed LoadWeight4(int4 p){ return Load4i8A(weights, p);}

        int Dot(int8_t4_packed _ws, int8_t4_packed _vs, int acc)
        {
            return dot4add_i8packed(_ws, _vs, acc);
        }

        void StoreOutput4(int3 p, int4 vs)
        {
            const half4 outputValue = vs * inputScale * weightsScale + Load4hA(bias, p.z);
            Store4i8A(output, p, pack_clamp_s8(int16_t4(round(Relu(outputValue) * outputScale))));
        
            if(p.z != 0) return;
        }
    };

    Algorithm algorithm = {
        input.quantizationScale,
        weights.quantizationScale,
        1.0f / output.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output, computeShaderParams
    };

    TemplatedConv2D_3413_NHWC_44<int4, int8_t4_packed, int8_t4_packed>(algorithm, beginY, beginX, endY, endX, strideY, strideX, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams);
}

template<typename SI, typename SW, typename SB, typename SO>
void FusedConv2D_k1b_Relu(
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{   
    FusedConv2D_k1b_Relu(0, 0, 0, 0, 1, 1, 0, 0, input, weights, bias, output, computeShaderParams);
}