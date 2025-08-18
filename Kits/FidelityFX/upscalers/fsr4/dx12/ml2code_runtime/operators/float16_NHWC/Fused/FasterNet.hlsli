// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"



template<uint numFeatures, typename SI, typename SW0, typename SW1,  typename SB, typename SW2, typename SO>
void FasterNet48_NoGroup(
    const uint3 perThreadWork,
    const int3 workOffset,
    const uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW0> weights0,
    const Tensor4h_NHWC<SW1> weights1,
    const Tensor1h<SB> bias1,
    const Tensor4h_NHWC<SW2> weights2,
    const Tensor3h_NHWC<SO> output
)
{

    if (numGroups != 1) return;
    if (numFeatures != 48) return; // special case where we don't split concat + conv inputs on half

    for (uint y = 0; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase, false))
                continue;
            const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

            // Store intermediary output in VGPR
            half conv0_output[numFeatures];
            int3 pi = piBase;

            /************************************************/
            // Concat
            /************************************************/
            for (uint z = 16; z < numFeatures; z+=4)
            {
                int3 po = int3(poBase.xy, z);

                half4 input4 = Load4hA(input, po);
                conv0_output[z]   = input4.x;
                conv0_output[z+1] = input4.y;
                conv0_output[z+2] = input4.z;
                conv0_output[z+3] = input4.w;
            }
            /************************************************/
            // Conv 3x3 (PW)
            /************************************************/
            uint baseReadOffset = input.OffsetOf(piBase);
            [unroll]
            for (uint f = 0 ; f < 16; ++f)
            {
                half accumulator = 0;
                [unroll]
                for (uint ky = 0; ky != weights0.logicalSize.y; ++ky)
                {
                    const uint columnReadOffset = baseReadOffset + ky * input.storageByteStrides.y;
                    [unroll]
                    for (uint kx = 0; kx != weights0.logicalSize.x; ++kx)
                    {
                        int3 psBase = int3(pi.xy + uint2(kx, ky), 0);
                        uint inputOffset = columnReadOffset + kx * input.storageByteStrides.x;
                        /*Grzegorz Makowski Note: Edge padding - instead of pad with zeros we pad with the border pixel values.
                        It turns out that the lack of edge padding does not seem to be visible in games but improves performance a lot ( especially in layers with big number of features like bottlenecks)
                        However if in the future we would want it we should implement additional mechanism of indication if that edge padding is necessary or not if we want to be generic.*/

                        if(!ValidPosition(input, psBase, false))
                        {
                            continue;
                        }
                        // if(!InsideTensor(input, psBase))// Edge padding
                        // {
                        //     int3 pad_idx = EdgeOffset(input, psBase);
                        //     inputOffset = input.OffsetOf(pad_idx);
                        // }

                        uint kernelOffset = weights0.OffsetOf(uint4(kx, ky, 0, f));

                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 8)
                        {
                            const uint4 inputDwords = input.storage.Load4(inputOffset);
                            const uint4 kernelDwords = weights0.storage.Load4(kernelOffset);

                            accumulator += dot(Unpack4h(inputDwords.xy), Unpack4h(kernelDwords.xy));
                            accumulator += dot(Unpack4h(inputDwords.zw), Unpack4h(kernelDwords.zw));

                            inputOffset += 8*2;
                            kernelOffset += 8*2;
                        }
                    }
                }

                conv0_output[f] = accumulator;
            }
            /************************************************/
            // Conv 1x1 (Expand) + Relu
            /************************************************/
            half relu_output[numFeatures*2];
            for (uint i = 0; i < numFeatures*2; i++)
            {
                half accumulator = Load1hU(bias1, i);
                for (int j = 0; j < numFeatures; j+=4)
                {
                    half4 conv_output = half4(conv0_output[j], conv0_output[j+1], conv0_output[j+2], conv0_output[j+3]);
                    accumulator += dot(conv_output, Load4hA(weights1, uint4(0,0,j,i)));
                }

                relu_output[i] = Relu(accumulator);
            }
            /************************************************/
            // Conv 1x1 (Contract) + Add
            /************************************************/
            for (uint i = 0; i < numFeatures; i+=4) // num channels in the output
            {
                half4 accumulator = 0;
                for (uint j = 0; j < numFeatures*2; j+=4)
                {
                    half4 relu_out = half4(relu_output[j], relu_output[j+1], relu_output[j+2], relu_output[j+3]);
                    accumulator.x += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i)));
                    accumulator.y += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+1)));
                    accumulator.z += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+2)));
                    accumulator.w += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+3)));
                }

                const int3 po = int3(poBase.xy, i);
                Store4hA(output, po, accumulator + Load4hA(input, po));
            }
        }
}

