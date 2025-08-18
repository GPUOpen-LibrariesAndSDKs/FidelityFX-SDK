// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#include "ml2code_runtime/config.hlsli"

#define ML2C_DECL_TENSOR(name, rank) \
    template<typename Storage> \
    struct name \
    { \
        uint##rank logicalSize; /* Logical size of tensor */ \
        int##rank threadGroupSliceStart; /* Start in logical coordinates of this thread group's work. This can be outside of the logical size of the underlying tensor. */ \
        uint##rank threadGroupSliceSize; /* Size in logical units of this thread group's work */ \
        uint##rank storageSize; /* Underlying storage size of tensor must be as big or bigger than logicalSize. Values outside logicalSize are 0 and is safe to read **up to alignment** */ \
        uint##rank storageByteStrides; /* The stride in bytes of this thread group slice */ \
        uint##rank paddingBegin; /* The offset in logical coordinates for any padding used. */ \
        uint##rank paddingEnd; /* Additional padding in logical coordinates used each dimension after the logical data. */ \
        uint threadGroupStorageByteOffset; /* Offset in bytes of the start of this thread group slice, can be negative if the thread group logically starts outside the tensor */ \
        Storage storage; /* The backing storage of this tensor, must be addressed in DWORD/4B alignment */ \
        \
        /* Computes the byte offset of the logical tensor position p. \
        p.xyzw always follows WHCN ordering, no matter what is the ordering declared in the tensor type name like `Tensor3i8_NHWC`. */ \
        uint OffsetOf(uint##rank p) { return dot(storageByteStrides, p + paddingBegin - threadGroupSliceStart) + threadGroupStorageByteOffset; } \
    };

#define ML2C_DECL_QUANTIZED_TENSOR(name, rank) \
    template<typename Storage> \
    struct name \
    { \
        uint##rank logicalSize; /* Logical size of tensor */ \
        int##rank threadGroupSliceStart; /* Start in logical coordinates of this thread group's work. This can be outside of the logical size of the underlying tensor. */ \
        uint##rank threadGroupSliceSize; /* Size in logical units of this thread group's work */ \
        uint##rank storageSize; /* Underlying storage size of tensor must be as big or bigger than logicalSize. Values outside logicalSize are 0 and is safe to read */ \
        uint##rank storageByteStrides; /* The stride in bytes of this thread group slice */ \
        uint##rank paddingBegin; /* The offset in logical coordinates for any padding used. */ \
        uint##rank paddingEnd; /* Additional padding in logical coordinates used each dimension after the logical data. */  \
        uint threadGroupStorageByteOffset; /* Offset in bytes of the start of this thread group slice, can be negative if the thread group logically starts outside the tensor */ \
        float quantizationScale; /* Scale to apply to data being read */\
        /* float quantizationOffset; */ \
        Storage storage; /* The backing storage of this tensor, must be addressed in DWORD/4B alignment */ \
        \
        /* Computes the byte offset of the logical tensor position p. \
        p.xyzw always follows WHCN ordering, no matter what is the ordering declared in the tensor type name like `Tensor3i8_NHWC`. */ \
        uint OffsetOf(uint##rank p) { return dot(storageByteStrides, p + paddingBegin - threadGroupSliceStart) + threadGroupStorageByteOffset; } \
    };

struct NullTensor {};

struct ComputeShaderParams
{
    uint3 numThreads;
    uint3 groupID;          // Group ID within dispatch.
    uint3 groupThreadID;    // Thread ID local in the group.
    uint3 dispatchThreadID; // Thread ID global in the entire dispatch.
};

enum class AxisSpecifier
{
    N,
    C,
    H,
    W,
};

enum class PaddingMode
{
    Constant,
    Reflect,
    Edge,
    Wrap
};

float1 Unpack1f(uint1 v) { return asfloat(v); }
float2 Unpack2f(uint2 v) { return asfloat(v); }
float3 Unpack3f(uint3 v) { return asfloat(v); }
float4 Unpack4f(uint4 v) { return asfloat(v); }

uint1 Pack1f(float1 v) { return asuint(v); }
uint2 Pack2f(float2 v) { return asuint(v); }
uint3 Pack3f(float3 v) { return asuint(v); }
uint4 Pack4f(float4 v) { return asuint(v); }


half1 Unpack1h(uint a) { return half(f16tof32(a)); }
half2 Unpack2h(uint a) { return half2(f16tof32(uint2(a & 0xFFFF, a >> 16))); }
half2 Unpack2h(uint a, uint b) { return half2(f16tof32(a), f16tof32(b)); }
half4 Unpack4h(uint2 v) { return half4(Unpack2h(v.x), Unpack2h(v.y)); }

uint Pack2h(half2 v) { return uint(f32tof16(v.x) | (f32tof16(v.y) << 16)); }
uint2 Pack4h(half4 v) { return uint2(Pack2h(v.xy), Pack2h(v.zw)); }


uint Pack4u8(float4 vs) {
    return uint(vs.x)
        | (uint(vs.y) << 8)
        | (uint(vs.z) << 16)
        | (uint(vs.w) << 24);
}

template<typename Tensor, typename PosType>
bool InsideSlice(Tensor t, PosType p)
{
#if ML2C_ENABLE_SLICE_BOUNDS
    return all(and(p >= t.threadGroupSliceStart, p < t.threadGroupSliceStart + t.threadGroupSliceSize));
#else
    return true;
#endif
}

template<typename Tensor, typename PosType>
bool InsideTensor(Tensor t, PosType p)
{
    return all(and(p >= 0, p < t.logicalSize));
}


template<typename Tensor, typename PosType>
bool ValidPosition(Tensor t, PosType p)
{
    return InsideSlice(t, p) && InsideTensor(t, p);
}

template<typename Tensor, typename PosType>
int3 EdgeOffset(Tensor t, PosType p)
{
    int2 xy = min(max(p.xy,int2(0,0)),t.logicalSize.xy - int2(1,1));
    return int3(xy,p.z);
}

template<typename Tensor, typename PosType>
bool ValidPosition(Tensor t, PosType p, bool needSliceBounds)
{
    return (!needSliceBounds || InsideSlice(t, p)) && InsideTensor(t, p);
}

template<typename Tensor, typename PosType>
bool ValidPosition(Tensor t, PosType p, bool needSliceBounds, bool needGlobalBounds)
{
    return (!needSliceBounds || InsideSlice(t, p)) && (!needGlobalBounds || InsideTensor(t, p));
}

template<typename T>
bool NeedSliceBounds(T perThreadWork, T numThreads, T sliceSize)
{
#if ML2C_ENABLE_SLICE_BOUNDS
    return any((perThreadWork * numThreads) != sliceSize);
#else
    return false;
#endif
}

bool NeedGlobalBounds(int3 sliceStart, uint3 sliceSize, uint3 tensorSize)
{
#if ML2C_ENABLE_GLOBAL_BOUNDS
    // TODO: this does not work since sliceStart is slice relative
    if (any(sliceStart < 0))
        return true;

    return any(tensorSize % sliceSize) != 0;
#else
    return false;
#endif
}

uint DivideRoundingUp(uint a, uint b) { return (a + b - 1) / b; }
uint2 DivideRoundingUp(uint2 a, uint2 b) { return (a + b - 1.xx) / b; }
uint3 DivideRoundingUp(uint3 a, uint3 b) { return (a + b - 1.xxx) / b; }
uint4 DivideRoundingUp(uint4 a, uint4 b) { return (a + b - 1.xxxx) / b; }

uint SplitWork(uint workSize, uint numThreads, uint minWorkPerThread)
{
    return max(DivideRoundingUp(workSize, numThreads), minWorkPerThread);
}

uint2 SplitWork(uint2 workSize, uint2 numThreads, uint2 minWorkPerThread)
{
    return max(DivideRoundingUp(workSize, numThreads), minWorkPerThread);
}

uint3 SplitWork(uint3 workSize, uint3 numThreads, uint3 minWorkPerThread)
{
    return max(DivideRoundingUp(workSize, numThreads), minWorkPerThread);
}

bool IsDWordAligned(uint p) { return (p & 0x3) == 0; }
bool IsAligned(uint p, uint alignment) { return (p % alignment) == 0; }
uint AlignDownToDWord(uint p) { return p & ~3; }
