// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedInnerConv3413_44(
    Algorithm algorithm,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    AccumulatorType accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if ((f % 4) == 0)
            accumulator = algorithm.LoadBias4(f);

        [unroll]
        for (uint ky = 0; ky < algorithm.kernelSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx < algorithm.kernelSize.x; ++kx)
                [unroll]
                for (uint c = 0; c < algorithm.kernelSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f); // position in weights
                    int3 ps = piBase + pw.xyz;

                    const WeightsType ws = algorithm.LoadWeight4(pw);
                    const InputType vs = algorithm.LoadInput4(ps);

                    accumulator[f % 4] = algorithm.Dot(ws, vs, accumulator[f % 4]);
                }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            algorithm.StoreOutput4(po, accumulator);
        }
    }
}


template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedInnerConvPadded3413_44(
    Algorithm algorithm,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    AccumulatorType accumulator;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = algorithm.LoadBias4(f);

        [unroll]
        for (uint ky = 0; ky != algorithm.kernelSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != algorithm.kernelSize.x; ++kx)
            {
                if (!algorithm.ValidInput(int3(piBase.xy + uint2(kx, ky), 0)))
                    continue;

                [unroll]
                for (uint c = 0; c != algorithm.kernelSize.z; c += 4)
                {
                    const uint4 pw = uint4(kx, ky, c, f); // position in weights
                    int3 ps = piBase + pw.xyz;

                    const WeightsType ws = algorithm.LoadWeight4(pw);
                    const InputType vs = algorithm.LoadInput4(ps);

                    accumulator[f % 4] = algorithm.Dot(ws, vs, accumulator[f % 4]);
                }
            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            algorithm.StoreOutput4(po, accumulator);
        }
    }
}


template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedConv2D_3413_NHWC_44(
    const Algorithm algorithm,
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint strideY, uint strideX,
    const int3 sliceStart,
    const uint3 sliceSize,
    const ComputeShaderParams computeShaderParams)
{
    const uint3 workPerTick = uint3(1, 1, sliceSize.z);
    const uint3 perThreadWork = SplitWork(sliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = sliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);
    const uint numFeatures = algorithm.kernelSize.w;

    // // Padded Case
    if (beginX != 0 || beginY != 0)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!algorithm.ValidOutput(poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    TemplatedInnerConvPadded3413_44<AccumulatorType, InputType, WeightsType>(algorithm, piBase, poBase, numFeatures);
            }
    }
    else
    {
        // Unpadded Case
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!algorithm.ValidOutput(poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    TemplatedInnerConv3413_44<AccumulatorType, InputType, WeightsType>(algorithm, piBase, poBase, numFeatures);
            }
    }
}

template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedInnerConv4PaddedGroups_4313_NHWC_44(
    Algorithm algorithm,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    AccumulatorType accumulator;
    int3 pi = piBase;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = algorithm.LoadBias4(f);

        // Assumes groups=2
        if (f == (numFeatures / 2))
            pi = piBase + int3(0, 0, algorithm.kernelSize.z);

        [unroll]
        for (uint ky = 0; ky != algorithm.kernelSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != algorithm.kernelSize.x; ++kx)
            {
                if (algorithm.ValidInput(int3(pi.xy + int2(kx, ky), 0)))
                {
                    [unroll]
                    for (uint c = 0; c != algorithm.kernelSize.z; c += 4)
                    {
                        const uint4 pw = uint4(kx, ky, c, f); // position in weights
                        int3 ps = pi + pw.xyz;

                        const WeightsType ws = algorithm.LoadWeight4(pw);
                        const InputType vs = algorithm.LoadInput4(ps);

                        accumulator[f % 4] = algorithm.Dot(ws, vs, accumulator[f % 4]);
                    }
                }
            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            algorithm.StoreOutput4(po, accumulator);
        }
    }
}

template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedInnerConv4PaddedGroups_NoBias_4313_NHWC_44(
    Algorithm algorithm,
    const int3 piBase,
    const int3 poBase,
    const uint numFeatures)
{
    AccumulatorType accumulator;
    int3 pi = piBase;

    [unroll]
    for (uint f = 0 ; f < numFeatures ;)
    {
        if (!(f % 4))
            accumulator = 0;

        // Assumes groups=2
        if (f == (numFeatures / 2))
            pi = piBase + int3(0, 0, algorithm.kernelSize.z);

        [unroll]
        for (uint ky = 0; ky != algorithm.kernelSize.y; ++ky)
            [unroll]
            for (uint kx = 0; kx != algorithm.kernelSize.x; ++kx)
            {
                if (algorithm.ValidInput(int3(pi.xy + int2(kx, ky), 0)))
                {
                    [unroll]
                    for (uint c = 0; c != algorithm.kernelSize.z; c += 4)
                    {
                        const uint4 pw = uint4(kx, ky, c, f); // position in weights
                        int3 ps = pi + pw.xyz;

                        const WeightsType ws = algorithm.LoadWeight4(pw);
                        const InputType vs = algorithm.LoadInput4(ps);

                        accumulator[f % 4] = algorithm.Dot(ws, vs, accumulator[f % 4]);
                    }
                }
            }

        f++;
        if (!(f % 4))
        {
            int3 po = poBase;
            po.z = f - 4;

            algorithm.StoreOutput4(po, accumulator);
        }
    }
}