template<uint numFeatures, typename SI, typename SW0, typename SW1,  typename SB, typename SW2, typename SO>
void FasterNet_Group2(
    const uint3 perThreadWork,
    const int3 workOffset,
    const uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW0> weights0,
    const Tensor4h_NHWC<SW1> weights1,
    const Tensor1h<SB> bias1,
    const Tensor4h_NHWC<SW2> weights2,
    const Tensor3h_NHWC<SO> output
)
{
    if(numGroups != 2) return;
    if(numFeatures != 16 && numFeatures != 32 && numFeatures!= 64) return;
    for (uint y = 0; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase, false))
                continue;

            const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

            // Store intermediary output in VGPR
            half conv0_output[numFeatures];
            int3 pi = piBase;

            /************************************************/
            // Concat
            /************************************************/
            for (uint z = numFeatures / 2; z < numFeatures; z+=4)
            {
                int3 po = int3(poBase.xy, z);

                half4 input4 = Load4hA(input, po);
                conv0_output[z]   = input4.x;
                conv0_output[z+1] = input4.y;
                conv0_output[z+2] = input4.z;
                conv0_output[z+3] = input4.w;
            }

            uint baseReadOffset = input.OffsetOf(piBase);
            /************************************************/
            // Conv 3x3 (PW) [group]
            /************************************************/
            int group_offset = 0;
            [unroll]
            for (uint f = 0 ; f < numFeatures / 2; ++f)
            {
                half accumulator = 0;
                if (f == numFeatures / 4)
                {
                    group_offset = weights0.logicalSize.z;
                    pi = int3(piBase.xy, weights0.logicalSize.z);
                    baseReadOffset = input.OffsetOf(pi);
                }

                [unroll]
                for (uint ky = 0; ky != weights0.logicalSize.y; ++ky)
                {
                    const uint columnReadOffset = baseReadOffset + ky * input.storageByteStrides.y;
                    [unroll]
                    for (uint kx = 0; kx != weights0.logicalSize.x; ++kx)
                    {
                        int3 psBase = int3(pi.xy + uint2(kx, ky), group_offset);
                        uint inputOffset = columnReadOffset + kx * input.storageByteStrides.x;
                        /*Grzegorz Makowski Note: Edge padding - instead of pad with zeros we pad with the border pixel values.
                        It turns out that the lack of edge padding does not seem to be visible in games but improves performance a lot ( especially in layers with big number of features like bottlenecks)
                        However if in the future we would want it we should implement additional mechanism of indication if that edge padding is necessary or not if we want to be generic.*/
                        if(!ValidPosition(input, psBase, false))
                        {
                            continue;
                        }
                        // if(!InsideTensor(input, psBase))// Edge padding
                        // {
                        //     int3 pad_idx = EdgeOffset(input, psBase);
                        //     inputOffset = input.OffsetOf(pad_idx);
                        // }

                        uint kernelOffset = weights0.OffsetOf(uint4(kx, ky, 0, f));

                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 8)
                        {
                            const uint4 inputDwords = input.storage.Load4(inputOffset);
                            const uint4 kernelDwords = weights0.storage.Load4(kernelOffset);

                            accumulator += dot(Unpack4h(inputDwords.xy), Unpack4h(kernelDwords.xy));
                            accumulator += dot(Unpack4h(inputDwords.zw), Unpack4h(kernelDwords.zw));

                            inputOffset += 8*2;
                            kernelOffset += 8*2;
                        }
                    }
                }

                conv0_output[f] = accumulator;
            }
            /************************************************/
            // Conv 1x1 (Expand) + Relu
            /************************************************/
            half relu_output[numFeatures*2];
            for (uint i = 0; i < numFeatures*2; i++)
            {
                half accumulator = Load1hU(bias1, i);
                for (int j = 0; j < numFeatures; j+=4)
                {
                    half4 conv_output = half4(conv0_output[j], conv0_output[j+1], conv0_output[j+2], conv0_output[j+3]);
                    accumulator += dot(conv_output, Load4hA(weights1, uint4(0,0,j,i)));
                }

                relu_output[i] = Relu(accumulator);
            }
            /************************************************/
            // Conv 1x1 (Contract) + Add
            /************************************************/
            for (uint i = 0; i < numFeatures; i+=4) // num channels in the output
            {
                half4 accumulator = 0;
                for (uint j = 0; j < numFeatures*2; j+=4)
                {
                    half4 relu_out = half4(relu_output[j], relu_output[j+1], relu_output[j+2], relu_output[j+3]);
                    accumulator.x += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i)));
                    accumulator.y += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+1)));
                    accumulator.z += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+2)));
                    accumulator.w += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+3)));
                }

                const int3 po = int3(poBase.xy, i);
                Store4hA(output, po, accumulator + Load4hA(input, po));
            }
        }
}

