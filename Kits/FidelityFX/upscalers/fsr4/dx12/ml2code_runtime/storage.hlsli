// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

struct BufferStorage
{
    ByteAddressBuffer buffer;

    uint Load1(uint byteOffset) { return buffer.Load(byteOffset); }
    uint2 Load2(uint byteOffset) { return buffer.Load2(byteOffset); }
    uint3 Load3(uint byteOffset) { return buffer.Load3(byteOffset); }
    uint4 Load4(uint byteOffset) { return buffer.Load4(byteOffset); }
};

template<uint N>
struct ConstantBufferStorage
{
    uint dwords[N];

    uint Load1(uint byteOffset) {
        const uint dwordOffset = byteOffset >> 2;
        return dwords[dwordOffset];
    }
    uint2 Load2(uint byteOffset) {
        const uint dwordOffset = byteOffset >> 2;
        return uint2(dwords[dwordOffset], dwords[dwordOffset+1]);
    }
    uint3 Load3(uint byteOffset) {
        const uint dwordOffset = byteOffset >> 2;
        return uint3(dwords[dwordOffset], dwords[dwordOffset+1], dwords[dwordOffset+2]);
    }
    uint4 Load4(uint byteOffset) {
        const uint dwordOffset = byteOffset >> 2;
        return uint4(dwords[dwordOffset], dwords[dwordOffset+1], dwords[dwordOffset+2], dwords[dwordOffset+3]);
    }
};

struct RWBufferStorage
{
    RWByteAddressBuffer buffer;

    void Store1(uint byteOffset, uint v) { buffer.Store(byteOffset, v); }
    void Store2(uint byteOffset, uint2 v) { buffer.Store2(byteOffset, v); }
    void Store3(uint byteOffset, uint3 v) { buffer.Store3(byteOffset, v); }
    void Store4(uint byteOffset, uint4 v) { buffer.Store4(byteOffset, v); }

    uint Load1(uint byteOffset) { return buffer.Load(byteOffset); }
    uint2 Load2(uint byteOffset) { return buffer.Load2(byteOffset); }
    uint3 Load3(uint byteOffset) { return buffer.Load3(byteOffset); }
    uint4 Load4(uint byteOffset) { return buffer.Load4(byteOffset); }
};

#ifndef ML2C_GROUPSHARED_SIZE
#define ML2C_GROUPSHARED_SIZE 32*1024
#endif

groupshared uint ml2c_ldsScratch[ML2C_GROUPSHARED_SIZE/4];

struct LDSStorage
{
    uint ldsDWordOffset;

    uint Load1(uint byteOffset) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        return ml2c_ldsScratch[dwordOffset];
    }
    uint2 Load2(uint byteOffset) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        return uint2(ml2c_ldsScratch[dwordOffset], ml2c_ldsScratch[dwordOffset + 1]);
    }
    uint3 Load3(uint byteOffset) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        return uint3(ml2c_ldsScratch[dwordOffset], ml2c_ldsScratch[dwordOffset + 1], ml2c_ldsScratch[dwordOffset + 2]);
    }
    uint4 Load4(uint byteOffset) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        return uint4(ml2c_ldsScratch[dwordOffset], ml2c_ldsScratch[dwordOffset + 1], ml2c_ldsScratch[dwordOffset + 2], ml2c_ldsScratch[dwordOffset + 3]);
    }

    void Store1(uint byteOffset, uint v) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        ml2c_ldsScratch[dwordOffset] = v;
    }
    void Store2(uint byteOffset, uint2 v) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        ml2c_ldsScratch[dwordOffset] = v.x;
        ml2c_ldsScratch[dwordOffset+1] = v.y;
    }
    void Store3(uint byteOffset, uint3 v) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        ml2c_ldsScratch[dwordOffset] = v.x;
        ml2c_ldsScratch[dwordOffset+1] = v.y;
        ml2c_ldsScratch[dwordOffset+2] = v.z;
    }
    void Store4(uint byteOffset, uint4 v) {
        const uint dwordOffset = ldsDWordOffset + (byteOffset >> 2);
        ml2c_ldsScratch[dwordOffset] = v.x;
        ml2c_ldsScratch[dwordOffset + 1] = v.y;
        ml2c_ldsScratch[dwordOffset + 2] = v.z;
        ml2c_ldsScratch[dwordOffset + 3] = v.w;
    }
};
