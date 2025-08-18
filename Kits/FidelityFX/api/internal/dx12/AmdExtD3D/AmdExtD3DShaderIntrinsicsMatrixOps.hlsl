/* Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef _AMDEXTD3DSHADERINTRINICSMATRIXOPS_HLSL
#define _AMDEXTD3DSHADERINTRINICSMATRIXOPS_HLSL

/**
***********************************************************************************************************************
* @brief WaveMatrix type
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixType_A             0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixType_B             0x1
#define AmdExtD3DShaderIntrinsicsWaveMatrixType_Accumulator   0x2

/**
***********************************************************************************************************************
* @brief Matrix element data format.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4    0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4    0x1
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8    0x2
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8    0x3
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16   0x4    // E5M10
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16  0x5    // E8M7
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32   0x6
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32   0x7
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U32   0x8
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8   0x9    // E5M2
#define AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8   0xa    // E4M3

/**
***********************************************************************************************************************
* @brief WaveMatrix object register numbers
***********************************************************************************************************************
*/
struct AmdMatrixContainer
{
    uint r[8];
};

/**
***********************************************************************************************************************
* @brief Supported packed types and shader representation
***********************************************************************************************************************
*/
using amd_uint4_t8_packed = uint32_t; // 8 packed uint4_t values in a uint32_t
using amd_int4_t8_packed  = uint32_t; // 8 packed int4_t values in a uint32_t
using amd_fp8_t4_packed   = uint32_t; // 4 packed fp8 values in a uint32_t
using amd_bf8_t4_packed   = uint32_t; // 4 packed bf8 values in a uint32_t