template<uint numFeatures, typename SI, typename SW0, typename SW1,  typename SB, typename SW2, typename SO>
void FasterNet_NoGroup(
    const uint3 perThreadWork,
    const int3 workOffset,
    const uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW0> weights0,
    const Tensor4h_NHWC<SW1> weights1,
    const Tensor1h<SB> bias1,
    const Tensor4h_NHWC<SW2> weights2,
    const Tensor3h_NHWC<SO> output
)
{
    if(numGroups != 1) return;
    if(numFeatures != 16 && numFeatures != 32 && numFeatures!= 64) return;
    for (uint y = 0; y < perThreadWork.y; y++)
        for (uint x = 0; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase, false))
               continue;

            const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

            // Store intermediary output in VGPR
            half conv0_output[numFeatures];
            int3 pi = piBase;

            /************************************************/
            // Concat
            /************************************************/
            for (uint z = numFeatures / 2; z < numFeatures; z+=4)
            {
                int3 po = int3(poBase.xy, z);

                half4 input4 = Load4hA(input, po);
                conv0_output[z]   = input4.x;
                conv0_output[z+1] = input4.y;
                conv0_output[z+2] = input4.z;
                conv0_output[z+3] = input4.w;
            }

            uint baseReadOffset = input.OffsetOf(piBase);
            /************************************************/
            // Conv 3x3 (PW) [group]
            /************************************************/
            [unroll]
            for (uint f = 0 ; f < numFeatures / 2; ++f)
            {
                half accumulator = 0;

                [unroll]
                for (uint ky = 0; ky != weights0.logicalSize.y; ++ky)
                {
                    const uint columnReadOffset = baseReadOffset + ky * input.storageByteStrides.y;

                    [unroll]
                    for (uint kx = 0; kx != weights0.logicalSize.x; ++kx)
                    {
                        int3 psBase = int3(pi.xy + uint2(kx, ky), 0);
                        uint inputOffset = columnReadOffset + kx * input.storageByteStrides.x;
                        /*Grzegorz Makowski Note: Edge padding - instead of pad with zeros we pad with the border pixel values.
                        It turns out that the lack of edge padding does not seem to be visible in games but improves performance a lot ( especially in layers with big number of features like bottlenecks)
                        However if in the future we would want it we should implement additional mechanism of indication if that edge padding is necessary or not if we want to be generic.*/
                        if (!ValidPosition(input, psBase, false))
                        {
                            continue;
                        }
                        // if(!InsideTensor(input, psBase))// Edge padding
                        // {
                        //     int3 pad_idx = EdgeOffset(input, psBase);
                        //     inputOffset = input.OffsetOf(pad_idx);
                        // }

                        uint kernelOffset = weights0.OffsetOf(uint4(kx, ky, 0, f));

                        [unroll]
                        for (uint c = 0; c != weights0.logicalSize.z; c += 8)
                        {
                            const uint4 inputDwords = input.storage.Load4(inputOffset);
                            const uint4 kernelDwords = weights0.storage.Load4(kernelOffset);

                            accumulator += dot(Unpack4h(inputDwords.xy), Unpack4h(kernelDwords.xy));
                            accumulator += dot(Unpack4h(inputDwords.zw), Unpack4h(kernelDwords.zw));

                            inputOffset += 8*2;
                            kernelOffset += 8*2;
                        }
                    }
                }

                conv0_output[f] = accumulator;
            }
            /************************************************/
            // Conv 1x1 (Expand) + Relu
            /************************************************/
            half relu_output[numFeatures*2];
            for (uint i = 0; i < numFeatures*2; i++)
            {
                half accumulator = Load1hU(bias1, i);
                for (int j = 0; j < numFeatures; j+=4)
                {
                    half4 conv_output = half4(conv0_output[j], conv0_output[j+1], conv0_output[j+2], conv0_output[j+3]);
                    accumulator += dot(conv_output, Load4hA(weights1, uint4(0,0,j,i)));
                }

                relu_output[i] = Relu(accumulator);
            }
            /************************************************/
            // Conv 1x1 (Contract) + Add
            /************************************************/
            for (uint i = 0; i < numFeatures; i+=4) // num channels in the output
            {
                half4 accumulator = 0;
                for (uint j = 0; j < numFeatures*2; j+=4)
                {
                    half4 relu_out = half4(relu_output[j], relu_output[j+1], relu_output[j+2], relu_output[j+3]);
                    accumulator.x += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i)));
                    accumulator.y += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+1)));
                    accumulator.z += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+2)));
                    accumulator.w += dot(relu_out, Load4hA(weights2, uint4(0, 0, j, i+3)));
                }

                const int3 po = int3(poBase.xy, i);
                Store4hA(output, po, accumulator + Load4hA(input, po));
            }
        }
}

