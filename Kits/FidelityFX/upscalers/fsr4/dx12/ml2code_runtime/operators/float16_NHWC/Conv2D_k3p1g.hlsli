// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_uint8.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"
#include "ml2code_runtime/operators/templates/Conv2D.hlsli"

template<typename SI, typename SW, typename SO>
void InnerConv4_16_32(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    half accumulators[32];
    for (uint f = 0 ; f != 32; ++f)
        accumulators[f] = 0;

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            half4 vs[4];
            for (uint c = 0; c != weights.logicalSize.z; c += 4)
            {
                int3 ps = piBase + uint3(kx, ky, c);
                vs[c >> 2] = Load4hA(input, ps);
            }

            for (uint f = 0; f != numFeatures; ++f )
            {
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f);
                    const half4 ws = Load4hA(weights, pw);

                    accumulators[f] += dot(ws, vs[c >> 2]);
                }
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]));
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void InnerConv4_16_32(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    half accumulators[32];
    uint biasOffset = bias.OffsetOf(0);

    [unroll]
    for (uint f = 0 ; f != 32; f+=8)
    {
        const uint4 biasDwords = bias.storage.Load4(biasOffset);
        half4 b = Unpack4h(biasDwords.xy);

        accumulators[f]   = b.x;
        accumulators[f+1] = b.y;
        accumulators[f+2] = b.z;
        accumulators[f+3] = b.w;

        b = Unpack4h(biasDwords.zw);
        accumulators[f+4]   = b.x;
        accumulators[f+5] = b.y;
        accumulators[f+6] = b.z;
        accumulators[f+7] = b.w;

        biasOffset += 8*2;
    }

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            half4 vs[4];
            for (uint c = 0; c != weights.logicalSize.z; c += 4)
            {
                int3 ps = piBase + uint3(kx, ky, c);
                vs[c >> 2] = Load4hA(input, ps);
            }

            for (uint f = 0; f != numFeatures; ++f )
            {
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f);
                    const half4 ws = Load4hA(weights, pw);

                    accumulators[f] += dot(ws, vs[c >> 2]);
                }
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]));
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void InnerConv4_32_64(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    half accumulators[64];
    uint biasOffset = bias.OffsetOf(0);

    [unroll]
    for (uint f = 0 ; f != 64; f+=8)
    {
        const uint4 biasDwords = bias.storage.Load4(biasOffset);
        half4 b = Unpack4h(biasDwords.xy);

        accumulators[f]   = b.x;
        accumulators[f+1] = b.y;
        accumulators[f+2] = b.z;
        accumulators[f+3] = b.w;

        b = Unpack4h(biasDwords.zw);
        accumulators[f+4]   = b.x;
        accumulators[f+5] = b.y;
        accumulators[f+6] = b.z;
        accumulators[f+7] = b.w;

        biasOffset += 8*2;
    }

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            half4 vs[8];
            for (uint c = 0; c != weights.logicalSize.z; c += 4)
            {
                int3 ps = piBase + uint3(kx, ky, c);
                vs[c >> 2] = Load4hA(input, ps);
            }

            for (uint f = 0; f != numFeatures; ++f )
            {
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f);
                    const half4 ws = Load4hA(weights, pw);

                    accumulators[f] += dot(ws, vs[c >> 2]);
                }
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]));
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void InnerConv4_8_16(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    float accumulators[16];
    [unroll]
    for (uint f = 0 ; f != 16; f++)
    {
        accumulators[f] = 0;
    }

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
    {
        const uint columnReadOffset = input.OffsetOf(piBase) + ky * input.storageByteStrides.y;
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            const uint inputOffset = columnReadOffset + kx * input.storageByteStrides.x;

            half4 vs[2];
            const uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = Unpack4h(inputDwords.xy);
            vs[1] = Unpack4h(inputDwords.zw);

            for (uint f = 0; f != numFeatures; ++f )
            {
                const uint weightsOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                const uint4 weightDwords = weights.storage.Load4(weightsOffset);

                half4 ws = Unpack4h(weightDwords.xy);
                accumulators[f] = dot2add(ws.xy, vs[0].xy, accumulators[f]);
                accumulators[f] = dot2add(ws.zw, vs[0].zw, accumulators[f]);

                ws = Unpack4h(weightDwords.zw);
                accumulators[f] = dot2add(ws.xy, vs[1].xy, accumulators[f]);
                accumulators[f] = dot2add(ws.zw, vs[1].zw, accumulators[f]);
            }
        }
    }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]) + Load4hA(bias, po.z));
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void InnerConv4_10_16(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    float accumulators[16];
    [unroll]
    for (uint f = 0 ; f != 16; f++)
    {
        accumulators[f] = 0;
    }

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
    {
        const uint columnReadOffset = input.OffsetOf(piBase) + ky * input.storageByteStrides.y;
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            const uint inputOffset = columnReadOffset + kx * input.storageByteStrides.x;

            half4 vs[2];
            const uint4 inputDwords = input.storage.Load4(inputOffset);
            vs[0] = Unpack4h(inputDwords.xy);
            vs[1] = Unpack4h(inputDwords.zw);
            half2 last = Load2hA(input, piBase + int3(kx,ky,8));

            for (uint f = 0; f != numFeatures; ++f )
            {
                const uint weightsOffset = weights.OffsetOf(uint4(kx, ky, 0, f));
                const uint4 weightDwords = weights.storage.Load4(weightsOffset);

                half4 ws = Unpack4h(weightDwords.xy);
                accumulators[f] = dot2add(ws.xy, vs[0].xy, accumulators[f]);
                accumulators[f] = dot2add(ws.zw, vs[0].zw, accumulators[f]);

                ws = Unpack4h(weightDwords.zw);
                accumulators[f] = dot2add(ws.xy, vs[1].xy, accumulators[f]);
                accumulators[f] = dot2add(ws.zw, vs[1].zw, accumulators[f]);

                accumulators[f] = dot2add(Load2hA(weights, uint4(kx, ky, 8, f)), last, accumulators[f]);
            }
        }
    }

    [unroll]
    for (uint f = 0; f < numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]) + Load4hA(bias, po.z));
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2dDownsample_2x2(
    uint strideY, uint strideX,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    uint3 groupThreadId)
{
    /* TODO: Performance optimizations:

      1) dot2add needs to be incorporated here. The workflow makes it easy (might save registers)
         and would then pass around one fp32 value between threads
           - This may save on registers, which should help the wave64 version of this shader,
             which is bound more on register usage for occupancy than wave32.

      2) In the current setup, the compiler outputs 1 wave op for each fp16 value we share. In
         theory, it should be able to pack the fp16 values into 32bit registers, and reduce the #
         of wave ops by half, but I could not force it to do that without getting incorrect results
         in the output.
            - If we use dot2add, this may be a non issue. But something to keep track of

        ^ First experiments im running don't appear to make a difference in performance for wave32 or 64

      3) For wave64, you need to use WaveGetLaneId instead of the thread id from CS inputs to enable
         it. The wave ops also need to be changed from + 1, + 2, + 4, to ^ 1, ^ 2, ^ 4. This will
         use dpp over lds memory queue. Right now this is slower due to register usage, but
         in the future, this might be faster

      4) Right now each thread is actually doing very little work. This is good and fast, but the
         the shader may benefit from unrolling some work. So we process 2 (4,2,8) blocks in one
         threadgroup. This has some benefits:
            - Inputs can be preloaded outside of all the loops, so their influence on perf is really
              small
            - Weights are constantly reloaded across threadgroups. They are coherent, but this hurts
              VMEM queue. With a 2x unroll setup, we still use the exact same weights, so we reduce
              VMEM instrucitons from weights across the dispatch by half. ALU across the dispatch
              remains similar, so overall, proportion of high latency instructions go down. Should
              help with performance.

        ^ Initial experiments don't appear to make this any better.

      5) Using uints instead of ints for scalars can reduce amount of scalar instructions. In this case,
         im unsure how much this comes up, but if we load ints from constant buffers, it can matter.
         Not a big perf problem, but if we have instruction cache misses, might be useful.

      6) Right now all our loads are 64bit. Can we modify them to 128bit and process 8 elements per thread?

    */

    //#define WAVE_64

    // NOTE: Controls how many fp16 values we process per thread
    // IMPORTANT: This has implications on the number of threads, so the thread group size needs
    // to be scaled based on this value
    uint numZPerThread = 4;

    // NOTE: This is the num threads only for this function. We reinterpret the thread ids to have
    // the wave ops correctly share data between threads. We require threads to be nearby each other
    // in Z, and this is not true with the default HW swizzle
    // IMPORTANT: This value depends on numZPerThread. In theory, the Z value = input.z / numZPerThread
    // but then we need to scale the other sizes to fit it in 64 threads
    uint3 numThreads = uint3(4, 2, 8);
    if (input.logicalSize.z == 16)
    {
        numThreads = uint3(8, 2, 4);
    }

#ifdef WAVE_64
    uint ThreadIndex = WaveGetLaneIndex();
#else
    uint ThreadIndex = groupThreadId.x;
#endif
    uint3 ThreadId;
    {
        ThreadId.z = ThreadIndex % numThreads.z;
        ThreadId.x = (ThreadIndex / numThreads.z) % numThreads.x;
        ThreadId.y = (ThreadIndex / (numThreads.z * numThreads.x)) % numThreads.y;
    }

    // NOTE: Number of wave ops for adds is kind of like a binary search, where each wave op increases summed thread count by a
    // multiple of 2
    uint NumWaveOps = log2(numThreads.z & -numThreads.z);

    int3 PosInputBase = int3(output.threadGroupSliceStart.xy, 0) + ThreadId;
    PosInputBase.z *= numZPerThread; // Account for how much loading we do in each thread
    int2 PosOutputBase = PosInputBase.xy;
    PosInputBase.xy *= int2(strideX, strideY);

    if (!(all(PosOutputBase >= 0) && all(PosOutputBase < output.logicalSize.xy)))
    {
        return;
    }

    // NOTE: Inputs are the same for all below iterations, so we cache them here
    half4 ValueInputs[2*2]; // TODO: This should be KernelX * KernelY
    {
        ValueInputs[0] = Load4hA(input, PosInputBase + int3(0, 0, 0));
        ValueInputs[1] = Load4hA(input, PosInputBase + int3(1, 0, 0));
        ValueInputs[2] = Load4hA(input, PosInputBase + int3(0, 1, 0));
        ValueInputs[3] = Load4hA(input, PosInputBase + int3(1, 1, 0));
    }

    // NOTE: UNROLL_NUM stores the number of W values we compute in each outer loop
#define UNROLL_NUM 8
    int NumOuterIterations = weights.logicalSize.w / UNROLL_NUM;
    for (int OuterFeatureId = 0; OuterFeatureId < NumOuterIterations; ++OuterFeatureId)
    {
        // IMPORTANT: UNROLL_NUM must be a multiple of numZPerThread, since we only ever work with
        // groups of 4 fp16 values at a time
        // NOTE: We store half4's to store 4 Z values in the thread, but we store UNROLL_NUM number
        // of accumulators, since each represents the accumulated result for a different feature/W value
        half4 Accumulator[UNROLL_NUM];

        [unroll]
        for (int UnRollId = 0; UnRollId < UNROLL_NUM; ++UnRollId)
        {
            Accumulator[UnRollId] = 0;
        }

        // NOTE: Here we calculate the accumulated value for every Z value in a half4 (we do not
        // merge these values yet). We also do this for every feature we are calulating in
        // UNROLL_NUm
        for (int InnerFeatureId = 0; InnerFeatureId < UNROLL_NUM; ++InnerFeatureId)
        {
            int FeatureId = OuterFeatureId * UNROLL_NUM + InnerFeatureId;

            for (int KernelY = 0; KernelY != weights.logicalSize.y; ++KernelY)
            {
                for (int KernelX = 0; KernelX != weights.logicalSize.x; ++KernelX)
                {
                    int4 PosWeight = int4(KernelX, KernelY, PosInputBase.z, FeatureId);

                    // NOTE: These loads across the threadgroup will load entire blocks of Z from
                    // the tensor. Since tensors are swizzled first by Z, this will be a completely
                    // coherent load, with as much of the cache line used as we can feasibly do
                    half4 ValueWeights = Load4hA(weights, PosWeight);

                    Accumulator[InnerFeatureId] += ValueWeights * ValueInputs[KernelY * 2 + KernelX];
                }
            }
        }

        // NOTE: Here we accumulate our results inside the thread, and then across threads, so
        // that we accumulate across the entire Z axis. Our thread ids are swizzled to make this
        // work. Accumulated value will be stored in thread id with Z == 0 (all other threads will
        // contain garbage)
        [unroll]
        for (int UnRollId = 0; UnRollId < UNROLL_NUM; ++UnRollId)
        {
            // NOTE: We first accumulate the value for the thread acorss z values (half4 stores 4
            // z values, so we accumulate these into 1).
            half2 Accumulator2 = Accumulator[UnRollId].xy + Accumulator[UnRollId].zw;
            Accumulator[UnRollId].x = Accumulator2.x + Accumulator2.y;

            // NOTE: We accumulate our convolutions across threads with different Z values.
            // Since our tensor is 32 in size, and we process 4 z values per thread, we have 8
            // threads with different z values we need to accumultae between. Accumulated value
            // will be stored in thread with Z value == 0.
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

        if (ThreadId.z == 0)
        {
            // NOTE: Threads that store correct accumulated results will now write them to memory.
            // Since UNROLL_NUM is a multiple of 4, we will be storing 4 values at a time. These
            // stores are less effective (most threads are inactive), but they cost very little of
            // the final execution time
            for (int UnRollId = 0; UnRollId < (UNROLL_NUM / numZPerThread); ++UnRollId)
            {
                int StartFeatureId = OuterFeatureId * UNROLL_NUM + UnRollId * numZPerThread;
                int3 PosOutput = int3(PosOutputBase, StartFeatureId);
                // NOTE: We merge our different feature results into a coherent vector, since
                // the Z axis of the output stores results of different features (W values)
                half4 WriteResult = half4(Accumulator[4*UnRollId + 0].x,
                                          Accumulator[4*UnRollId + 1].x,
                                          Accumulator[4*UnRollId + 2].x,
                                          Accumulator[4*UnRollId + 3].x);

                // NOTE: We apply bias here. Its not perfect, but also not a significant part of the
                // frame cost, so we don't really lose much here
                WriteResult += Load4hA(bias, StartFeatureId);
                Store4hA(output, PosOutput, WriteResult);
            }
        }
    }

    #undef UNROLL_NUM
    #undef WAVE_64
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2dDownsample_2x2_UnRolled2x(
    uint strideY, uint strideX,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    uint3 groupThreadId)
{
    /*
        This version of the shader runs 2 blocks of pixels in X. This halves the number of thread groups by half. I also have
        code here to halve the number of wave ops we run. So far this version is unused, because performance was not really any
        different.
    */

    //#define WAVE_64
    #define HALF_WAVE_OPS
    #define REGULAR_DOT_ACCUMULATE

    // NOTE: Controls how many fp16 values we process per thread
    // IMPORTANT: This has implications on the number of threads, so the thread group size needs
    // to be scaled based on this value
    uint numZPerThread = 4;

    // NOTE: This is the num threads only for this function. We reinterpret the thread ids to have
    // the wave ops correctly share data between threads. We require threads to be nearby each other
    // in Z, and this is not true with the default HW swizzle
    // IMPORTANT: This value depends on numZPerThread. In theory, the Z value = input.z / numZPerThread
    // but then we need to scale the other sizes to fit it in 64 threads
    uint3 numThreads = uint3(4, 2, 8);
    if (input.logicalSize.z == 16)
    {
        numThreads = uint3(4, 4, 4);
    }

#ifdef WAVE_64
    uint ThreadIndex = WaveGetLaneIndex();
#else
    uint ThreadIndex = groupThreadId.x;
#endif
    uint3 ThreadId;
    {
        ThreadId.z = ThreadIndex % numThreads.z;
        ThreadId.x = (ThreadIndex / numThreads.z) % numThreads.x;
        ThreadId.y = (ThreadIndex / (numThreads.z * numThreads.x)) % numThreads.y;
    }

    // NOTE: Number of wave ops for adds is kind of like a binary search, where each wave op increases summed thread count by a
    // multiple of 2
    uint NumWaveOps = log2(numThreads.z & -numThreads.z);

    int3 PosInputBase = int3(output.threadGroupSliceStart.xy, 0) + ThreadId;
    PosInputBase.z *= numZPerThread; // Account for how much loading we do in each thread
    int2 PosOutputBase = PosInputBase.xy;
    PosInputBase.xy *= int2(strideX, strideY);

    if (!(all(PosOutputBase >= 0) && all(PosOutputBase < output.logicalSize.xy)))
    {
        return;
    }

#define NUM_BLOCKS 1
    // NOTE: Inputs are the same for all below iterations, so we cache them here
    half4 ValueInputs[NUM_BLOCKS][2*2]; // TODO: This should be KernelX * KernelY
    {
        [unroll]
        for (int BlockId = 0; BlockId < NUM_BLOCKS; ++BlockId)
        {
            ValueInputs[BlockId][0] = Load4hA(input, PosInputBase + int3(0, 0, 0) + BlockId * weights.logicalSize.x * int3(numThreads.x, 0, 0));
            ValueInputs[BlockId][1] = Load4hA(input, PosInputBase + int3(1, 0, 0) + BlockId * weights.logicalSize.x * int3(numThreads.x, 0, 0));
            ValueInputs[BlockId][2] = Load4hA(input, PosInputBase + int3(0, 1, 0) + BlockId * weights.logicalSize.x * int3(numThreads.x, 0, 0));
            ValueInputs[BlockId][3] = Load4hA(input, PosInputBase + int3(1, 1, 0) + BlockId * weights.logicalSize.x * int3(numThreads.x, 0, 0));
        }
    }

    // NOTE: UNROLL_NUM stores the number of W values we compute in each outer loop
#define UNROLL_NUM 8
    int NumOuterIterations = weights.logicalSize.w / UNROLL_NUM;
    for (int OuterFeatureId = 0; OuterFeatureId < NumOuterIterations; ++OuterFeatureId)
    {
        // IMPORTANT: UNROLL_NUM must be a multiple of numZPerThread, since we only ever work with
        // groups of 4 fp16 values at a time
        // NOTE: We store half4's to store 4 Z values in the thread, but we store UNROLL_NUM number
        // of accumulators, since each represents the accumulated result for a different feature/W value
        half4 Accumulator[NUM_BLOCKS][UNROLL_NUM];

        [unroll]
        for (int UnRollId = 0; UnRollId < UNROLL_NUM; ++UnRollId)
        {
            [unroll]
            for (int BlockId = 0; BlockId < NUM_BLOCKS; ++BlockId)
            {
                Accumulator[BlockId][UnRollId] = 0;
            }
        }

        // NOTE: Here we calculate the accumulated value for every Z value in a half4 (we do not
        // merge these values yet). We also do this for every feature we are calulating in
        // UNROLL_NUM
        for (int InnerFeatureId = 0; InnerFeatureId < UNROLL_NUM; ++InnerFeatureId)
        {
            int FeatureId = OuterFeatureId * UNROLL_NUM + InnerFeatureId;

            for (int KernelY = 0; KernelY != weights.logicalSize.y; ++KernelY)
            {
                for (int KernelX = 0; KernelX != weights.logicalSize.x; ++KernelX)
                {
                    int4 PosWeight = int4(KernelX, KernelY, PosInputBase.z, FeatureId);

                    // NOTE: These loads across the threadgroup will load entire blocks of Z from
                    // the tensor. Since tensors are swizzled first by Z, this will be a completely
                    // coherent load, with as much of the cache line used as we can feasibly do
                    half4 ValueWeights = Load4hA(weights, PosWeight);

                    [unroll]
                    for (int BlockId = 0; BlockId < NUM_BLOCKS; ++BlockId)
                    {
                        Accumulator[BlockId][InnerFeatureId] += ValueWeights * ValueInputs[BlockId][KernelY * 2 + KernelX];
                    }
                }
            }
        }

        // NOTE: Here we accumulate our results inside the thread, and then across threads, so
        // that we accumulate across the entire Z axis. Our thread ids are swizzled to make this
        // work. Accumulated value will be stored in thread id with Z == 0 (all other threads will
        // contain garbage)
#ifndef HALF_WAVE_OPS

        [unroll]
        for (int UnRollId = 0; UnRollId < UNROLL_NUM; ++UnRollId)
        {
            [unroll]
            for (int BlockId = 0; BlockId < NUM_BLOCKS; ++BlockId)
            {
                // NOTE: We first accumulate the value for the thread acorss z values (half4 stores 4
                // z values, so we accumulate these into 1).
                half2 Accumulator2 = Accumulator[BlockId][UnRollId].xy + Accumulator[BlockId][UnRollId].zw;
                Accumulator[BlockId][UnRollId].x = Accumulator2.x + Accumulator2.y;

                // NOTE: We accumulate our convolutions across threads with different Z values.
                // Since our tensor is 32 in size, and we process 4 z values per thread, we have 8
                // threads with different z values we need to accumultae between. Accumulated value
                // will be stored in thread with Z value == 0.
                [unroll]
                for (uint WaveOpId = 0; WaveOpId < NumWaveOps; ++WaveOpId)
                {
#ifdef WAVE_64
                    Accumulator[BlockId][UnRollId].x += WaveReadLaneAt(Accumulator[BlockId][UnRollId].x, (ThreadIndex ^ (1 << WaveOpId)));
#else
                    Accumulator[BlockId][UnRollId].x += WaveReadLaneAt(Accumulator[BlockId][UnRollId].x, (ThreadIndex + (1 << WaveOpId)));
#endif
                }
            }
        }

#else

        // NOTE: Accumulate into half2 vectors
        half2 WaveAccumulator[NUM_BLOCKS][UNROLL_NUM / 2];
        [unroll]
        for (int UnRollId = 0; UnRollId < UNROLL_NUM / 2; ++UnRollId)
        {
            [unroll]
            for (int BlockId = 0; BlockId < NUM_BLOCKS; ++BlockId)
            {
                // NOTE: We first accumulate the value for the thread acorss z values (half4 stores 4
                // z values, so we accumulate these into 1).
#ifndef REGULAR_DOT_ACCUMULATE
                {
                    half2 Accumulator2 = Accumulator[BlockId][2*UnRollId + 0].xy + Accumulator[BlockId][2*UnRollId + 0].zw;
                    Accumulator[BlockId][2*UnRollId + 0].x = Accumulator2.x + Accumulator2.y;
                }
                {
                    half2 Accumulator2 = Accumulator[BlockId][2*UnRollId + 1].xy + Accumulator[BlockId][2*UnRollId + 1].zw;
                    Accumulator[BlockId][2*UnRollId + 1].x = Accumulator2.x + Accumulator2.y;
                }

                WaveAccumulator[BlockId][UnRollId] = half2(Accumulator[BlockId][2*UnRollId + 0].x, Accumulator[BlockId][2*UnRollId + 1].x);
#else
                WaveAccumulator[BlockId][UnRollId] = half2(dot(Accumulator[BlockId][2*UnRollId + 0], half4(1, 1, 1, 1)),
                                                           dot(Accumulator[BlockId][2*UnRollId + 1], half4(1, 1, 1, 1)));
#endif

                // NOTE: We accumulate our convolutions across threads with different Z values.
                // Since our tensor is 32 in size, and we process 4 z values per thread, we have 8
                // threads with different z values we need to accumultae between. Accumulated value
                // will be stored in thread with Z value == 0.
                [unroll]
                for (uint WaveOpId = 0; WaveOpId < NumWaveOps; ++WaveOpId)
                {
                    uint PackedAccumulator = Pack2h(WaveAccumulator[BlockId][UnRollId]);

#ifdef WAVE_64
                    uint NearbyPackedAccumulator = WaveReadLaneAt(PackedAccumulator, (ThreadIndex ^ (1 << WaveOpId)));
#else
                    int NearbyPackedAccumulator = WaveReadLaneAt(PackedAccumulator, (ThreadIndex + (1 << WaveOpId)));
#endif

                    WaveAccumulator[BlockId][UnRollId] = Unpack2h(PackedAccumulator) + Unpack2h(NearbyPackedAccumulator);
                }
            }
        }

#endif

        if (ThreadId.z == 0)
        {
            // NOTE: Threads that store correct accumulated results will now write them to memory.
            // Since UNROLL_NUM is a multiple of 4, we will be storing 4 values at a time. These
            // stores are less effective (most threads are inactive), but they cost very little of
            // the final execution time
            for (int UnRollId = 0; UnRollId < (UNROLL_NUM / numZPerThread); ++UnRollId)
            {
                int StartFeatureId = OuterFeatureId * UNROLL_NUM + UnRollId * numZPerThread;
                int3 PosOutput = int3(PosOutputBase, StartFeatureId);
                half4 Bias = Load4hA(bias, StartFeatureId);

                [unroll]
                for (int BlockId = 0; BlockId < NUM_BLOCKS; ++BlockId)
                {
                    // NOTE: We merge our different feature results into a coherent vector, since
                    // the Z axis of the output stores results of different features (W values)
#ifndef HALF_WAVE_OPS
                    half4 WriteResult = half4(Accumulator[BlockId][4*UnRollId + 0].x,
                                              Accumulator[BlockId][4*UnRollId + 1].x,
                                              Accumulator[BlockId][4*UnRollId + 2].x,
                                              Accumulator[BlockId][4*UnRollId + 3].x);
#else
                    half4 WriteResult = half4(WaveAccumulator[BlockId][2*UnRollId + 0],
                                              WaveAccumulator[BlockId][2*UnRollId + 1]);
#endif

                    // NOTE: We apply bias here. Its not perfect, but also not a significant part of the
                    // frame cost, so we don't really lose much here
                    WriteResult += Bias;
                    Store4hA(output, PosOutput + BlockId * int3(numThreads.x, 0, 0), WriteResult);
                }
            }
        }
    }

    #undef HALF_WAVE_OPS
    #undef UNROLL_NUM
    #undef NUM_BLOCKS
    #undef WAVE_64
}

template<typename SI, typename SW, typename SO>
void InnerConv4(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    if (input.logicalSize.z == 16 && numFeatures == 32)
        return InnerConv4_16_32(piBase, poBase, numFeatures, input, weights, output);

    half4 accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = 0;

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
                accumulator[f % 4] += PipelinedDot(input.OffsetOf(piBase + uint3(kx, ky, 0)), input, weights.OffsetOf(uint4(kx, ky, 0, f)), weights, weights.logicalSize.z);

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, half4(accumulator[0], accumulator[1], accumulator[2], accumulator[3]));
        }
    }
}


