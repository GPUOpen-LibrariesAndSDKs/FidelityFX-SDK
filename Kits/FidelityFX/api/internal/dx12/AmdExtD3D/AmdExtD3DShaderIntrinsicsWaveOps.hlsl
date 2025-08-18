/* Copyright (c) 2017-2025 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef _AMDEXTD3DSHADERINTRINICSWAVE_HLSL
#define _AMDEXTD3DSHADERINTRINICSWAVE_HLSL

#include "AmdExtD3Dshaderintrinsics.hlsl"

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum
*
*   Performs reduction operation across a wave and returns the result of the reduction (sum of all threads in a wave)
*   to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveActiveSum(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveActiveSum(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveActiveSum(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveActiveSum(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveSum(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveSum(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveSum(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveSum(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveSum(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveSum(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveSum(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveSum<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveSum(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct
*
*   Performs reduction operation across a wave and returns the result of the reduction (product of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveActiveProduct(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveActiveProduct(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveActiveProduct(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveActiveProduct(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveProduct(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveProduct(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveProduct(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveProduct(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveProduct(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveProduct(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveProduct(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveProduct<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveProduct(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin
*
*   Performs reduction operation across a wave and returns the result of the reduction (minimum of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveActiveMin(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveActiveMin(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveActiveMin(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveActiveMin(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveMin(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveMin(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveMin(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveMin(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveMin(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveMin(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveMin(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMin<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveMin(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax
*
*   Performs reduction operation across a wave and returns the result of the reduction (maximum of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveActiveMax(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveActiveMax(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveActiveMax(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveActiveMax(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveMax(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveMax(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveMax(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveMax(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveMax(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveMax(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveMax(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveMax<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveMax(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd
*
*   Performs reduction operation across a wave and returns the result of the reduction (Bitwise AND of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitAnd<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveBitAnd(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr
*
*   Performs reduction operation across a wave and returns the result of the reduction (Bitwise OR of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveBitOr(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveBitOr(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveBitOr(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveBitOr(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveBitOr(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveBitOr(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveBitOr(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitOr<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveBitOr(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor
*
*   Performs reduction operation across a wave and returns the result of the reduction (Bitwise XOR of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveActiveBitXor(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveActiveBitXor(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveActiveBitXor(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveActiveBitXor(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveActiveBitXor(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveActiveBitXor(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveActiveBitXor(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveActiveBitXor<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveActiveBitXor(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, 0);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum
*
*   Performs reduction operation across a wave and returns the result of the reduction (sum of all threads in a wave)
*   to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveClusterSum(float src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveClusterSum(float2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveClusterSum(float3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveClusterSum(float4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterSum(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterSum(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterSum(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterSum(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterSum(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterSum(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterSum(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterSum<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterSum(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_AddU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct
*
*   Performs reduction operation across a wave and returns the result of the reduction (product of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveClusterProduct(float src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveClusterProduct(float2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveClusterProduct(float3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveClusterProduct(float4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterProduct(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterProduct(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterProduct(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterProduct(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterProduct(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterProduct(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterProduct(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterProduct<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterProduct(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MulU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin
*
*   Performs reduction operation across a wave and returns the result of the reduction (minimum of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveClusterMin(float src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveClusterMin(float2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveClusterMin(float3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveClusterMin(float4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterMin(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterMin(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterMin(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterMin(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterMin(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterMin(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterMin(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMin<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterMin(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MinU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax
*
*   Performs reduction operation across a wave and returns the result of the reduction (maximum of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveClusterMax(float src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveClusterMax(float2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveClusterMax(float3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveClusterMax(float4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxF, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterMax(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterMax(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterMax(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterMax(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxI, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterMax(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterMax(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterMax(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterMax<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterMax(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_MaxU, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd
*
*   Performs reduction operation across a wave and returns the result of the reduction (Bitwise AND of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitAnd<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterBitAnd(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_And, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr
*
*   Performs reduction operation across a wave and returns the result of the reduction (Bitwise OR of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterBitOr(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterBitOr(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterBitOr(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterBitOr(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterBitOr(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterBitOr(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterBitOr(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitOr<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterBitOr(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Or, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor
*
*   Performs reduction operation across a wave and returns the result of the reduction (Bitwise XOR of all threads in a
*   wave) to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveClusterBitXor(int src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveClusterBitXor(int2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveClusterBitXor(int3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveClusterBitXor(int4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveClusterBitXor(uint src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WaveClusterBitXor(uint2 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WaveClusterBitXor(uint3 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveClusterBitXor<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WaveClusterBitXor(uint4 src, uint clusterSize)
{
    return AmdExtD3DShaderIntrinsics_WaveReduce(AmdExtD3DShaderIntrinsicsWaveOp_Xor, src, clusterSize);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum
*
*   Performs a prefix (exclusive) scan operation across a wave and returns the resulting sum to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePrefixSum(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePrefixSum(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePrefixSum(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePrefixSum(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePrefixSum(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePrefixSum(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePrefixSum(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePrefixSum(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePrefixSum(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePrefixSum(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePrefixSum(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixSum<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePrefixSum(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct
*
*   Performs a prefix scan operation across a wave and returns the resulting product to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePrefixProduct(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePrefixProduct(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePrefixProduct(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePrefixProduct(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePrefixProduct(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePrefixProduct(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePrefixProduct(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePrefixProduct(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePrefixProduct(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePrefixProduct(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePrefixProduct(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixProduct<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePrefixProduct(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin
*
*   Performs a prefix scan operation across a wave and returns the resulting minimum value to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePrefixMin(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePrefixMin(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePrefixMin(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePrefixMin(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePrefixMin(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePrefixMin(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePrefixMin(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePrefixMin(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePrefixMin(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePrefixMin(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePrefixMin(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMin<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePrefixMin(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax
*
*   Performs a prefix scan operation across a wave and returns the resulting maximum value to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePrefixMax(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePrefixMax(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePrefixMax(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePrefixMax(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePrefixMax(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePrefixMax(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePrefixMax(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePrefixMax(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePrefixMax(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePrefixMax(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePrefixMax(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePrefixMax<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePrefixMax(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Exclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum
*
*   Performs a Postfix (Inclusive) scan operation across a wave and returns the resulting sum to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePostfixSum(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePostfixSum(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePostfixSum(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePostfixSum(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePostfixSum(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePostfixSum(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePostfixSum(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePostfixSum(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePostfixSum(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePostfixSum(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePostfixSum(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixSum<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePostfixSum(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_AddU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct
*
*   Performs a Postfix scan operation across a wave and returns the resulting product to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePostfixProduct(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePostfixProduct(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePostfixProduct(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePostfixProduct(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePostfixProduct(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePostfixProduct(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePostfixProduct(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePostfixProduct(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePostfixProduct(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePostfixProduct(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePostfixProduct(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixProduct<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePostfixProduct(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MulU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin
*
*   Performs a Postfix scan operation across a wave and returns the resulting minimum value to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePostfixMin(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePostfixMin(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePostfixMin(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePostfixMin(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePostfixMin(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePostfixMin(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePostfixMin(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePostfixMin(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePostfixMin(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePostfixMin(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePostfixMin(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMin<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePostfixMin(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MinU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax
*
*   Performs a Postfix scan operation across a wave and returns the resulting maximum value to all participating lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_WaveScan) returned S_OK.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WavePostfixMax(float src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<float2>
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WavePostfixMax(float2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<float3>
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WavePostfixMax(float3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<float4>
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WavePostfixMax(float4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxF,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<int>
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WavePostfixMax(int src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<int2>
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WavePostfixMax(int2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<int3>
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WavePostfixMax(int3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<int4>
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WavePostfixMax(int4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxI,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<uint>
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WavePostfixMax(uint src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<uint2>
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_WavePostfixMax(uint2 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<uint3>
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_WavePostfixMax(uint3 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WavePostfixMax<uint4>
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_WavePostfixMax(uint4 src)
{
    return AmdExtD3DShaderIntrinsics_WaveScan(AmdExtD3DShaderIntrinsicsWaveOp_MaxU,
                                              AmdExtD3DShaderIntrinsicsWaveOp_Inclusive,
                                              src);
}
#endif // _AMDEXTD3DSHADERINTRINICSWAVE_HLSL
