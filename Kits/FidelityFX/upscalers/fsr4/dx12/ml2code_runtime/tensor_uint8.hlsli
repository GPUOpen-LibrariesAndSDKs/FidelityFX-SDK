// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/globals.hlsli"
#include "ml2code_runtime/storage.hlsli"

ML2C_DECL_TENSOR(Tensor1u8, 1);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor3u8_NHWC, 3);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor4u8_NHWC, 4);


#define ML2C_DECL_LOADA_U8(tensor, rank) \
    template<typename S> \
    uint8_t4_packed Load4u8A(tensor<S> t, int##rank p) \
    { \
        uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_READS_U8(byteAddress); \
        return (uint8_t4_packed)t.storage.Load1(byteAddress); \
    }

ML2C_DECL_LOADA_U8(QuantizedTensor3u8_NHWC, 3)

#define ML2C_DECL_STOREA_U8(tensor, rank) \
    template<typename S> \
    void Store4u8A(tensor<S> t, int##rank p, uint8_t4_packed packed) \
    { \
        uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_WRITES_U8(byteAddress, packed); \
        t.storage.Store1(byteAddress, packed); \
    }

ML2C_DECL_STOREA_U8(QuantizedTensor3u8_NHWC, 3)