template<typename SI, typename SW, typename SO>
void InnerConv4Padded_16_32(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    half accumulators[32];
    for (uint f = 0 ; f != 32; ++f)
        accumulators[f] = 0;

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                continue;

            half4 vs[4];
            for (uint c = 0; c != weights.logicalSize.z; c += 4)
            {
                int3 ps = piBase + uint3(kx, ky, c);
                vs[c >> 2] = Load4hA(input, ps);
            }

            for (uint f = 0; f != numFeatures; ++f )
            {
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f);
                    const half4 ws = Load4hA(weights, pw);

                    accumulators[f] += dot(ws, vs[c >> 2]);
                }
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]));
    }
}


template<typename SI, typename SW, typename SO>
void InnerConv4Padded(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    if (input.logicalSize.z == 16 && numFeatures == 32)
        return InnerConv4Padded_16_32(piBase, poBase, numFeatures, input, weights, output);

    half4 accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = 0;

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
            {
                if (ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                {
                    [unroll]
                    for (uint c = 0; c < weights.logicalSize.z; c += 4)
                    {
                        const uint4 pw = uint4(kx, ky, c, f); // position in weights
                        int3 ps = piBase + pw.xyz;

                        const half4 ws = Load4hA(weights, pw);
                        const half4 vs = Load4hA(input, ps);

                        accumulator[f % 4] += dot(ws, vs);
                    }
                }
            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, accumulator);
        }
    }
}

