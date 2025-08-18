// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/globals.hlsli"
#include "ml2code_runtime/storage.hlsli"

ML2C_DECL_TENSOR(Tensor1i32, 1);
ML2C_DECL_QUANTIZED_TENSOR(QuantizedTensor1i32, 1);


#define ML2C_DECL_LOAD_I32(tensor, rank, n) \
    template<typename S> \
    int##n Load##n##i32(tensor<S> t, int##rank p) \
    { \
        uint byteAddress = t.OffsetOf(p); \
        ML2C_POISON_ON_UNALIGNED_READS_I32(byteAddress); \
        return (int##n)t.storage.Load##n(byteAddress); \
    }

ML2C_DECL_LOAD_I32(Tensor1i32, 1, 1)
ML2C_DECL_LOAD_I32(Tensor1i32, 1, 2)
ML2C_DECL_LOAD_I32(Tensor1i32, 1, 3)
ML2C_DECL_LOAD_I32(Tensor1i32, 1, 4)
ML2C_DECL_LOAD_I32(QuantizedTensor1i32, 1, 1)
ML2C_DECL_LOAD_I32(QuantizedTensor1i32, 1, 2)
ML2C_DECL_LOAD_I32(QuantizedTensor1i32, 1, 4)
