// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"

// Optimize version doesn't produce correct results when testing the model, commenting out for now

// template<typename T, typename T2, typename T3>
// void PipelinedAdd(const uint start, uint count, T a, T2 b, T3 output)
// {
//     // Warning: this code assumes a, b, and output all have the same size and stride
//     uint offset = start;

//     [unroll]
//     for (int i = 0; i != count; )
//     {
//         const int left = count - i;

// #if 1
//         if (left >= 8)
//         {
//             uint4 dwordsA = a.storage.Load4(offset);
//             uint4 dwordsB = b.storage.Load4(offset);
//             uint4 result;
//             result.x = Pack2h(Unpack2h(dwordsA.x) + Unpack2h(dwordsB.x));
//             result.y = Pack2h(Unpack2h(dwordsA.y) + Unpack2h(dwordsB.y));
//             result.z = Pack2h(Unpack2h(dwordsA.z) + Unpack2h(dwordsB.z));
//             result.w = Pack2h(Unpack2h(dwordsA.w) + Unpack2h(dwordsB.w));
//             output.storage.Store4(offset, result);
//             offset += 16;
//             i += 8;
//         }
//         else if (left == 4)
//         {
//             uint2 dwordsA = a.storage.Load2(offset);
//             uint2 dwordsB = b.storage.Load2(offset);
//             uint2 result;
//             result.x = Pack2h(Unpack2h(dwordsA.x) + Unpack2h(dwordsB.x));
//             result.y = Pack2h(Unpack2h(dwordsA.y) + Unpack2h(dwordsB.y));
//             output.storage.Store2(offset, result);
//             offset += 8;
//             i += 4;
//         }
//         else if (left == 2)
//         {
//             uint dwordA = a.storage.Load1(offset);
//             uint dwordB = b.storage.Load1(offset);
//             uint result = Pack2h(Unpack2h(dwordA) + Unpack2h(dwordB.x));
//             output.storage.Store1(offset, result);
//             offset += 4;
//             i += 2;
//         }

// #else
//         const int3 po = start + int3(0, 0, i);

//         if (left >= 4)
//         {
//             Store4hA(output, po, Load4hA(a, po) + Load4hA(b, po));
//             i += 4;
//         }
//         else if (left == 2)
//         {
//             Store2hA(output, po, Load2hA(a, po) + Load2hA(b, po));
//             i += 2;
//         }
// #endif
//     }
// }

// template<typename SA, typename SB, typename SO>
// void Add(
//     const Tensor3h_NHWC<SA> a,
//     const Tensor3h_NHWC<SB> b,
//     const Tensor3h_NHWC<SO> output,
//     const uint3 numThreads,
//     const uint3 threadIndex)
// {
//     const uint3 workPerTick = uint3(1, 1, 2);
//     const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, numThreads, workPerTick);
//     const uint3 threadWorkStart = threadIndex * perThreadWork;
//     const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

//     if (any(threadWorkStart >= output.threadGroupSliceSize))
//         return;

//     for (int y = 0; y < perThreadWork.y; y++)
//         for (int x = 0; x < perThreadWork.x; x++)
//             PipelinedAdd(output.OffsetOf(workOffset + int3(x, y, 0)), perThreadWork.z, a, b, output);
// }

template <typename SA, typename SB, typename SO>
void PipelinedAdd(const int3 start, uint count, const Tensor3h_NHWC<SA> a, const Tensor3h_NHWC<SB> b, Tensor3h_NHWC<SO> output)
{
    [unroll]
    for (int i = 0; i < count; )
    {
        const int3 po = start + int3(0, 0, i);

        const int left = count - i;
        if (left >= 4)
        {
            Store4hA(output, po, Load4hA(a, po) + Load4hA(b, po));
            i += 4;
        }
        else if (left == 2)
        {
            Store2hA(output, po, Load2hA(a, po) + Load2hA(b, po));
            i += 2;
        }
        else
            return;
    }
}

template <typename SA, typename SB, typename SO>
void PipelinedAdd(const int3 start, uint count, const Tensor3h_NHWC<SA> a, const QuantizedTensor3i8_NHWC<SB> b, Tensor3h_NHWC<SO> output)
{
    [unroll]
    for (int i = 0; i < count; )
    {
        const int3 po = start + int3(0, 0, i);

        const int left = count - i;
        if (left >= 4)
        {
            Store4hA(output, po, Load4hA(a, po) + unpack_s8s16(Load4i8A(b, po)) * b.quantizationScale);
            i += 4;
        }
        else
            return;
    }
}

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor3h_NHWC<SA> a,
    const Tensor1h<SB> b,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize) || any(threadWorkStart >= output.logicalSize.xyz))
        return;

    const half addend = Load2hA(b, 0).x;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
        {
            if (!ValidPosition(output, workOffset + int3(x, y, 0)))
                continue;
            const uint count = output.logicalSize.z;

            [unroll]
            for (int i = 0; i < count; )
            {
                const int3 po = workOffset + int3(x, y, i);

                const int left = count - i;
                if (left >= 4)
                {
                    Store4hA(output, po, Load4hA(a, po) + addend);
                    i += 4;
                }
                else if (left == 2)
                {
                    Store2hA(output, po, Load2hA(a, po) + addend);
                    i += 2;
                }
                else
                    return;
            }

        }
}

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor1h<SA> a,
    const Tensor3h_NHWC<SB> b,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    const half addend = Load2hA(a, 0).x;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
        {
            const uint count = output.storageSize.z;

            [unroll]
            for (int i = 0; i < count; )
            {
                const int3 po = workOffset + int3(x, y, i);

                const int left = count - i;
                if (left >= 4)
                {
                    Store4hA(output, po, Load4hA(b, po) + addend);
                    i += 4;
                }
                else if (left == 2)
                {
                    Store2hA(output, po, Load2hA(b, po) + addend);
                    i += 2;
                }
                else
                    return;
            }
        }
}

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor3h_NHWC<SA> a,
    const Tensor3h_NHWC<SB> b,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
            PipelinedAdd(workOffset + int3(x, y, 0), perThreadWork.z, a, b, output);
}

template<typename SA, typename SB, typename SO>
void Add(
    const Tensor3h_NHWC<SA> a,
    const QuantizedTensor3i8_NHWC<SB> b,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 2);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
            PipelinedAdd(workOffset + int3(x, y, 0), perThreadWork.z, a, b, output);
}