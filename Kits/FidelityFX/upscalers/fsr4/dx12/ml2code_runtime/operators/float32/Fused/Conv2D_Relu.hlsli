// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

void FusedConv2D_Relu(
    uint beginY, uint beginX, 
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3fDesc inputDesc, ByteAddressBuffer input, 
    const Tensor4fDesc weightsDesc, ByteAddressBuffer weights, 
    const Tensor1fDesc biasDesc, ByteAddressBuffer bias, 
    const Tensor3fDesc outputDesc, RWByteAddressBuffer output, 
    const ComputeShaderParams computeShaderParams)
{
    // Assumes strides == 1 and dilations == 1
    if (any(computeShaderParams.dispatchThreadID >= outputDesc.size))
        return;

    const uint m = computeShaderParams.dispatchThreadID.z;
    const int3 pi = int3(computeShaderParams.dispatchThreadID.xy, 0) - int3(beginX, beginY, 0);

    const int kernelStartY = pi.y < 0 ? -pi.y : 0;
    const int readEndY = pi.y + weightsDesc.size.y;
    const int kernelEndY = readEndY >= inputDesc.size.y ? weightsDesc.size.y + inputDesc.size.y - readEndY : weightsDesc.size.y;
    
    float accumulator = bias.Load<float>(biasDesc.OffsetOf(m));

    for (uint c = 0; c != inputDesc.size.z; ++c)
    {
        for (int ky = kernelStartY; ky != kernelEndY; ++ky)
        {
            const int kernelStartX = pi.x < 0 ? -pi.x : 0;
            const int readEndX = pi.x + weightsDesc.size.x;
            const int kernelEndX = readEndX >= inputDesc.size.x ? weightsDesc.size.x + inputDesc.size.x - readEndX : weightsDesc.size.x;

            for (int kx = kernelStartX; kx != kernelEndX; ++kx)
            {
                const uint4 pw = uint4(kx, ky, c, m); // position in weights
                const uint3 ps = pi + pw.xyz; // position in sample in input

                const float w = weights.Load<float>(weightsDesc.OffsetOf(pw));
                const float v = input.Load<float>(inputDesc.OffsetOf(ps));

                accumulator += w * v;
            }
        }
    }

    const uint3 po = computeShaderParams.dispatchThreadID; // position in output
    output.Store(outputDesc.OffsetOf(po), Relu(accumulator));
}