/**
***********************************************************************************************************************
* @brief AmdWaveMatrix type trait.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType> struct AmdWaveMatrix;
template <typename AmdWaveMatrix> struct AmdWaveMatrixTrait;

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix< AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, DIMM, DIMN, matrixType> >
{
    using elementType = half;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, DIMM, DIMN, matrixType> >
{
    using elementType = float;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32, DIMM, DIMN, matrixType> >
{
    using elementType = int;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16, DIMM, DIMN, matrixType> >
{
    using elementType = uint;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8, DIMM, DIMN, matrixType> >
{
    using elementType = int8_t4_packed;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8, DIMM, DIMN, matrixType> >
{
    using elementType = uint8_t4_packed;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8, DIMM, DIMN, matrixType> >
{
    using elementType = amd_bf8_t4_packed;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, DIMM, DIMN, matrixType> >
{
    using elementType = amd_fp8_t4_packed;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4, DIMM, DIMN, matrixType> >
{
    using elementType = amd_int4_t8_packed;
};

template <uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrixTrait <AmdWaveMatrix<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4, DIMM, DIMN, matrixType> >
{
    using elementType = amd_uint4_t8_packed;
};

/**
***********************************************************************************************************************
* @brief Matrix type.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN>
using AmdWaveMatrixA = AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, AmdExtD3DShaderIntrinsicsWaveMatrixType_A>;

template <uint matrixDataFmt, uint DIMM, uint DIMN>
using AmdWaveMatrixB = AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, AmdExtD3DShaderIntrinsicsWaveMatrixType_B>;

template <uint matrixDataFmt, uint DIMM, uint DIMN>
using AmdWaveMatrixAccumulator = AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, AmdExtD3DShaderIntrinsicsWaveMatrixType_Accumulator>;

/**
***********************************************************************************************************************
* @brief WaveMatrix Object Class
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
struct AmdWaveMatrix {
    using elementType = typename AmdWaveMatrixTrait<AmdWaveMatrix>::elementType;

    // Fixed eight register for current matrix.
    AmdMatrixContainer container;

    // Loading matrix object from srv, raw, global buffer.
    void Load(ByteAddressBuffer srv, uint startOffsetInBytes, uint strideInBytes, bool bColMajor = false);
    void Load(RWByteAddressBuffer uav, uint startOffsetInBytes, uint strideInBytes, bool bColMajor = false);
#if AMD_AGS_WMMA_INTERNAL
    void Load(uint addr_lo, uint addr_hi, uint startOffsetInBytes, uint strideInBytes, bool bColMajor = false);
#endif

    // Storing matrix object to raw buffer, global buffer.
    void Store(RWByteAddressBuffer uav, uint startOffsetInBytes, uint strideInBytes, bool bColMajor = false);
#if AMD_AGS_WMMA_INTERNAL
    void Store(uint addr_lo, uint addr_hi, uint startOffsetInBytes, uint strideInBytes, bool bColMajor = false);
#endif

#if AMD_AGS_WMMA_INTERNAL
    // Loading matrix sparsity index from srv, raw, global buffer.
    uint LoadSparsityIndex(ByteAddressBuffer srv, uint startOffsetInBytes);
    uint LoadSparsityIndex(RWByteAddressBuffer uav, uint startOffsetInBytes);
    uint LoadSparsityIndex(uint addr_lo, uint addr_hi, uint startOffsetInBytes);
#endif

    // Number of components of a matrix type accessible to the current thread
    uint Length();

    // Initialize the matrix with scalar value.
    void Fill(elementType data);

    // Return component i value
    elementType Element(uint i);

    // Set data to component i
    void SetElement(uint i, elementType data);

    // Copy data from input matrix, this might do type conversion or potential transpose.
    template <uint srcMatrixDataFmt, uint srcMatrixType>
    void Copy(AmdWaveMatrix<srcMatrixDataFmt, DIMM, DIMN, srcMatrixType> input);

    // Copy data from input matrix, this might do type conversion or potential transpose.
    // Conversions from higher to lower precision float formats will saturate to [MIN|MAX]_F*
    // instead of Inf when overflow occurs.
    template <uint srcMatrixDataFmt, uint srcMatrixType>
    void CopySat(AmdWaveMatrix<srcMatrixDataFmt, DIMM, DIMN, srcMatrixType> input);

    // Return the matrix data format(INT4, INT8, F16, F32)
    uint GetMatrixDataFmt() { return matrixDataFmt; }

    // Return matrix dimension M
    uint GetDimm() { return DIMM; }

    // Return matrix dimension N
    uint GetDimn() { return DIMN; }

    // Return matrix type(A, B, Accumulator)
    uint GetMatrixType() { return matrixType; }

    // Operator overloading to enable elementwise operations
    AmdWaveMatrix operator+(AmdWaveMatrix mat);
    AmdWaveMatrix operator-(AmdWaveMatrix mat);
    AmdWaveMatrix operator*(AmdWaveMatrix mat);
    AmdWaveMatrix operator/(AmdWaveMatrix mat);

    // Matrix times a scalar value
    // Every element is multiplied by the scalar
    AmdWaveMatrix operator*(elementType scalar);
};

/**
***********************************************************************************************************************
* @brief Load data from groupshared memory
***********************************************************************************************************************
*/
#define AMD_GROUPSHARED_LOAD(matrix_object, gsArray, offsetInArrayElements, strideInElements, bColMajor) \
do \
{ \
    uint __ref_variable; \
    uint inst = MakeAmdShaderIntrinsicsInstruction( \
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsLoad, AmdExtD3DShaderIntrinsicsOpcodePhase_0, 0); \
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, offsetInArrayElements, strideInElements, __ref_variable); \
    InterlockedAdd(gsArray[__ref_variable], 0, __ref_variable); \
    matrix_object.container = AmdExtD3DShaderIntrinsics_WaveMatrixGroupsharedLoad( \
        __ref_variable, matrix_object.GetDimm(), matrix_object.GetDimn(), matrix_object.GetMatrixType(), matrix_object.GetMatrixDataFmt(), bColMajor); \
} while(0)

