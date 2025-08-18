// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/globals.hlsli"
#include "ml2code_runtime/storage.hlsli"

ML2C_DECL_TENSOR(Tensor1i8, 1);
ML2C_DECL_TENSOR(Tensor3i8_NHWC, 3);
ML2C_DECL_TENSOR(Tensor4i8_NHWC, 4);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor3i8_NHWC, 3);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor4i8_NHWC, 4);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor4i8_HWCN, 4);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor4i8_HWNC, 4);


#define ML2C_DECL_LOADA_I8(tensor, rank) \
    template<typename S> \
    int8_t4_packed Load4i8A(tensor<S> t, int##rank p) \
    { \
        uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_READS_I8(byteAddress); \
        return (int8_t4_packed)t.storage.Load1(byteAddress); \
    }

ML2C_DECL_LOADA_I8(Tensor3i8_NHWC, 3)
ML2C_DECL_LOADA_I8(Tensor4i8_NHWC, 4)
ML2C_DECL_LOADA_I8(QuantizedTensor3i8_NHWC, 3)
ML2C_DECL_LOADA_I8(QuantizedTensor4i8_NHWC, 4)
ML2C_DECL_LOADA_I8(QuantizedTensor4i8_HWCN, 4)
ML2C_DECL_LOADA_I8(QuantizedTensor4i8_HWNC, 4)

#define ML2C_DECL_STOREA_I8(tensor, rank) \
    template<typename S> \
    void Store4i8A(tensor<S> t, int##rank p, int8_t4_packed packed) \
    { \
        uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_WRITES_I8(byteAddress, packed); \
        t.storage.Store1(byteAddress, packed); \
    }

ML2C_DECL_STOREA_I8(Tensor3i8_NHWC, 3)
ML2C_DECL_STOREA_I8(QuantizedTensor3i8_NHWC, 3)
