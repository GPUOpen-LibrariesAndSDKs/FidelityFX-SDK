// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/globals.hlsli"
#include "ml2code_runtime/storage.hlsli"

ML2C_DECL_TENSOR(Tensor1h, 1)
ML2C_DECL_TENSOR(Tensor2h, 2)
ML2C_DECL_TENSOR(Tensor3h, 3)
ML2C_DECL_TENSOR(Tensor4h, 4)
ML2C_DECL_TENSOR(Tensor3h_NHWC, 3)
ML2C_DECL_TENSOR(Tensor4h_NHWC, 4)
ML2C_DECL_TENSOR(Tensor4h_HWCN, 4)
ML2C_DECL_TENSOR(Tensor4h_HWNC, 4)
ML2C_DECL_TENSOR(Tensor4h_HNWC, 4)

#define ML2C_DECL_LOADA_H(tensor, rank, numDwords, n) \
    template<typename S> \
    half##n Load##n##hAOffset(tensor<S> t, uint offset) \
    { \
        return Unpack##n##h(t.storage.Load##numDwords(offset)); \
    } \
    \
    template<typename S> \
    half##n Load##n##hA(tensor<S> t, int##rank p) \
    { \
        const uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_READS_H(byteAddress); \
        return Load##n##hAOffset(t, byteAddress); \
    }

ML2C_DECL_LOADA_H(Tensor1h, 1, 1, 2)
ML2C_DECL_LOADA_H(Tensor1h, 1, 2, 4)
ML2C_DECL_LOADA_H(Tensor2h, 2, 1, 2)
ML2C_DECL_LOADA_H(Tensor2h, 2, 2, 4)
ML2C_DECL_LOADA_H(Tensor3h, 3, 1, 2)
ML2C_DECL_LOADA_H(Tensor3h, 3, 2, 4)
ML2C_DECL_LOADA_H(Tensor4h, 4, 1, 2)
ML2C_DECL_LOADA_H(Tensor4h, 4, 2, 4)
ML2C_DECL_LOADA_H(Tensor3h_NHWC, 3, 1, 2)
ML2C_DECL_LOADA_H(Tensor3h_NHWC, 3, 2, 4)
ML2C_DECL_LOADA_H(Tensor4h_NHWC, 4, 1, 2)
ML2C_DECL_LOADA_H(Tensor4h_HNWC, 4, 1, 2)
ML2C_DECL_LOADA_H(Tensor4h_NHWC, 4, 2, 4)
ML2C_DECL_LOADA_H(Tensor4h_HWCN, 4, 2, 4)
ML2C_DECL_LOADA_H(Tensor4h_HWNC, 4, 2, 4)
ML2C_DECL_LOADA_H(Tensor4h_HNWC, 4, 2, 4)

#define ML2C_DECL_STOREA_H(tensor, rank, numDwords, n) \
    template<typename S> \
    void Store##n##hAOffset(tensor<S> t, uint offset, half##n v) \
    { \
        t.storage.Store##numDwords(offset, Pack##n##h(v)); \
    } \
    \
    template<typename S> \
    void Store##n##hA(tensor<S> t, int##rank p, half##n v) \
    { \
        uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_WRITES_H(byteAddress, v); \
        Store##n##hAOffset(t, byteAddress, v); \
    }

ML2C_DECL_STOREA_H(Tensor1h, 1, 1, 2)
ML2C_DECL_STOREA_H(Tensor1h, 1, 2, 4)
ML2C_DECL_STOREA_H(Tensor2h, 2, 1, 2)
ML2C_DECL_STOREA_H(Tensor2h, 2, 2, 4)
ML2C_DECL_STOREA_H(Tensor3h, 3, 1, 2)
ML2C_DECL_STOREA_H(Tensor3h, 3, 2, 4)
ML2C_DECL_STOREA_H(Tensor4h, 4, 1, 2)
ML2C_DECL_STOREA_H(Tensor4h, 4, 2, 4)
ML2C_DECL_STOREA_H(Tensor3h_NHWC, 3, 1, 2)
ML2C_DECL_STOREA_H(Tensor3h_NHWC, 3, 2, 4)
ML2C_DECL_STOREA_H(Tensor4h_NHWC, 4, 1, 2)
ML2C_DECL_STOREA_H(Tensor4h_HNWC, 4, 1, 2)
ML2C_DECL_STOREA_H(Tensor4h_NHWC, 4, 2, 4)
ML2C_DECL_STOREA_H(Tensor4h_HNWC, 4, 2, 4)


