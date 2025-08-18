// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.

#include "ml2code_runtime/tensor_float32.hlsli"
#include "ml2code_runtime/tensor_int32.hlsli"

template<typename SI, typename SO>
void Resize2x_nearest(
    const Tensor3f<SI> input,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    for (int c = 0 ; c < perThreadWork.z; c++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const uint3 pi = uint3(po.xy >> 1, po.z);

                const float v = Load1f(input, pi);
                Store1f(output, po, v);
            }
}

template<typename SI, typename SS, typename Roi, typename SO>
void Resize_nearest(
    const Tensor3f<SI> input,
    const Roi _roi,
    const Tensor1f<SS> _scales,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const float4 scales = Load4f(_scales, 0).wzyx;

    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    for (int c = 0 ; c < perThreadWork.z; c++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const uint2 piXY = uint2(min(round(po.xy / scales), input.logicalSize.xy-1));
                const uint3 pi = uint3(piXY, po.z);

                const float v = Load1f(input, pi);
                Store1f(output, po, v);
            }
}


template<typename SI, typename SS, typename Roi, typename SO, typename Size>
void Resize_nearest(
    const Tensor3f<SI> input,
    const Roi _roi,
    const Tensor1f<SS> _scales,
    const Size _size,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const float2 scales = Load2f(_scales, 0);

    const uint3 workPerTick = uint3(1, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, workPerTick);
    const uint3 threadWorkStart = computeShaderParams.groupThreadID * perThreadWork;
    const int3 workOffset = output.threadGroupSliceStart + threadWorkStart;

    if (any(threadWorkStart >= output.threadGroupSliceSize))
        return;

    for (int c = 0 ; c < perThreadWork.z; c++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const uint3 pi = uint3(po.xy / scales, po.z);

                const float v = Load1f(input, pi);
                Store1f(output, po, v);
            }
}

template<typename SI, typename Roi, typename SS, typename SO>
void Resize_nearest2x(
    const Tensor3f<SI> input,
    const Roi _roi,
    const Tensor1f<SS> /*size*/,
    const Tensor3f<SO> output,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, computeShaderParams.numThreads, 1);
    const int3 workOffset = output.threadGroupSliceStart + computeShaderParams.groupThreadID * perThreadWork;

    for (uint c = 0 ; c < perThreadWork.z; c++)
        for (uint y = 0 ; y < perThreadWork.y; y ++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const uint3 po = workOffset + uint3(x, y, c);
                if (!ValidPosition(output, po))
                    continue;

                const uint3 pi = uint3(po.xy >> 1, po.z);

                const float v = Load1f(input, pi);
                Store1f(output, po, v);
            }
}
