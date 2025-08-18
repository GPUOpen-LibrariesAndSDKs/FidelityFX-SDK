// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/padding.hlsli"

template<typename TI, typename SO>
void SetPaddingConstant(
    TI input,
    Tensor1i8<SC> constant_value,
    Tensor3i8_NHWC <SO> output,
    ComputeShaderParams computeShaderParams,
    const uint3 paddingBegin,
    const uint3 paddingEnd)
{
    //TODO Use constant_value
    //TODO Find more reliable way of calculating the stride, ratio of slice sizes would work if we'd set input tensor slices to the range we are accessing per thread
    const int3 stride = output.logicalSize / input.logicalSize; // Should probably be always 1 for Pad operator
    
    ResetPaddingInt8(output, computeShaderParams.dispatchThreadID, paddingBegin.x > 0 || paddingEnd.x > 0, paddingBegin.y > 0 || paddingEnd.y > 0, stride.xy);
}

template<typename SI,  typename SO>
void SetPaddingReflect(
    QuantizedTensor3i8_NHWC< SI> input,
    Tensor3i8_NHWC <SO> output,
    ComputeShaderParams computeShaderParams,
    const uint3 paddingBegin,
    const uint3 paddingEnd)
{
    //TODO Implement
}

template<typename SI,  typename SO>
void SetPaddingEdge(
    QuantizedTensor3i8_NHWC< SI> input,
    Tensor3i8_NHWC <SO> output,
    ComputeShaderParams computeShaderParams,
    const uint3 paddingBegin,
    const uint3 paddingEnd)
{
    //TODO Implement
}

template<typename SI, typename SO>
void SetPaddingWrap(
    QuantizedTensor3i8_NHWC< SI> input,
    Tensor1i32<SP> padding,
    Tensor3i8_NHWC <SO> output,
    ComputeShaderParams computeShaderParams,
    const uint3 paddingBegin,
    const uint3 paddingEnd)
{
    //TODO Implement
}

template<typename SI, typename SP, typename SC, typename SO>
void Pad(
    PaddingMode paddingMode,
    QuantizedTensor3i8_NHWC<SI> input,
    Tensor1i32<SP> padding,
    Tensor1i8<SC> constant_value,
    Tensor3i8_NHWC <SO> output,
    ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    
    const int3 paddingBegin = Load4i32(padding, 0).xyz;
    const int3 paddingEnd = Load4i32(padding, 4).xyz;
    
    for (uint y = 0; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            
            if (!ValidPosition(output, poBase))
                continue;
            
            //TODO assert padding values work with padding of tensor
    
            //TODO Can we just update padding memory in place without copy if we can make input and output match?
            
            //Copy non padding memory
           
            for (uint c = 0; c < input.logicalSize.z; c += 4) //Assume we are memory padding to multiples of four for int8
            {
                int8_t4_packed values = Load4i8A(input, poBase + int3(0, 0, c));
                const int16_t4 dequantized = round(values * input.quantizationScale);
                Store4i8A(output, poBase + int3(0, 0, c) + paddingBegin, pack_clamp_s8(dequantized));
            }
            
            //Create copy of output tensor, except the newly added padding memory is not considered part of logical dimensions anymore
            //so we can use our padding reset tools
            Tensor3i8_NHWC < SO > output_reshaped = output;
            output_reshaped.paddingBegin = paddingBegin; //Add on top if we want to reset additional preexisting padding memory as well.
            output_reshaped.paddingEnd = paddingEnd; //Add on top if we want to reset additional preexisting padding memory as well.
            output_reshaped.logicalSize = input.logicalSize; //-= paddingBegin + paddingEnd;
            output_reshaped.threadGroupStorageByteOffset += dot(output.paddingBegin, output.storageByteStrides); //Move in start of tensor memory to the start of the newly created padding memory
    
    
            //Actual padding value set
            switch (paddingMode)
            {
                case PaddingMode::Constant:
                    SetPaddingConstant(input, constant_value, output_reshaped, computeShaderParams, paddingBegin, paddingEnd);
                    break;
                case PaddingMode::Reflect:
                    SetPaddingReflect(input, output_reshaped, computeShaderParams, paddingBegin, paddingEnd);
                    break;
                case PaddingMode::Edge:
                    SetPaddingEdge(input, output_reshaped, computeShaderParams, paddingBegin, paddingEnd);
                    break;
                case PaddingMode::Wrap:
                    SetPaddingWrap(input, output_reshaped, computeShaderParams, paddingBegin, paddingEnd);
                    break;
            }
        }
}

static const uint _Zero_Dword[1] =
{
    // 0 0 0 0
    0x0,
};

//Default constant value to zero
template<typename SI, typename SP, typename SO>
void Pad(
    PaddingMode paddingMode,
    QuantizedTensor3i8_NHWC<SI> input,
    Tensor1i32<SP> padding,
    Tensor3h_NHWC <SO> output,
    ComputeShaderParams computeShaderParams)
{
    const ConstantBufferStorage<1> storage_Zero_Dword = {_Zero_Dword};
        
    const Tensor1i8< ConstantBufferStorage <1> > _Constant_Zero_Dword = {
        1, // logicalSize
        0, // threadGroupSliceStart
        1, // threadGroupSliceSize
        2, // storageSize
        2, // storageByteStrides
        0, // paddingOffset
        0, // paddingOffset
        0, // threadGroupStorageByteOffset
        storage_Zero_Dword
    };
    
    Pad(paddingMode, input, padding, _Constant_Zero_Dword, output, computeShaderParams);

}