#define ML2C_DECL_LOAD_1hU(tensor, rank) \
    template<typename S> \
    half Load1hU(tensor<S> t, int##rank p) \
    { \
        const uint byteAddress = t.OffsetOf(p); \
        uint packed = t.storage.Load1(AlignDownToDWord(byteAddress)); \
        if (!IsDWordAligned(byteAddress)) \
            packed >>= 16; \
        return Unpack1h(packed); \
    }

ML2C_DECL_LOAD_1hU(Tensor1h, 1)
ML2C_DECL_LOAD_1hU(Tensor2h, 2)
ML2C_DECL_LOAD_1hU(Tensor3h, 3)
ML2C_DECL_LOAD_1hU(Tensor4h, 4)
ML2C_DECL_LOAD_1hU(Tensor3h_NHWC, 3)
ML2C_DECL_LOAD_1hU(Tensor4h_NHWC, 4)

#define ML2C_DECL_LOAD_2hU(tensor, rank) \
    template<typename S> \
    half2 Load2hU(tensor<S> t, int##rank p) \
    { \
        const uint byteAddress = t.OffsetOf(p); \
        if (!(byteAddress & 0x3)) \
            return Unpack2h(t.storage.Load1(byteAddress)); \
    \
        const uint2 packedValues = t.storage.Load2(AlignDownToDWord(byteAddress)); \
        return Unpack2h(packedValues.x >> 16, packedValues.y); \
    }

ML2C_DECL_LOAD_2hU(Tensor1h, 1)
ML2C_DECL_LOAD_2hU(Tensor2h, 2)
ML2C_DECL_LOAD_2hU(Tensor3h, 3)
ML2C_DECL_LOAD_2hU(Tensor4h, 4)
ML2C_DECL_LOAD_2hU(Tensor3h_NHWC, 3)
ML2C_DECL_LOAD_2hU(Tensor4h_NHWC, 4)


template<typename T, typename T2>
void CopyTensor1h(const T input, const T2 output, const uint3 numThreads, const uint3 groupThreadId)
{
    // TODO: this is trivial. Just make thread == 0 in the group read the input. Naturally, each thread can write it's own portion into group WriteGroupSharedToGlobal
    if (any(groupThreadId != 0))
        return;

    for (int x = 0; x != output.threadGroupSliceSize; x += 2)
    {
        const int po = output.threadGroupSliceStart + x;
        if (ValidPosition(output, po))
            Store2hA(output, po, Load2hA(input, po));
    }
}


template<typename T, typename T2>
void CopyTensor2h(const T input, const T2 output, const uint3 numThreads, const uint3 groupThreadId)
{
    // TODO: this is trivial. Just make thread == 0 in the group read the input. Naturally, each thread can write it's own portion into group WriteGroupSharedToGlobal
    if (any(groupThreadId != 0))
        return;

    for (int y = 0; y != output.threadGroupSliceSize.y; ++y)
        for (int x = 0; x != output.threadGroupSliceSize.x; x += 2)
        {
            const int2 po = output.threadGroupSliceStart + uint2(x, y);
            if (ValidPosition(output, po))
                Store2hA(output, po, Load2hA(input, po));
        }
}


template<typename T, typename T2>
void CopyTensor3h(const T input, const T2 output, const uint3 numThreads, const uint3 groupThreadId)
{
    // TODO: this is trivial. Just make thread == 0 in the group read the input. Naturally, each thread can write it's own portion into group WriteGroupSharedToGlobal
    if (any(groupThreadId != 0))
        return;

    for (int c = 0; c != output.threadGroupSliceSize.z; ++c)
        for (int y = 0; y != output.threadGroupSliceSize.y; ++y)
            for (int x = 0; x != output.threadGroupSliceSize.x; x += 2)
            {
                const int3 po = output.threadGroupSliceStart + int3(x, y, c);
                if (ValidPosition(output, po))
                    Store2hA(output, po, Load2hA(input, po));
            }
}

template<typename SI, typename SO>
void CopyTensor(const Tensor3h<SI> input, const Tensor3h<SO> output, const uint3 numThreads, uint3 groupThreadId)
{
    const uint3 workPerTick = uint3(2, 1, 1);
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize, numThreads, workPerTick);
    const int3 workOffset = output.threadGroupSliceStart + groupThreadId * perThreadWork;

    for (int z = 0; z < perThreadWork.z; z++)
        for (int y = 0; y < perThreadWork.y; y++)
            for (int x = 0; x < perThreadWork.x; x+=2)
            {
                const int3 po = workOffset + int3(x, y, z);
                if (!ValidPosition(output, po))
                    continue;

                Store2hA(output, po, Load2hA(input, po));
            }
}

template<typename SI, typename SO>
void CopyTensor4h(const Tensor4h_NHWC<SI> input, const Tensor4h_NHWC<SO> output, const uint3 numThreads, uint3 groupThreadId)
{
    const uint3 perThreadWork = SplitWork(output.threadGroupSliceSize.xyw, numThreads, 1);
    const uint3 tmp = groupThreadId * perThreadWork;
    const int4 workOffset = output.threadGroupSliceStart + int4(tmp.xy, 0, tmp.z);

    for (int y = 0; y < perThreadWork.y; y++)
        for (int x = 0; x < perThreadWork.x; x++)
            for (int n = 0; n < perThreadWork.z; n++)
            {
                const int4 poBase = workOffset + int4(x, y, 0, n);
                if (!ValidPosition(output, poBase))
                    continue;

                [unroll]
                for (int c = 0 ; c != output.logicalSize.z; c += 8)
                {
                    const int4 po = poBase + int4(0, 0, c, 0);

                    const uint4 dwords = input.storage.Load4(input.OffsetOf(po));
                    output.storage.Store4(output.OffsetOf(po), dwords);
                }
            }
}
