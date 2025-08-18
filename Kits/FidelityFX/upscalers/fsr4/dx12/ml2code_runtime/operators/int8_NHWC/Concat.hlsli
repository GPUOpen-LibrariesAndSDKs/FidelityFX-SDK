// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier, // must be C for now
    const QuantizedTensor3i8_NHWC<SA> a,
    const QuantizedTensor3i8_NHWC<SB> b,
    const Tensor3i8_NHWC<SO> output,
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

                int8_t4_packed v;
                if (c < a.logicalSize.z)
                    v = Load4i8A(a, po);
                else
                    v = Load4i8A(b, po - int3(0, 0, a.logicalSize.z));

                Store4i8A(output, po, v);
            }
}

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3h_NHWC<SA> a,
    const Tensor3h_NHWC<SB> b,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (!IsAligned(a.storageSize.z, 4) || !IsAligned(b.storageSize.z, 4))
        return;

    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const float rcpScale = 1.0 / output.quantizationScale;

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
                    v = Load4hA(a, po);
                else
                    v = Load4hA(b, po - int3(0, 0, a.logicalSize.z));

                const int8_t4_packed packed = pack_clamp_s8(int16_t4(round(v * rcpScale)));
                Store4i8A(output, po, packed);
            }
}

template<typename SA, typename SB, typename SO>
void Concat(
    AxisSpecifier axisSpecifier, // must be C for now
    const Tensor3h_NHWC<SA> a,
    const QuantizedTensor3i8_NHWC< SB> b,
    const QuantizedTensor3i8_NHWC<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    if (!IsAligned(a.storageSize.z, 4) || !IsAligned(b.storageSize.z, 4))
        return;

    const uint3 workPerTick = uint3(1, 1, output.logicalSize.z);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const float rcpScale = 1.0 / output.quantizationScale;

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
                    v = Load4hA(a, po);
                else
                    v = unpack_s8s32(Load4i8A(b, po - int3(0, 0, a.logicalSize.z))) * b.quantizationScale;

                const int8_t4_packed packed = pack_clamp_s8(int16_t4(round(v * rcpScale)));
                Store4i8A(output, po, packed);
            }
}