template<typename SI, typename SW, typename SO>
void InnerConv4PaddedGrouped_32_32(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    if (numGroups != 2)
        return;

    half accumulators[32];
    for (uint f = 0 ; f != 32; ++f)
        accumulators[f] = 0;

    [unroll]
    for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
        {
            if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                continue;

            half4 vs[8];
            for (uint c = 0; c != input.logicalSize.z; c += 4)
            {
                int3 ps = piBase + uint3(kx, ky, c);
                vs[c >> 2] = Load4hA(input, ps);
            }

            half4 ws[4];

            uint vsOffset = 0;
            for (uint f = 0; f != numFeatures; ++f)
            {
                if (f == numFeatures / numGroups)
                    vsOffset = weights.logicalSize.z;

                // Load all the weights for this channel
                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f);
                    ws[c >> 2] = Load4hA(weights, pw);
                }

                for (uint c = 0; c != weights.logicalSize.z; c += 4)
                    accumulators[f] += dot(ws[c >> 2], vs[(c + vsOffset) >> 2]);
            }
        }

    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        int3 po = poBase;
        po.z += f;

        Store4hA(output, po, half4(accumulators[f], accumulators[f+1], accumulators[f+2], accumulators[f+3]));
    }
}


template<typename SI, typename SW, typename SO>
void InnerConv4PaddedGroups(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output)
{
    if (input.logicalSize.z == 32 && numFeatures == 32)
        return InnerConv4PaddedGrouped_32_32(piBase, poBase, numFeatures, numGroups, input, weights, output);

    half4 accumulator;
    int3 pi = piBase;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = 0;

        // Assumes groups=2
        if (f == numFeatures / 2)
            pi = piBase + int3(0,0,weights.logicalSize.z);

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
            {
                if (ValidPosition(input, int3(pi.xy + int2(kx, ky), 0)))
                {
                    [unroll]
                    for (uint c = 0; c != weights.logicalSize.z; c += 4)
                    {
                        const uint4 pw = uint4(kx, ky, c, f); // position in weights
                        int3 ps = pi + pw.xyz;

                        const half4 ws = Load4hA(weights, pw);
                        const half4 vs = Load4hA(input, ps);

                        accumulator[f % 4] += dot(ws, vs);
                    }
                }


            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, accumulator);
        }
    }
}