/**
***********************************************************************************************************************
* @brief Store data to groupshared memory
***********************************************************************************************************************
*/
#define AMD_GROUPSHARED_STORE(matrix_object, gsArray, offsetInArrayElements, strideInElements, bColMajor) \
do \
{ \
    uint __ref_variable; \
    uint inst = MakeAmdShaderIntrinsicsInstruction( \
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore, AmdExtD3DShaderIntrinsicsOpcodePhase_0, 0); \
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, offsetInArrayElements, strideInElements, __ref_variable); \
    InterlockedAdd(gsArray[__ref_variable], 0, __ref_variable); \
    AmdExtD3DShaderIntrinsics_WaveMatrixGroupsharedStore( \
        __ref_variable, matrix_object.GetDimm(), matrix_object.GetDimn(), matrix_object.container, \
        matrix_object.GetMatrixType(), matrix_object.GetMatrixDataFmt(), bColMajor); \
} while(0)

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
* @brief Loading matrix sparsity index from groupshared memory
***********************************************************************************************************************
*/
#define AMD_GROUPSHARED_SPARSITY_INDEX_LOAD(matrix_object, gsArray, offsetInArrayElements, index) \
do \
{ \
    if (matrix_object.GetMatrixType() != AmdExtD3DShaderIntrinsicsWaveMatrixType_A) \
    { \
        index = 0; \
    } \
    uint __ref_variable = AmdExtD3DMatrix_MatrixSparsityIndexLoadPhase0(AmdExtD3DShaderIntrinsicsSparsityIndexMem_GroupShared, 0, 0); \
    InterlockedAdd(gsArray[__ref_variable], 0, __ref_variable); \
    uint shape = AmdExtD3DMatrix_MatrixShape(matrix_object.GetDimm(), matrix_object.GetDimn()); \
    index = AmdExtD3DMatrix_MatrixSparsityIndexLoad( \
        matrix_object.GetMatrixDataFmt(), AmdExtD3DShaderIntrinsicsSparsityIndexMem_GroupShared, shape, offsetInArrayElements, __ref_variable); \
} while(0)
#endif

/**
***********************************************************************************************************************
* @brief WaveMatrix Multiply
***********************************************************************************************************************
*/
template <uint matrixDataFmtA, uint matrixDataFmtB, uint matrixDataFmtAccumulator, uint DIMM, uint DIMN, uint DIMK>
AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> AmdWaveMatrixMultiply
    (AmdWaveMatrixA<matrixDataFmtA, DIMM, DIMK> matA,
     AmdWaveMatrixB<matrixDataFmtB, DIMK, DIMN> matB,
     AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> matAccumulator);

#if AMD_AGS_WMMA_INTERNAL
template <uint matrixDataFmtA, uint matrixDataFmtB, uint matrixDataFmtAccumulator, uint DIMM, uint DIMN, uint DIMX, uint DIMY>
AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> AmdWaveMatrixMultiply
    (AmdWaveMatrixA<matrixDataFmtA, DIMM, DIMX> matA,
     AmdWaveMatrixB<matrixDataFmtB, DIMY, DIMN> matB,
     AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> matAccumulator,
     uint index);
#endif

/**
***********************************************************************************************************************
* @brief Conversion and Packing function
***********************************************************************************************************************
*/
// Produce fp8 packed values from 4 f32 input values, overflow produces NaN
amd_fp8_t4_packed amd_pack_fp8(float32_t4 values);

// Produce fp8 packed values from 4 f32 input values, saturating overflowed values
amd_fp8_t4_packed amd_pack_sat_fp8(float32_t4 values);

// Unpack to f32 values
float32_t4 amd_unpack_fp8(amd_fp8_t4_packed value);

// Produce bf8 packed values from 4 f32 input values, overflow produced Inf
amd_bf8_t4_packed amd_pack_bf8(float32_t4 values);

// Produce bf8 packed values from 4 f32 input values, saturating overflowed values
amd_bf8_t4_packed amd_pack_sat_bf8(float32_t4 values);

// Unpack to f32 values
float32_t4 amd_unpack_bf8(amd_bf8_t4_packed value);

// [lo, high] Drop high bits
amd_int4_t8_packed amd_pack_s4(int32_t4 a, int32_t4 b);

// [lo, high] Drop high bits
amd_uint4_t8_packed amd_pack_u4(uint32_t4 a, uint32_t4 b);

// [lo, high] clamp to [-8, 7]
amd_int4_t8_packed amd_pack_clamp_s4(int32_t4 a, int32_t4 b);

// [lo, high] clamp to [0, 15]
amd_uint4_t8_packed amd_pack_clamp_u4(uint32_t4 a, uint32_t4 b);

#include "AmdExtD3DShaderIntrinsicsMatrixOpsHelper.hlsl"

#endif // _AMDEXTD3DSHADERINTRINICSMATRIXOPS_HLSL
