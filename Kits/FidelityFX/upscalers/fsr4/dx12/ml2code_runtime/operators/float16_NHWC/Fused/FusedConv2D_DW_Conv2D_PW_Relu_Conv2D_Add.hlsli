// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename SI, typename SW0, typename SB0,typename SW1, typename SB1,typename SW2, typename SB2, typename SO>
void FusedConv2D_DW_Conv2D_PW_Relu_Conv2D_Add(
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const Tensor4h_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const Tensor4h_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (computeShaderParams.numThreads.z != 1)
        return;
    if (weights0.logicalSize.w != 16)
        return;
    if (weights1.logicalSize.w != 32)
        return;
    if (weights2.logicalSize.w != 16)
        return;

    // each thread produces all channels
    if (computeShaderParams.numThreads.z != 1)
        return;

    // All channels are present in input
    // if (input.threadGroupSliceSize.z != weights.logicalSize.z)
    //     return;

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint numFeatures = weights0.logicalSize.w;

    for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

                if (IsAligned(numFeatures, 4))
                {
                    // Store intermediary output in VGPR
                    float conv0_output[16]; //numFeatures
                    half relu_output[32];

                    {
                        for (uint f = 0 ; f != 16; f+=4)
                        {
                            half4 bias = Load4hA(bias0, f);
                            conv0_output[f]   = bias.x;
                            conv0_output[f+1] = bias.y;
                            conv0_output[f+2] = bias.z;
                            conv0_output[f+3] = bias.w;
                        }


                        [unroll]
                        for (uint i = 0; i < 32; i+=4)
                        {
                            half4 accumulator = Load4hA(bias1, i);
                            relu_output[i]   = accumulator.x;
                            relu_output[i+1] = accumulator.y;
                            relu_output[i+2] = accumulator.z;
                            relu_output[i+3] = accumulator.w;
                        }

                        const uint baseReadOffset = input.OffsetOf(piBase);
                        [unroll]
                        for (uint ky = 0; ky != weights0.logicalSize.y; ++ky)
                        {
                            const uint columnReadOffset = baseReadOffset + ky * input.storageByteStrides.y;
                            [unroll]
                            for (uint kx = 0; kx != weights0.logicalSize.x; ++kx)
                            {
                                if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                                    continue;

                                uint inputOffset = columnReadOffset + kx * input.storageByteStrides.x;

                                half4 vs[4];
                                [unroll]
                                for (uint c = 0; c != weights0.logicalSize.z; c += 8)
                                {
                                    const uint4 inputDwords = input.storage.Load4(inputOffset);

                                    vs[(c >> 2)] = Unpack4h(inputDwords.xy);
                                    vs[(c >> 2)+1] = Unpack4h(inputDwords.zw);

                                    inputOffset += 8*2;
                                }

                                [unroll]
                                for (uint f = 0; f != numFeatures; ++f )
                                {
                                    uint kernelOffset = weights0.OffsetOf(uint4(kx, ky, 0, f));
                                    for (uint c = 0; c != weights0.logicalSize.z; c += 8)
                                    {
                                        const uint4 kernelDwords = weights0.storage.Load4(kernelOffset);
                                        half4 ws0 = Unpack4h(kernelDwords.xy);
                                        half4 ws1 = Unpack4h(kernelDwords.zw);

                                        conv0_output[f] = dot2add(ws0.xy, vs[c >> 2].xy, conv0_output[f]);
                                        conv0_output[f] = dot2add(ws0.zw, vs[c >> 2].zw, conv0_output[f]);
                                        conv0_output[f] = dot2add(ws1.xy, vs[(c >> 2)+1].xy, conv0_output[f]);
                                        conv0_output[f] = dot2add(ws1.zw, vs[(c >> 2)+1].zw, conv0_output[f]);

                                        kernelOffset += 8*2;
                                    }
                                }
                            }
                        }
                    }

                    for (uint i = 0; i < 32; i++)
                    {
                        half accumulator = relu_output[i];
                        for (int j = 0; j < 16; j+=4)
                        {
                            half4 conv_output = half4(conv0_output[j], conv0_output[j+1], conv0_output[j+2], conv0_output[j+3]);
                            accumulator += dot(conv_output, Load4hA(weights1, uint4(0,0,j,i)));
                        }

                        relu_output[i] = Relu(accumulator);
                    }


                    for (uint i = 0; i < 16; i+=4) // num channels in the output
                    {
                        float4 accumulator = Load4hA(bias2, i);
                        for (uint j = 0; j < 32; j+=4)
                        {
                            const half4 relu_out = half4(relu_output[j], relu_output[j+1], relu_output[j+2], relu_output[j+3]);
                            //accumulator.x += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i)));
                            half4 weights = Load4hA(weights2, uint4(0, 0, j, i));
                            accumulator.x = dot2add(relu_out.xy, weights.xy, accumulator.x);
                            accumulator.x = dot2add(relu_out.zw, weights.zw, accumulator.x);

                            weights = Load4hA(weights2, uint4(0, 0, j, i+1));
                            accumulator.y = dot2add(relu_out.xy, weights.xy, accumulator.y);
                            accumulator.y = dot2add(relu_out.zw, weights.zw, accumulator.y);

                            weights = Load4hA(weights2, uint4(0, 0, j, i+2));
                            accumulator.z = dot2add(relu_out.xy, weights.xy, accumulator.z);
                            accumulator.z = dot2add(relu_out.zw, weights.zw, accumulator.z);

                            weights = Load4hA(weights2, uint4(0, 0, j, i+3));
                            accumulator.w = dot2add(relu_out.xy, weights.xy, accumulator.w);
                            accumulator.w = dot2add(relu_out.zw, weights.zw, accumulator.w);
                        }

                        const int3 po = int3(poBase.xy, i);
                        Store4hA(output, po, half4(accumulator) + Load4hA(input, po));
                    }
                }
            }


}
