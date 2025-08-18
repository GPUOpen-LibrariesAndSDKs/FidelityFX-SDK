// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3h_NHWC<SA> a,
    const Tensor3h_NHWC<SB> b,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (!IsAligned(a.storageSize.z, 2) || !IsAligned(b.storageSize.z, 2))
        return;
    
    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (int y = 0 ; y < perThreadWork.y; y++)
        for (int x = 0 ; x < perThreadWork.x; x++)
            for (int z = 0 ; z < perThreadWork.z; z += 2)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const int c = po.z;

                half2 v;
                // TODO: Only works if a is even in x/y dimensions, consider case where a is odd
                if (c < a.logicalSize.z)
                    v = Load2hA(a, po);
                else
                    v = Load2hA(b, po - int3(0, 0, a.logicalSize.z));

                Store2hA(output, po, v);
            }
}

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier, // must be C for now
    const QuantizedTensor3i8_NHWC<SA> a,
    const QuantizedTensor3i8_NHWC<SB> b,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (!IsAligned(a.storageSize.z, 4) || !IsAligned(b.storageSize.z, 4))
        return;

    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (int y = 0 ; y < perThreadWork.y; y++)
        for (int x = 0 ; x < perThreadWork.x; x++)
            for (int z = 0 ; z < perThreadWork.z; z += 4)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const int c = po.z;

                half4 v;
                if (c < a.logicalSize.z)
                    v = unpack_s8s16(Load4i8A(a, po)) * a.quantizationScale;
                else
                    v = unpack_s8s16(Load4i8A(b, po - int3(0, 0, a.logicalSize.z))) * b.quantizationScale;

                Store4hA(output, po, v);
            }
}

template<typename SI, typename SO>
void Concat(
    const float quant0,
    const float quant1,
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3i8_NHWC<SI> input,
    const Tensor3h_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (!IsAligned(input.storageSize.z, 2) || !IsAligned(output.storageSize.z, 2))
        return;

    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
            for (int z = 0; z < perThreadWork.z; z += 4)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const int c = po.z;

                half4 v;
                if (c < input.logicalSize.z / 2)
                    v = unpack_s8s16(Load4i8A(input, po)) * quant0;
                else
                    v = unpack_s8s16(Load4i8A(input, po)) * quant1;

                Store4hA(output, po, v);
            }
}

template<typename SI, typename SO>
void Concat(
    const float quant0,
    const float quant1,
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3i8_NHWC<SI> input,
    const Tensor3i8_NHWC< SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (!IsAligned(input.storageSize.z, 2) || !IsAligned(output.storageSize.z, 2))
        return;

    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
            for (int z = 0; z < perThreadWork.z; z += 4)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                const int c = po.z;

                half4 v;
                if (c < input.logicalSize.z / 2)
                    v = unpack_s8s16(Load4i8A(input, po)) * quant0 / output.quantizationScale;
                else
                    v = unpack_s8s16(Load4i8A(input, po)) * quant1 / output.quantizationScale;
                
                const int8_t4_packed result = pack_clamp_s8(int16_t4(round(v)));

                Store4i8A(output, po, result);
            }
}