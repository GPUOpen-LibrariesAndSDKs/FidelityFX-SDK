// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/tensor_float16.hlsli"

template<typename AccumulatorType, typename InputType, typename WeightsType, typename Algorithm>
void TemplatedConvTranspose2D_2x2_2x2_3413_NHWC_44(
    Algorithm algorithm,
    const int3 sliceStart,
    const uint3 sliceSize,
    const ComputeShaderParams computeShaderParams)
{
    // no padding, not dilation, stride=2x2, kernel size=2x2
    const uint3 workPerTick = uint3(1, 1, sliceSize.z);
    const uint3 perThreadWork = SplitWork(sliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = sliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 ks = algorithm.kernelSize.xy;

    const uint strideX = 2;
    const uint strideY = 2;

    for (uint y = 0 ; y < perThreadWork.y; y++)
        for (uint x = 0 ; x < perThreadWork.x; x++)
        {
            const int3 poBase = workOffset + int3(x, y, 0);
            if (!algorithm.ValidOutput(poBase))
                continue;

            for (uint z = 0; z < perThreadWork.z; z += 4)
            {
                int3 po = poBase + int3(0,0,z);
                const uint2 strides = uint2(strideX, strideY);

                AccumulatorType accumulator = 0;
                [unroll]
                for (int ky = 0 ; ky < ks.y; ++ky)
                    [unroll]
                    for (int kx = 0 ; kx < ks.x; ++kx)
                    {
                        const int2 pk = ks - 1 - int2(kx, ky);
                        const int2 stridedPs = po.xy + int2(kx, ky) - (ks/2) - (ks & 1);
                        if (any((stridedPs % strides) != 0))
                            continue;

                        for (int c = 0 ; c != algorithm.kernelSize.w; c += 4)
                        {
                            const int3 ps = int3(stridedPs / strides, c);
                            const InputType vs = algorithm.LoadInput4(ps);

                            const WeightsType w0 = algorithm.LoadWeight4(uint4(pk, po.z, c));
                            const WeightsType w1 = algorithm.LoadWeight4(uint4(pk, po.z, c+1));
                            const WeightsType w2 = algorithm.LoadWeight4(uint4(pk, po.z, c+2));
                            const WeightsType w3 = algorithm.LoadWeight4(uint4(pk, po.z, c+3));

                            accumulator += w0 * vs.x +
                                w1 * vs.y +
                                w2 * vs.z +
                                w3 * vs.w;
                        }
                    }

                algorithm.StoreOutput4(po, accumulator);
            }

        }
}