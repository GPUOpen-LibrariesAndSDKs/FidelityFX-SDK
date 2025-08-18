// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"

template<typename SI, typename SW0, typename SB0 ,typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FusedConv2D_k3p1b_Conv2D_k1b_SqrSwish_Conv2D_k1b(
    const Tensor3h_NHWC<SI>     input,
    const Tensor4h_NHWC<SW0>    weights0,
    const Tensor1h<SB0>         bias0,
    const Tensor4h_NHWC<SW1>    weights1,
    const Tensor1h<SB1>         bias1,
    const Tensor4h_NHWC<SW2>    weights2,
    const Tensor1h<SB2>         bias2,
    const Tensor3h_NHWC<SO>     output,
    const ComputeShaderParams computeShaderParams)
{
    return; //TODO remove
    
    //TODO Op Header
    
    //TODO conv0 3x3 input -> result0
    
    //TODO conv1 1x1 result0 -> result1
    
    //TODO conv2 1x1 result1 -> result2
    
    //Write result2
}

template<typename SI, typename SW0, typename SB0 ,typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FusedConv2D_k3p1b_Conv2D_k1b_SqrSwish_Conv2D_k1b(
    const Tensor3h_NHWC<SI>     input,
    const Tensor4h_NHWC<SW0>    weights0,
    const Tensor1h<SB0>         bias0,
    const Tensor4h_NHWC<SW1>    weights1,
    const Tensor1h<SB1>         bias1,
    const Tensor4h_NHWC<SW2>    weights2,
    const Tensor1h<SB2>         bias2,
    const Tensor3h_NHWC<SO>     output0,
    const Tensor3h_NHWC<SO>     output2,
    const ComputeShaderParams computeShaderParams)
{
    return; //TODO remove
    
    //TODO Op Header
    
    //TODO conv0 3x3 input -> result0
    
    //TODO write result0
    
    //TODO conv1 1x1 result0 -> result1
    
    //TODO conv2 1x1 result1 -> result2
    
    //TODO Write result2
}

template<typename SI, typename SW0, typename SB0 ,typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FusedConv2D_k3p1b_Conv2D_k1b_SqrSwish_Conv2D_k1b(
    const Tensor3h_NHWC<SI>     input,
    const Tensor4h_HWNC< SW0>   weights0,
    const Tensor1h<SB0>         bias0,
    const Tensor4h_HWNC< SW1>   weights1,
    const Tensor1h<SB1>         bias1,
    const Tensor4h_HWNC< SW2>   weights2,
    const Tensor1h<SB2>         bias2,
    const Tensor3h_NHWC<SO>     output,
    const ComputeShaderParams computeShaderParams)
{
    return; //TODO remove
    
    //TODO Op Header
    
    //TODO conv0 3x3 input -> result0
    
    //TODO conv1 1x1 result0 -> result1
    
    //TODO conv2 1x1 result1 -> result2
    
    //Write result2
}

template<typename SI, typename SW0, typename SB0 ,typename SW1, typename SB1, typename SW2, typename SB2, typename SO>
void FusedConv2D_k3p1b_Conv2D_k1b_SqrSwish_Conv2D_k1b(
    const Tensor3h_NHWC<SI>      input,
    const Tensor4h_HWNC< SW0>    weights0,
    const Tensor1h<SB0>          bias0,
    const Tensor4h_HWNC< SW1>    weights1,
    const Tensor1h<SB1>          bias1,
    const Tensor4h_HWNC< SW2>    weights2,
    const Tensor1h<SB2>          bias2,
    const Tensor4h_HWNC< SO>     output0,
    const Tensor3h_NHWC<SO>      output2,
    const ComputeShaderParams computeShaderParams)
{
    return; //TODO remove
    
    //TODO Op Header
    
    //TODO conv0 3x3 input -> result0
    
    //TODO write result0
    
    //TODO conv1 1x1 result0 -> result1
    
    //TODO conv2 1x1 result1 -> result2
    
    //TODO Write result2
}