template<uint numFeatures, typename SI, typename SW, typename SB, typename SO>
void FasterNet64_Waveops(
                    uint numGroups,
                    const Tensor3h_NHWC<SI> input,
                    const Tensor4h_NHWC<SW> weights0,
                    const Tensor4h_NHWC<SW> weights1,
                    const Tensor1h<SB> bias1,
                    const Tensor4h_NHWC<SW> weights2,
                    const Tensor3h_NHWC<SO> output,
                    const uint3 numThreads,
                    const uint3 threadIndex)
{
#define WAVE_64

    uint NumZPerThread = 4;

#ifdef WAVE_64
    uint ThreadIndex = WaveGetLaneIndex();
#else
    uint ThreadIndex = threadIndex.x;
#endif

    uint4 NumThreads_2D = uint4(output.threadGroupSliceSize.xy, 4, 4);
    uint4 ThreadOffsets_2D = uint4(NumThreads_2D.z * NumThreads_2D.w,
                                   NumThreads_2D.z * NumThreads_2D.w * NumThreads_2D.x,
                                   1,
                                   NumThreads_2D.z);
    uint4 ThreadId_2D;
    {
        ThreadId_2D.z = (ThreadIndex / ThreadOffsets_2D.z) % NumThreads_2D.z;
        ThreadId_2D.w = (ThreadIndex / ThreadOffsets_2D.w) % NumThreads_2D.w;
        ThreadId_2D.x = (ThreadIndex / ThreadOffsets_2D.x) % NumThreads_2D.x;
        ThreadId_2D.y = (ThreadIndex / ThreadOffsets_2D.y) % NumThreads_2D.y;
    }

    uint3 NumThreads_1D = uint3(output.threadGroupSliceSize.xy, 16);
    uint3 ThreadId_1D;
    {
        ThreadId_1D.z = ThreadIndex % NumThreads_1D.z;
        ThreadId_1D.x = (ThreadIndex / (NumThreads_1D.z)) % NumThreads_1D.x;
        ThreadId_1D.y = (ThreadIndex / (NumThreads_1D.z * NumThreads_1D.x)) % NumThreads_1D.y;
    }

    // NOTE: Thread is outside of bounds, skip it
    if (!(all(output.threadGroupSliceStart.xy >= 0) && all(output.threadGroupSliceStart.xy < output.logicalSize.xy)))
    {
        return;
    }

    //
    // NOTE: 2D Convolution (32 -> 32, Group Count = 2)
    //
    half4 OutputConv0;
    {
#define UNROLL_NUM_0 4 // NOTE: Controls the # of features we compute in parallel (for storing purposes)
        int3 PosInputBase = int3(output.threadGroupSliceStart.xy, 0) + ThreadId_2D.xyz;
        PosInputBase.z *= NumZPerThread; // Account for how much loading we do in each thread

        // NOTE: Number of wave ops for adds is kind of like a binary search, where each wave op increases summed thread count by a
        // multiple of 2
        uint NumWaveOps = log2(NumThreads_2D.z & -NumThreads_2D.z);

        uint NumOuterIterations = weights0.logicalSize.w / (UNROLL_NUM_0 * NumThreads_2D.w);
        for (uint OuterFeatureId = 0; OuterFeatureId < NumOuterIterations; ++OuterFeatureId)
        {
            float Accumulator[UNROLL_NUM_0];

            [unroll]
            for (uint UnRollId = 0; UnRollId < UNROLL_NUM_0; ++UnRollId)
            {
                Accumulator[UnRollId] = 0;
            }

            // In the general case, we need to compute this inside the below loop, but since we will never have 1 thread in one group and
            // a different thread in another at the same time, we can calculate it here
            uint GroupConvId;
            uint HalfInputDepth = 16;
            {
                uint FeatureId = OuterFeatureId * UNROLL_NUM_0 * NumThreads_2D.w;
                GroupConvId = FeatureId / HalfInputDepth;
            }

            [unroll]
            for (uint InnerFeatureId = 0; InnerFeatureId < UNROLL_NUM_0; ++InnerFeatureId)
            {
                // TODO: This has worse load patterns for weight memory, but it makes the wave ops easier. Is that a good trade off? Can we swizzle?
                uint FeatureId = (OuterFeatureId * UNROLL_NUM_0 * NumThreads_2D.w + ThreadId_2D.w * NumThreads_2D.w + InnerFeatureId);

                // NOTE: These unrolls are slower at the current moment. Check later
                [unroll]
                for (int KernelY = 0; KernelY < weights0.logicalSize.y; ++KernelY)
                {
                    [unroll]
                    for (int KernelX = 0; KernelX < weights0.logicalSize.x; ++KernelX)
                    {
                        int4 PosWeight = int4(KernelX, KernelY, PosInputBase.z, FeatureId);
                        int3 PosInput = PosInputBase + int3(KernelX - 1, KernelY - 1, GroupConvId * HalfInputDepth);
                        half4 ValueWeights = Load4hA(weights0, PosWeight);
                        half4 ValueInputs = Load4hA(input, PosInput);
                        if (KernelX == 0 && PosInput.x < 0)
                        {
                            ValueInputs = 0;
                        }

                        if (KernelY == 0 && PosInput.y < 0)
                        {
                            ValueInputs = 0;
                        }

                        if (KernelX == 2 && PosInput.x >= input.logicalSize.x)
                        {
                            ValueInputs = 0;
                        }

                        if (KernelY == 2 && PosInput.y >= input.logicalSize.y)
                        {
                            ValueInputs = 0;
                        }

                        Accumulator[InnerFeatureId].x = dot2add(ValueInputs.xy, ValueWeights.xy, Accumulator[InnerFeatureId].x);
                        Accumulator[InnerFeatureId].x = dot2add(ValueInputs.zw, ValueWeights.zw, Accumulator[InnerFeatureId].x);
                    }
                }
            }

            [unroll]
            for (uint UnRollId = 0; UnRollId < UNROLL_NUM_0; ++UnRollId)
            {
                [unroll]
                for (uint WaveOpId = 0; WaveOpId < NumWaveOps; ++WaveOpId)
                {
#ifdef WAVE_64
                    Accumulator[UnRollId].x += WaveReadLaneAt(Accumulator[UnRollId].x, (ThreadIndex ^ (1 << WaveOpId)));
#else
                    Accumulator[UnRollId].x += WaveReadLaneAt(Accumulator[UnRollId].x, (ThreadIndex + (1 << WaveOpId)));
#endif
                }
            }

            // NOTE: We find the thread id that we want to store our accumulated features
            [unroll]
            for (uint UnRollId = 0; UnRollId < (UNROLL_NUM_0 / NumZPerThread); ++UnRollId)
            {
                // mod(Pos1D.z, 16) = (Pos2D.w + LoopId_0 * 4)[0, 7]
                uint LoopId = (ThreadId_1D.z) / 4;
                uint1 WriteFeatureId = OuterFeatureId * UNROLL_NUM_0 * NumZPerThread + UnRollId * NumZPerThread + ThreadId_2D.w * NumZPerThread;
                half4 WriteResult = half4(Accumulator[NumZPerThread * UnRollId + 0].x,
                                          Accumulator[NumZPerThread * UnRollId + 1].x,
                                          Accumulator[NumZPerThread * UnRollId + 2].x,
                                          Accumulator[NumZPerThread * UnRollId + 3].x);

                // TODO: Can this be simplified?
                // NOTE: Read from the accumluated thread (threadid.z = 0) to the thread that actually wants to store these values
                uint4 ReadThreadId = uint4(ThreadId_2D.x, ThreadId_2D.y, 0, ThreadId_1D.z - LoopId * UNROLL_NUM_0);
                uint ReadThreadIndex = dot(ReadThreadId, ThreadOffsets_2D);
                WriteResult = WaveReadLaneAt(WriteResult, ReadThreadIndex);

                // TODO: Is this a select operation?
                if (OuterFeatureId == LoopId)
                {
                    OutputConv0 = WriteResult;
                }
            }
        }
    }

    int3 PosInputBase = int3(output.threadGroupSliceStart.xy, 0) + ThreadId_1D.xyz;
    PosInputBase.z *= NumZPerThread; // Account for how much loading we do in each thread
    uint2 PosOutputBase = PosInputBase.xy;

    //
    // NOTE: Concat
    //
    // TODO: Can I cache inputs in LDS or elsewhere to not have to reload? Realistically, we are grabbing inputs here, earlier above (but only half), and later for skip connections
    if ((ThreadId_1D.z * 4) >= 32)
    {
        OutputConv0 = Load4hA(input, PosInputBase);
    }

    // NOTE: Number of wave ops for adds is kind of like a binary search, where each wave op increases summed thread count by a
    // multiple of 2
    uint NumWaveOps = log2(NumThreads_1D.z & -NumThreads_1D.z);

    //
    // NOTE: 1D Convolution (64 -> 128)
    //

    // NOTE: We store intermediary values, so that we can do our second 1d convolution
    half4 OutputConv1[2];
    {
#define UNROLL_NUM_1 8 // NOTE: Controls the # of features we compute in parallel (for storing purposes)
        uint NumOuterIterations = weights1.logicalSize.w / (UNROLL_NUM_1);
        for (uint OuterFeatureId = 0; OuterFeatureId < NumOuterIterations; ++OuterFeatureId)
        {
            float Accumulator[UNROLL_NUM_1];

            [unroll]
            for (uint UnRollId = 0; UnRollId < UNROLL_NUM_1; ++UnRollId)
            {
                Accumulator[UnRollId] = 0;
            }

            for (uint InnerFeatureId = 0; InnerFeatureId < UNROLL_NUM_1; ++InnerFeatureId)
            {
                uint FeatureId = OuterFeatureId * UNROLL_NUM_1 + InnerFeatureId;
                uint4 PosWeight = uint4(0, 0, PosInputBase.z, FeatureId);
                half4 ValueWeights = Load4hA(weights1, PosWeight);

                Accumulator[InnerFeatureId] = dot2add(OutputConv0.xy, ValueWeights.xy, Accumulator[InnerFeatureId].x);
                Accumulator[InnerFeatureId] = dot2add(OutputConv0.zw, ValueWeights.zw, Accumulator[InnerFeatureId].x);
            }

            [unroll]
            for (uint UnRollId = 0; UnRollId < UNROLL_NUM_1; ++UnRollId)
            {
                // NOTE: Accumulate values here to have fewer wave ops
                [unroll]
                for (uint WaveOpId = 0; WaveOpId < NumWaveOps; ++WaveOpId)
                {
#ifdef WAVE_64
                    Accumulator[UnRollId].x += WaveReadLaneAt(Accumulator[UnRollId].x, (ThreadIndex ^ (1 << WaveOpId)));
#else
                    Accumulator[UnRollId].x += WaveReadLaneAt(Accumulator[UnRollId].x, (ThreadIndex + (1 << WaveOpId)));
#endif
                }
            }

            // NOTE: We find the thread id that we want to store our accumulated features
            [unroll]
            for (uint UnRollId = 0; UnRollId < (UNROLL_NUM_1 / NumZPerThread); ++UnRollId)
            {
                uint WriteFeatureId = OuterFeatureId * UNROLL_NUM_1 + UnRollId * NumZPerThread;
                uint HalfId = (WriteFeatureId / 64); // TODO: Wave read first lane opt here?
                uint WriteThreadId = (WriteFeatureId - 64 * HalfId) / NumZPerThread;

                half4 WriteResult = half4(Accumulator[NumZPerThread * UnRollId + 0].x,
                                          Accumulator[NumZPerThread * UnRollId + 1].x,
                                          Accumulator[NumZPerThread * UnRollId + 2].x,
                                          Accumulator[NumZPerThread * UnRollId + 3].x);
                WriteResult += Load4hA(bias1, WriteFeatureId);
                WriteResult = Relu(WriteResult);

                // NOTE: Read from the accumluated thread (threadid.z = 0) to the thread that actually wants to store these values
                uint ReadThreadId = ThreadId_1D.x * NumThreads_1D.z + ThreadId_1D.y * NumThreads_1D.x * NumThreads_1D.z;
                WriteResult = WaveReadLaneAt(WriteResult, ReadThreadId);

                // TODO: Can this be a select opreator?
                if (ThreadId_1D.z == WriteThreadId)
                {
                    // NOTE: This avoids movrel instructions, which lowers register usage and
                    // should just be faster
                    if (HalfId == 0)
                    {
                        OutputConv1[0] = WriteResult;
                    }
                    else
                    {
                        OutputConv1[1] = WriteResult;
                    }
                }
            }
        }
    }

    //
    // NOTE: 1D Convolution (128 -> 64)
    //

    {
#define UNROLL_NUM_2 8
        uint NumOuterIterations = weights2.logicalSize.w / (UNROLL_NUM_2);
        for (uint OuterFeatureId = 0; OuterFeatureId < NumOuterIterations; ++OuterFeatureId)
        {
            float Accumulator[UNROLL_NUM_2];

            [unroll]
            for (uint UnRollId = 0; UnRollId < UNROLL_NUM_2; ++UnRollId)
            {
                Accumulator[UnRollId] = 0;
            }

            [unroll]
            for (uint InnerFeatureId = 0; InnerFeatureId < UNROLL_NUM_2; ++InnerFeatureId)
            {
                uint FeatureId = OuterFeatureId * UNROLL_NUM_2 + InnerFeatureId;

                uint4 PosWeight = uint4(0, 0, PosInputBase.z, FeatureId);
                uint3 PosInput = PosInputBase;

                {
                    half4 ValueWeights = Load4hA(weights2, PosWeight);
                    half4 ValueInputs = OutputConv1[0];
                    Accumulator[InnerFeatureId] = dot2add(ValueInputs.xy, ValueWeights.xy, Accumulator[InnerFeatureId].x);
                    Accumulator[InnerFeatureId] = dot2add(ValueInputs.zw, ValueWeights.zw, Accumulator[InnerFeatureId].x);
                }

                PosWeight.z += 64;

                {
                    half4 ValueWeights = Load4hA(weights2, PosWeight);
                    half4 ValueInputs = OutputConv1[1];
                    Accumulator[InnerFeatureId] = dot2add(ValueInputs.xy, ValueWeights.xy, Accumulator[InnerFeatureId].x);
                    Accumulator[InnerFeatureId] = dot2add(ValueInputs.zw, ValueWeights.zw, Accumulator[InnerFeatureId].x);
                }
            }

            [unroll]
            for (uint UnRollId = 0; UnRollId < UNROLL_NUM_2; ++UnRollId)
            {
                // NOTE: Accumulate values here to have fewer wave ops
                [unroll]
                for (uint WaveOpId = 0; WaveOpId < NumWaveOps; ++WaveOpId)
                {
#ifdef WAVE_64
                    Accumulator[UnRollId] += WaveReadLaneAt(Accumulator[UnRollId], (ThreadIndex ^ (1 << WaveOpId)));
#else
                    Accumulator[UnRollId] += WaveReadLaneAt(Accumulator[UnRollId], (ThreadIndex + (1 << WaveOpId)));
#endif
                }
            }

            // NOTE: For grouped convs, we store twice at a time, one for each grouped feature
            if (ThreadId_1D.z == 0)
            {
                [unroll]
                for (uint UnRollId = 0; UnRollId < (UNROLL_NUM_2 / NumZPerThread); ++UnRollId)
                {
                    uint WriteFeatureId = OuterFeatureId * UNROLL_NUM_2 + UnRollId * NumZPerThread;
                    uint3 PosOutput = uint3(PosOutputBase, WriteFeatureId);
                    half4 WriteResult = half4(Accumulator[NumZPerThread * UnRollId + 0],
                                              Accumulator[NumZPerThread * UnRollId + 1],
                                              Accumulator[NumZPerThread * UnRollId + 2],
                                              Accumulator[NumZPerThread * UnRollId + 3]);

                    // NOTE: Compute skip connection
                    // TODO: This is inefficient
                    WriteResult += Load4hA(input, PosOutput);
                    Store4hA(output, PosOutput, WriteResult);
                }
            }
        }
    }
}