template<typename SI, typename SW, typename SB, typename SO>
void InnerConv4(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    if (input.logicalSize.z == 8 && numFeatures == 16)
        return InnerConv4_8_16(piBase, poBase, numFeatures, input, weights, bias, output);

    if (input.logicalSize.z == 10 && numFeatures == 16)
        return InnerConv4_10_16(piBase, poBase, numFeatures, input, weights, bias, output);

    if (input.logicalSize.z == 16 && numFeatures == 32)
        return InnerConv4_16_32(piBase, poBase, numFeatures, input, weights, bias, output);

    if (input.logicalSize.z == 32 && numFeatures == 64)
        return InnerConv4_32_64(piBase, poBase, numFeatures, input, weights, bias, output);

    half4 accumulator;
    const uint baseReadOffset = input.OffsetOf(piBase);

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = Load4hA(bias, f);

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
        {
            const uint columnReadOffset = baseReadOffset + ky * input.storageByteStrides.y;

            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
            {
                const uint rowReadOffset = columnReadOffset + kx * input.storageByteStrides.x;

                accumulator[f % 4] += PipelinedDot(rowReadOffset, input, weights.OffsetOf(uint4(kx, ky, 0, f)), weights, weights.storageSize.z);
            }
        }

        f++;
        if (f && !(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, accumulator);
        }
    }
}


