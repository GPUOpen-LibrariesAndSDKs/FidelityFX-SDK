// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/globals.hlsli"
#include "ml2code_runtime/storage.hlsli"

ML2C_DECL_TENSOR(Tensor1f, 1)
ML2C_DECL_TENSOR(Tensor2f, 2)
ML2C_DECL_TENSOR(Tensor3f, 3)
ML2C_DECL_TENSOR(Tensor4f, 4)
ML2C_DECL_TENSOR(Tensor3f_NHWC, 3)
ML2C_DECL_TENSOR(Tensor4f_NHWC, 4)
ML2C_DECL_TENSOR(Tensor4f_HWNC, 4)
ML2C_DECL_TENSOR(Tensor4f_HNWC, 4)

#define ML2C_DECL_LOAD_F(tensor, rank, n) \
    template<typename S> \
    float##n Load##n##f(tensor<S> t, int##rank p) \
    { \
        ML2C_POISON_ON_SLICE_OOB_LOAD_F(p); \
        ML2C_POISON_ON_TENSOR_OOB_LOAD_F(p); \
        return Unpack##n##f(t.storage.Load##n(t.OffsetOf(p))); \
    }

ML2C_DECL_LOAD_F(Tensor1f, 1, 1)
ML2C_DECL_LOAD_F(Tensor2f, 2, 1)
ML2C_DECL_LOAD_F(Tensor3f, 3, 1)
ML2C_DECL_LOAD_F(Tensor4f, 4, 1)
ML2C_DECL_LOAD_F(Tensor1f, 1, 2)
ML2C_DECL_LOAD_F(Tensor2f, 2, 2)
ML2C_DECL_LOAD_F(Tensor3f, 3, 2)
ML2C_DECL_LOAD_F(Tensor4f, 4, 2)
ML2C_DECL_LOAD_F(Tensor1f, 1, 3)
ML2C_DECL_LOAD_F(Tensor2f, 2, 3)
ML2C_DECL_LOAD_F(Tensor3f, 3, 3)
ML2C_DECL_LOAD_F(Tensor4f, 4, 3)
ML2C_DECL_LOAD_F(Tensor1f, 1, 4)
ML2C_DECL_LOAD_F(Tensor2f, 2, 4)
ML2C_DECL_LOAD_F(Tensor3f, 3, 4)
ML2C_DECL_LOAD_F(Tensor4f, 4, 4)
ML2C_DECL_LOAD_F(Tensor3f_NHWC, 3, 1)
ML2C_DECL_LOAD_F(Tensor4f_NHWC, 4, 1)
ML2C_DECL_LOAD_F(Tensor3f_NHWC, 3, 2)
ML2C_DECL_LOAD_F(Tensor4f_NHWC, 4, 2)
ML2C_DECL_LOAD_F(Tensor4f_HNWC, 4, 2)
ML2C_DECL_LOAD_F(Tensor3f_NHWC, 3, 3)
ML2C_DECL_LOAD_F(Tensor4f_NHWC, 4, 3)
ML2C_DECL_LOAD_F(Tensor4f_HNWC, 4, 3)
ML2C_DECL_LOAD_F(Tensor3f_NHWC, 3, 4)
ML2C_DECL_LOAD_F(Tensor4f_NHWC, 4, 4)
ML2C_DECL_LOAD_F(Tensor4f_HWNC, 4, 2)
ML2C_DECL_LOAD_F(Tensor4f_HNWC, 4, 4)


#define ML2C_DECL_STORE_F(tensor, rank, n) \
    template<typename S> \
    void Store##n##f(tensor<S> t, int##rank p, float##n v) \
    { \
        t.storage.Store##n(t.OffsetOf(p), Pack##n##f(v)); \
    }

