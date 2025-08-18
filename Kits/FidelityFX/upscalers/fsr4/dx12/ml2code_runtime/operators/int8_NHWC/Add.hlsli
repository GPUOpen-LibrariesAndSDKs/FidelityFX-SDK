// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

template<typename SA, typename SB, typename SO>
void PipelinedAdd(
    const int3 start,
    const uint numChannels,
    const Tensor3h_NHWC<SA> a,
    const Tensor3h_NHWC<SB> b,
    const QuantizedTensor3i8_NHWC<SO> output)
{
    const float rcpScale = 1.0 / output.quantizationScale;
    const uint aOffset = a.OffsetOf(start);
    const uint bOffset = b.OffsetOf(start);
    const uint writeOffset = output.OffsetOf(start);

    [unroll]
    for (int c = 0; c != numChannels; )
    {
        const int3 po = start + int3(0, 0, c);

        const int remainder = numChannels - c;
        if (remainder >= 8)
        {
            const uint4 packedAs = a.storage.Load4(aOffset + c * 2);
            const uint4 packedBs = b.storage.Load4(bOffset + c * 2);

            const int8_t4_packed2 packedI8s = int8_t4_packed2(
                pack_clamp_s8(int4(round((Unpack4h(packedAs.xy) + Unpack4h(packedBs.xy))* rcpScale))),
                pack_clamp_s8(int4(round((Unpack4h(packedAs.zw) + Unpack4h(packedBs.zw))* rcpScale)))
            );

            output.storage.Store2(writeOffset + c * 1, packedI8s);

            c += 8;
        }
        // else 
        // if (remainder >= 4)
        // {
        //     const uint2 packedAs = a.storage.Load2(aOffset + c * 2);
        //     const uint2 packedBs = b.storage.Load2(bOffset + c * 2);

        //     const int8_t4_packed packedI8 = pack_clamp_s8(int4(round((Unpack4h(packedAs.xy) + Unpack4h(packedBs.xy))* rcpScale)));

        //     output.storage.Store2(writeOffset + c * 1, packedI8);

        //     c += 4;
        // }
    }
}

template<typename SA, typename SB, typename SO>
void PipelinedAdd(
    const int3 start,
    const uint numChannels,
    const QuantizedTensor3i8_NHWC<SA> a,
    const Tensor3h_NHWC<SB> b,
    const QuantizedTensor3i8_NHWC<SO> output)
{
    const float inputScale = a.quantizationScale;
    const float rcpScale = 1.0 / output.quantizationScale;
    const uint aOffset = a.OffsetOf(start);
    const uint bOffset = b.OffsetOf(start);
    const uint writeOffset = output.OffsetOf(start);

    [unroll]
    for (int c = 0; c != numChannels; )
    {
        const int3 po = start + int3(0, 0, c);

        const int remainder = numChannels - c;
        if (remainder >= 8)
        {
            const uint2 packedAs = a.storage.Load2(aOffset + c);
            const uint4 packedBs = b.storage.Load4(bOffset + c * 2);

            const int8_t4_packed2 packedI8s = int8_t4_packed2(
                pack_clamp_s8(int4(round(((unpack_s8s16(packedAs.x) * inputScale) + Unpack4h(packedBs.xy))* rcpScale))),
                pack_clamp_s8(int4(round(((unpack_s8s16(packedAs.y) * inputScale) + Unpack4h(packedBs.zw))* rcpScale)))
            );

            output.storage.Store2(writeOffset + c * 1, packedI8s);

            c += 8;
        }
        // else 
        // if (remainder >= 4)
        // {
        //     const uint2 packedAs = a.storage.Load2(aOffset + c * 2);
        //     const uint2 packedBs = b.storage.Load2(bOffset + c * 2);

        //     const int8_t4_packed packedI8 = pack_clamp_s8(int4(round((Unpack4h(packedAs.xy) + Unpack4h(packedBs.xy))* rcpScale)));

        //     output.storage.Store2(writeOffset + c * 1, packedI8);

        //     c += 4;
        // }
    }
}

template<typename SA, typename SB, typename SO>
void PipelinedAdd(
    const int3 start,
    const uint numChannels,
    const Tensor3h_NHWC<SA> a,
    const QuantizedTensor3i8_NHWC<SB> b,
    const QuantizedTensor3i8_NHWC<SO> output)
{
    const float inputScale = b.quantizationScale;
    const float rcpScale = 1.0 / output.quantizationScale;
    const uint aOffset = a.OffsetOf(start);
    const uint bOffset = b.OffsetOf(start);
    const uint writeOffset = output.OffsetOf(start);

    [unroll]
    for (int c = 0; c != numChannels; )
    {
        const int3 po = start + int3(0, 0, c);

        const int remainder = numChannels - c;
        if (remainder >= 8)
        {
            const uint2 packedBs = b.storage.Load2(bOffset + c);
            const uint4 packedAs = a.storage.Load4(aOffset + c * 2);

            const int8_t4_packed2 packedI8s = int8_t4_packed2(
                pack_clamp_s8(int4(round(((unpack_s8s16(packedBs.x) * inputScale) + Unpack4h(packedAs.xy))* rcpScale))),
                pack_clamp_s8(int4(round(((unpack_s8s16(packedBs.y) * inputScale) + Unpack4h(packedAs.zw))* rcpScale)))
            );

            output.storage.Store2(writeOffset + c * 1, packedI8s);

            c += 8;
        }
        // else 
        // if (remainder >= 4)
        // {
        //     const uint2 packedAs = a.storage.Load2(aOffset + c * 2);
        //     const uint2 packedBs = b.storage.Load2(bOffset + c * 2);

        //     const int8_t4_packed packedI8 = pack_clamp_s8(int4(round((Unpack4h(packedAs.xy) + Unpack4h(packedBs.xy))* rcpScale)));

        //     output.storage.Store2(writeOffset + c * 1, packedI8);

        //     c += 4;
        // }
    }
}

template<typename T, typename T2, typename T3>
void Add(
    const T a,
    const T2 b,
    const T3 output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;
    const int3 workEnd = min(output.threadGroupSliceStart + output.threadGroupSliceSize, output.logicalSize);
    if (any(workOffset >= workEnd))
        return;


    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
            PipelinedAdd(workOffset + int3(x, y, 0), perThreadWork.z, a, b, output);
}