template<typename SI, typename SW, typename SB, typename SO>
void InnerConv4Padded(
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output)
{
    half4 accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = Load4hA(bias, f);

        [unroll]
        for (uint ky = 0; ky != weights.logicalSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != weights.logicalSize.x; ++kx)
            {
                int3 psBase = int3(piBase.xy + uint2(kx, ky), 0);
                if (ValidPosition(input, psBase, false))
                    accumulator[f % 4] += PipelinedDot(input.OffsetOf(psBase), input, weights.OffsetOf(uint4(kx, ky, 0, f)), weights, weights.logicalSize.z);
            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            Store4hA(output, po, accumulator);
        }
    }
}

template<typename SI, typename SW, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output,
    const uint3 numThreads,
    const uint3 threadIndex)
{
    // each thread produces all channels
    if (numThreads.z != 1)
        return;

    // All channels are present in input
    if (input.threadGroupSliceSize.z != weights.logicalSize.z)
        return;

    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + threadIndex * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);
    const uint numFeatures = weights.logicalSize.w;

    // const bool needGlobalBounds = any(or(workOffset.xy - int2(beginX, beginY) < 0,
    //     workOffset.xy + output.threadGroupSliceSize.xy - int2(beginX, beginY) + weights.logicalSize.xy > output.logicalSize.xy));
    const bool needGlobalBounds = true;

    if (!IsAligned(numFeatures, 4))
        return;

    // Padded Case
    if (beginX != 0 || beginY != 0)
    {
        if (needGlobalBounds)
        {
            for (uint y = 0 ; y < perThreadWork.y; y++)
                for (uint x = 0 ; x < perThreadWork.x; x++)
                {
                    const int3 poBase = workOffset + int3(x, y, 0);
                    if (!ValidPosition(output, poBase, false, needGlobalBounds))
                        continue;

                    const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);
                    InnerConv4Padded(piBase, poBase, numFeatures, input, weights, output);
                }
        }
        else
        {
            for (uint y = 0 ; y < perThreadWork.y; y++)
                for (uint x = 0 ; x < perThreadWork.x; x++)
                {
                    const int3 poBase = workOffset + int3(x, y, 0);
                    if (!ValidPosition(output, poBase, false, needGlobalBounds))
                        continue;

                    const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                    InnerConv4(piBase, poBase, numFeatures, input, weights, output);
                }
        }


    }
    else
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    InnerConv4(piBase, poBase, numFeatures, input, weights, output);
            }
    }
}