ML2C_DECL_STORE_F(Tensor1f, 1, 1)
ML2C_DECL_STORE_F(Tensor2f, 2, 1)
ML2C_DECL_STORE_F(Tensor3f, 3, 1)
ML2C_DECL_STORE_F(Tensor4f, 4, 1)
ML2C_DECL_STORE_F(Tensor1f, 1, 2)
ML2C_DECL_STORE_F(Tensor2f, 2, 2)
ML2C_DECL_STORE_F(Tensor3f, 3, 2)
ML2C_DECL_STORE_F(Tensor4f, 4, 2)
ML2C_DECL_STORE_F(Tensor1f, 1, 3)
ML2C_DECL_STORE_F(Tensor2f, 2, 3)
ML2C_DECL_STORE_F(Tensor3f, 3, 3)
ML2C_DECL_STORE_F(Tensor4f, 4, 3)
ML2C_DECL_STORE_F(Tensor1f, 1, 4)
ML2C_DECL_STORE_F(Tensor2f, 2, 4)
ML2C_DECL_STORE_F(Tensor3f, 3, 4)
ML2C_DECL_STORE_F(Tensor4f, 4, 4)
ML2C_DECL_STORE_F(Tensor3f_NHWC, 3, 1)
ML2C_DECL_STORE_F(Tensor4f_NHWC, 4, 1)
ML2C_DECL_STORE_F(Tensor3f_NHWC, 3, 2)
ML2C_DECL_STORE_F(Tensor4f_NHWC, 4, 2)
ML2C_DECL_STORE_F(Tensor4f_HNWC, 4, 2)
ML2C_DECL_STORE_F(Tensor3f_NHWC, 3, 3)
ML2C_DECL_STORE_F(Tensor4f_NHWC, 4, 3)
ML2C_DECL_STORE_F(Tensor4f_HNWC, 4, 3)
ML2C_DECL_STORE_F(Tensor3f_NHWC, 3, 4)
ML2C_DECL_STORE_F(Tensor4f_NHWC, 4, 4)
ML2C_DECL_STORE_F(Tensor4f_HNWC, 4, 4)

template<typename T, typename T2>
void CopyTensor1f(const T input, const T2 output, const uint3 numThreads, const uint3 groupThreadId)
{
    // TODO: this is trivial. Just make thread == 0 in the group read the input. Naturally, each thread can write it's own portion into group WriteGroupSharedToGlobal
    if (any(groupThreadId != 0))
        return;

    for (uint x = 0; x != output.threadGroupSliceSize; ++x)
    {
        const int po = output.threadGroupSliceStart + x;
        if (ValidPosition(output, po))
            Store1f(output, po, Load1f(input, po));
    }
}


template<typename T, typename T2>
void CopyTensor2f(const T input, const T2 output, const uint3 numThreads, const uint3 groupThreadId)
{
    // TODO: this is trivial. Just make thread == 0 in the group read the input. Naturally, each thread can write it's own portion into group WriteGroupSharedToGlobal
    if (any(groupThreadId != 0))
        return;

    for (uint y = 0; y != output.threadGroupSliceSize.y; ++y)
        for (uint x = 0; x != output.threadGroupSliceSize.x; ++x)
        {
            const int2 po = output.threadGroupSliceStart + uint2(x, y);
            if (ValidPosition(output, po))
                Store1f(output, po, Load1f(input, po));
        }
}


template<typename T, typename T2>
void CopyTensor3f(const T input, const T2 output, const uint3 numThreads, const uint3 groupThreadId)
{
    // TODO: this is trivial. Just make thread == 0 in the group read the input. Naturally, each thread can write it's own portion into group WriteGroupSharedToGlobal
    if (any(groupThreadId != 0))
        return;

    for (uint c = 0; c != output.threadGroupSliceSize.z; ++c)
        for (uint y = 0; y != output.threadGroupSliceSize.y; ++y)
            for (uint x = 0; x != output.threadGroupSliceSize.x; ++x)
            {
                const int3 po = output.threadGroupSliceStart + uint3(x, y, c);
                if (all(and(po < output.logicalSize, po >= 0)))
                    Store1f(output, po, Load1f(input, po));
            }
}