template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedGroupedConv2D_3413_NHWC_44(
    Algorithm algorithm,
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint strideY, uint strideX,
    uint groups,
    const int3 sliceStart,
    const uint3 sliceSize,
    const ComputeShaderParams computeShaderParams)
{
    // each thread produces all channels
    if (computeShaderParams.numThreads.z != 1)
        return;

    if (groups != 2)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, sliceSize.z);
    const uint3 perThreadWork = SplitWork(sliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = sliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);
    const uint numFeatures = algorithm.kernelSize.w;

    // Padded Case
    if (beginX != 0 || beginY != 0)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!algorithm.ValidOutput(poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    TemplatedInnerConv4PaddedGroups_4313_NHWC_44<AccumulatorType, InputType, WeightsType>(algorithm, piBase, poBase, numFeatures);
            }
    }
}

template<
    typename AccumulatorType,
    typename InputType,
    typename WeightsType,
    typename Algorithm>
void TemplatedGroupedConv2D_NoBias_3413_NHWC_44(
    Algorithm algorithm,
    uint beginY, uint beginX,
    uint endY, uint endX,
    uint strideY, uint strideX,
    uint groups,
    const int3 sliceStart,
    const uint3 sliceSize,
    const ComputeShaderParams computeShaderParams)
{
    // each thread produces all channels
    if (computeShaderParams.numThreads.z != 1)
        return;

    if (groups != 2)
        return; // not implemented

    const uint3 workPerTick = uint3(1, 1, sliceSize.z);
    const uint3 perThreadWork = SplitWork(sliceSize, computeShaderParams.numThreads, workPerTick);
    const int3 workOffset = sliceStart + computeShaderParams.groupThreadID * perThreadWork;
    const uint2 convStrides = uint2(strideX, strideY);
    const uint numFeatures = algorithm.kernelSize.w;

    // Padded Case
    if (beginX != 0 || beginY != 0)
    {
        for (uint y = 0 ; y < perThreadWork.y; y++)
            for (uint x = 0 ; x < perThreadWork.x; x++)
            {
                const int3 poBase = workOffset + int3(x, y, 0);
                if (!algorithm.ValidOutput(poBase))
                    continue;

                const int3 piBase = int3(poBase.xy*convStrides, 0) - int3(beginX, beginY, 0);

                if (IsAligned(numFeatures, 4))
                    TemplatedInnerConv4PaddedGroups_NoBias_4313_NHWC_44<AccumulatorType, InputType, WeightsType>(algorithm, piBase, poBase, numFeatures);
            }
    }
}



template<typename SI, typename SW>
float PipelinedDot(
    const uint baseInputOffset, Tensor3h_NHWC<SI> input,
    const uint baseKernelOffset, Tensor4h_NHWC<SW> kernel,
    const uint numChannels)
{
    float accumulator = 0;

    uint inputOffset = baseInputOffset;
    uint kernelOffset = baseKernelOffset;

    [unroll]
    for (uint c = 0; c < numChannels; )
    {
        const uint remainder = numChannels - c;

        if (remainder >= 8)
        {
            const uint4 inputDwords = input.storage.Load4(inputOffset);
            const uint4 kernelDwords = kernel.storage.Load4(kernelOffset);

            accumulator += dot(Unpack4h(inputDwords.xy), Unpack4h(kernelDwords.xy));
            accumulator += dot(Unpack4h(inputDwords.zw), Unpack4h(kernelDwords.zw));

            inputOffset += 8*2;
            kernelOffset += 8*2;

            c += 8;
        }
        else if (remainder >= 4)
        {
            const float4 vs = float4(Load4hAOffset(input, inputOffset));
            const float4 ws = float4(Load4hAOffset(kernel, kernelOffset));

            accumulator += dot(ws, vs);

            inputOffset += 4*2;
            kernelOffset += 4*2;

            c += 4;
        }
        else if (remainder == 2)
        {
            const float2 vs = float2(Load2hAOffset(input, inputOffset));
            const float2 ws = float2(Load2hAOffset(kernel, kernelOffset));

            accumulator += dot(ws, vs);

            inputOffset += 2*2;
            kernelOffset += 2*2;

            c += 2;
        }
    }

    return accumulator;
}