template<typename SI, typename SW, typename SB, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const uint3 numThreads,
    const uint3 threadIndex)
{
#if 1
    // NOTE: This will use this code path for both downsamples. Right now its not actually faster, so I'm not using it.
    //if (input.logicalSize.z == 32 || input.logicalSize.z == 16)
    if (input.logicalSize.z == 32 && output.logicalSize.z == 32)
    {
        Conv2dDownsample_2x2_UnRolled2x(strideY,
                            strideX,
                            input,
                            weights,
                            bias,
                            output,
                            threadIndex);
        return;
    }
#endif

    // each thread produces all channels
    if (numThreads.z != 1)
        return;

    // All channels are present in input
    if (input.threadGroupSliceSize.z != weights.logicalSize.z)
        return;

    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + threadIndex * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);
    const uint numFeatures = weights.logicalSize.w;

    const bool needSliceBounds = NeedSliceBounds(perThreadWork, numThreads, output.threadGroupSliceSize);
    const bool needGlobalBounds = NeedGlobalBounds(output.threadGroupSliceStart, output.threadGroupSliceSize, output.logicalSize);

    // Padded Case
    if (beginX != 0 || beginY != 0)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase, needSliceBounds, needGlobalBounds))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    InnerConv4Padded(piBase, poBase, numFeatures, input, weights, bias, output);
            }
    }
    else
    // Unpadded Case
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!ValidPosition(output, poBase, needSliceBounds, needGlobalBounds))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    InnerConv4(piBase, poBase, numFeatures, input, weights, bias, output);
            }
    }
}