template<uint numFeatures, typename SI, typename SW0, typename SB, typename SW1,typename SW2, typename SO>
void FasterNet(
    uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW0> weights0,
    const Tensor4h_NHWC<SW1> weights1,
    const Tensor1h<SB> bias1,
    const Tensor4h_NHWC<SW2> weights2,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    if (numFeatures == 64 || numFeatures == 32|| numFeatures == 16)
    {
        // Note: This is something we want to run on AMD hardware
        // TODO add some preprocessor mechanism instead of remember to uncomment.
        // if(numFeatures == 64 && numGroups == 1)
        // {
        //     FasterNet64_Waveops<numFeatures, SI, SW, SB, SO>(numGroups,
        //                input,
        //               weights0,
        //               weights1,
        //               bias1,
        //               weights2,
        //               output,
        //               computeShaderParams.numThreads,
        //               computeShaderParams.groupThreadID);
        //     return;
        // }
        if(numGroups == 1)
        {
            FasterNet_NoGroup<numFeatures, SI, SW0, SW1, SB, SW2, SO>(
                perThreadWork,
                workOffset,
                numGroups,
                input,
                weights0,
                weights1,
                bias1,
                weights2,
                output
            );
            return;
        }
        else if(numGroups == 2)
        {
            FasterNet_Group2<numFeatures, SI, SW0, SW1, SB, SW2, SO>(
                perThreadWork,
                workOffset,
                numGroups,
                input,
                weights0,
                weights1,
                bias1,
                weights2,
                output
            );
            return;
        }

    }
    else if(numFeatures == 48)
    {
        FasterNet48_NoGroup<numFeatures, SI, SW0, SW1, SB, SW2, SO>(
                perThreadWork,
                workOffset,
                numGroups,
                input,
                weights0,
                weights1,
                bias1,
                weights2,
                output
            );
            return;
    }
}