template<typename SI, typename SW, typename SO>
void Conv2D_k3p1g(
    uint numGroups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor3h_NHWC<SO> output,
    const uint3 numThreads,
    const uint3 threadIndex)
{
    // each thread produces all channels
    if (numThreads.z != 1)
        return;

    if (numGroups != 2)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, output.threadGroupSliceSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + threadIndex * perThreadWork;
    const uint2 convStrides = 1;
    const uint numFeatures = weights.logicalSize.w;

    if (!IsAligned(numFeatures, 4))
        return;


    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!ValidPosition(output, poBase))
                continue;

            const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(1, 1, 0);

            InnerConv4PaddedGroups(piBase, poBase, numFeatures, numGroups, input, weights, output);
        }
}


template<typename SI, typename SW, typename SB, typename SO>
void Conv2D(
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint dilationY, uint dilationX,
    uint strideY, uint strideX,
    uint groups,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW> weights,
    const QuantizedTensor1i32<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (dilationX != 1 || dilationY != 1)
        return; // not implemented

    struct Algorithm {
        half biasScale;
        half inputScale;
        half weightsScale;

        uint4 kernelSize;
        QuantizedTensor3i8_NHWC<SI> input;
        QuantizedTensor4i8_NHWC<SW> weights;
        QuantizedTensor1i32<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4i32(bias, p) * biasScale; }
        half4 LoadInput4(int3 p) { return unpack_s8s16(Load4i8A(input, p)) * inputScale; }
        half4 LoadWeight4(int4 p) { return unpack_s8s16(Load4i8A(weights, p)) * weightsScale;  }
        half Dot(half4 ws, half4 vs, half acc) { return dot(ws, vs) + acc; }
        void StoreOutput4(int3 p, half4 vs) { Store4hA(output, p, vs); }
    };

    Algorithm algorithm = {
        bias.quantizationScale,
        input.quantizationScale,
        weights.quantizationScale,
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedGroupedConv2D_3413_NHWC_44<half4, half4, half4>(algorithm, beginY, beginX, endY, endX, strideY, strideX, groups, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams.numThreads, computeShaderParams.groupThreadID);
}

template<typename SI, typename SW, typename SB, typename SO>
void Conv2D_k3p1g(
    uint groups,
    const Tensor3h_NHWC<SI> input,
    const Tensor4h_NHWC<SW> weights,
    const Tensor1h<SB> bias,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    struct Algorithm {
        uint4 kernelSize;
        Tensor3h_NHWC<SI> input;
        Tensor4h_NHWC<SW> weights;
        Tensor1h<SB> bias;
        Tensor3h_NHWC<SO> output;

        bool ValidInput(int3 p) { return ValidPosition(input, p); }
        bool ValidOutput(int3 p) { return ValidPosition(output, p); }

        half4 LoadBias4(int p) { return Load4hA(bias, p) ; }
        half4 LoadInput4(int3 p) { return Load4hA(input, p); }
        half4 LoadWeight4(int4 p) { return Load4hA(weights, p);  }

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
        weights.logicalSize,
        input, weights, bias, output
    };

    TemplatedGroupedConv2D_3413_NHWC_44<half4, half4, half4>(algorithm, 1, 1, 1, 1, 1, 1, groups, output.threadGroupSliceStart, output.threadGroupSliceSize,
        computeShaderParams.numThreads, computeShaderParams.groupThreadID);
}