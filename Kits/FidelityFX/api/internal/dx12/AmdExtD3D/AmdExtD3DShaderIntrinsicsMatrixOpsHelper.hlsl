/* Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef _AMDEXTD3DSHADERINTRINICSMATRIXOPSHELPER_HLSL
#define _AMDEXTD3DSHADERINTRINICSMATRIXOPSHELPER_HLSL

#include "AmdExtD3DShaderIntrinsics.hlsl"

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixOpcode defines for supported WMMA matrix multiplication operation.
*   To be used as the operation parameter for the AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_BF16_16X16X16_BF16     0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F16_16X16X16_F16       0x1
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF16      0x2
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF8_BF8   0x3
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF8_FP8   0x4
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_F16       0x5
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_FP8_BF8   0x6
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_FP8_FP8   0x7
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_I4        0x8
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_U4        0x9
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_IU4       0xa
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_UI4       0xb
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_I8        0xc
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_U8        0xd
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_IU8       0xe
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_UI8       0xf
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_I4        0x10
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_U4        0x11
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_IU4       0x12
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_UI4       0x13

#if AMD_AGS_WMMA_INTERNAL
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_BF16_16X16X32_BF16    0x14
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F16_16X16X32_F16      0x15
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF16     0x16
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF8_BF8  0x17
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF8_FP8  0x18
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_F16      0x19
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_FP8_BF8  0x1a
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_FP8_FP8  0x1b
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_I4       0x1c
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_U4       0x1d
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_IU4      0x1e
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_UI4      0x1f
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_I8       0x20
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_U8       0x21
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_IU8      0x22
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_UI8      0x23
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_I4       0x24
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_U4       0x25
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_IU4      0x26
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_UI4      0x27
#endif

#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED    0x28

/**
***********************************************************************************************************************
*   To be used as the operation parameter for the AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInstructions.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_OpsShift   0
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_OpsMask    0x7f
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_FlagShift  15
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_FlagMask   0x1

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixOpcode defines for specified wmma or swmma.
*   To be used as the operation parameter for the AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInstructions.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA   0

#if AMD_AGS_WMMA_INTERNAL
#define AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA  1
#endif

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixInOut defines specify the index of input or output data.
*   To be used as the operation parameter: AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInputInstructions
*                                          AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixOutputInstructions
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift         0
#define AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelMask          0xf
#define AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift   4
#define AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagMask    0xf
#define AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagShift  8
#define AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagMask   0xff

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixModifier defines are used to encode inst modifier.
*   To be used as the operation parameter for AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_DataFormatFlagShift  0
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_DataFormatFlagMask   0xf
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTypeFlagShift  4
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTypeFlagMask   0x7
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_LayoutFlagShift      7
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_LayoutFlagMask       0x1
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_ShapeShift           8
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_ShapeMask            0x7
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_IndexMemTypeShift    14
#define AmdExtD3DShaderIntrinsicsWaveMatrixModifier_IndexMemTypeMask     0x3

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex defines incate which register to used.
*   To be used as the operation parameter for wmma intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex_FirstReg   0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex_SecondReg  0x1

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixRegType defines incate register type.
*   To be used as the operation parameter for wmma intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg          0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixRegType_A_TempReg           0x1
#define AmdExtD3DShaderIntrinsicsWaveMatrixRegType_B_TempReg           0x2
#define AmdExtD3DShaderIntrinsicsWaveMatrixRegType_Accumulator_TempReg 0x3

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixLayout defines indicate the matrix data layout in memory.
*   To be used as the operation parameter for load/store intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixLayout_RowMajor     0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixLayout_ColumnMajor  0x1

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixConversion defines whether saturation falg is set.
*   To be used as the operation parameter for copy intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixConversion_SatToFmaxNone   0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixConversion_SatToFmax       0x1

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveMatrixShape defines indicate the matrix size.
*   To be used as the operation parameter for load/store intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X16  0x0
#define AmdExtD3DShaderIntrinsicsWaveMatrixShape_32X16  0x1
#define AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X32  0x2
#define AmdExtD3DShaderIntrinsicsWaveMatrixShape_64X16  0x3

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsSparsityIndexMem defines indicate index memory class.
*   To be used as the memory type parameter for sparsity index load intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsSparsityIndexMem_UavBuffer     0
#define AmdExtD3DShaderIntrinsicsSparsityIndexMem_GroupShared   1
#define AmdExtD3DShaderIntrinsicsSparsityIndexMem_GlobalBuffer  2
#endif

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsMatrixElementWiseOp defines element-wise arithmetic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Add    0x1
#define AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Sub    0x2
#define AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Mul    0x3
#define AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Div    0x4
#define AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Times  0x5

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsFloat8CvtOp defines for supported float8 conversion operation.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsFloat8CvtOp_FP8_2_F32   0x0
#define AmdExtD3DShaderIntrinsicsFloat8CvtOp_BF8_2_F32   0x1
#define AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_FP8   0x2
#define AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_BF8   0x3

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsFloat8Conversion defines specify conversion opcode saturation.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsFloat8Conversion_CvtOpShift   0
#define AmdExtD3DShaderIntrinsicsFloat8Conversion_CvtOpMask    0xff
#define AmdExtD3DShaderIntrinsicsFloat8Conversion_SatShift     8
#define AmdExtD3DShaderIntrinsicsFloat8Conversion_SatMask      0x1

/**
***********************************************************************************************************************
*   Type bitcast fuction.
***********************************************************************************************************************
*/
uint reinterpret_as_uint32(float value)
{
    return asuint(value);
}

uint reinterpret_as_uint32(half value)
{
    return asuint16(value);
}

uint reinterpret_as_uint32(uint value)
{
    return value;
}

uint reinterpret_as_uint32(int value)
{
    return asuint(value);
}

uint reinterpret_as_uint32(uint8_t4_packed value)
{
    return value;
}

uint reinterpret_as_uint32(int8_t4_packed value)
{
    return value;
}

float reinterpret_as_element_type(uint uVal, uint mdfmt, float typ_flag)
{
    return asfloat(uVal);
}

half reinterpret_as_element_type(uint uVal, uint mdfmt, half typ_flag)
{
    return asfloat16(uint16_t(uVal));
}

int reinterpret_as_element_type(uint uVal, uint mdfmt, int typ_flag)
{
    return asint(uVal);
}

uint reinterpret_as_element_type(uint uVal, uint mdfmt, uint typ_flag)
{
    return uVal;
}

uint8_t4_packed reinterpret_as_element_type(uint uVal, uint mdfmt, uint8_t4_packed typ_flag)
{
    return uVal;
}

int8_t4_packed reinterpret_as_element_type(uint uVal, uint mdfmt, int8_t4_packed typ_flag)
{
    return uVal;
}

/**
***********************************************************************************************************************
*   Type convert fuction.
***********************************************************************************************************************
*/
float AmdFormatBf16ToFp32(uint bf16_e8m7)
{
    return asfloat(bf16_e8m7 << 16);
}

uint AmdFormatFp32ToBf16(float fp32_e8m23)
{
    if (isnan(fp32_e8m23))
    {
        return 0x7FC0;
    }
    uint u32 = asuint(fp32_e8m23);
    uint bias = ((u32 >> 16) & 1) + 0x7FFF;
    return (u32 + bias) >> 16;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions
*
*   Creates float8 conversion instruction from supplied cvtOp, saturation flag.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Float8Conversion) returned S_OK.
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(uint cvtOp, uint sat)
{
    uint4 insts;
    const uint immed = (cvtOp << AmdExtD3DShaderIntrinsicsFloat8Conversion_CvtOpShift) |
                       (sat << AmdExtD3DShaderIntrinsicsFloat8Conversion_SatShift);
    insts.x = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Float8Conversion, 0, immed);
    insts.y = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Float8Conversion, 0, immed);
    insts.z = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Float8Conversion, 0, immed);
    insts.w = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Float8Conversion, 0, immed);
    return insts;
}

/**
***********************************************************************************************************************
*   Conversion and packing function
***********************************************************************************************************************
*/
// Produce fp8 packed values from 4 f32 input values, overflow produces NaN
amd_fp8_t4_packed amd_pack_fp8(float32_t4 values)
{
    uint4 retVal;
    uint4 insts = AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_FP8, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, reinterpret_as_uint32(values.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, reinterpret_as_uint32(values.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, reinterpret_as_uint32(values.z), 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, reinterpret_as_uint32(values.w), 0, retVal.w);
    return pack_u8(retVal);
}

// Produce fp8 packed values from 4 f32 input values, saturating overflowed values
amd_fp8_t4_packed amd_pack_sat_fp8(float32_t4 values)
{
    uint4 retVal;
    uint4 insts = AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_FP8, 1);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, reinterpret_as_uint32(values.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, reinterpret_as_uint32(values.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, reinterpret_as_uint32(values.z), 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, reinterpret_as_uint32(values.w), 0, retVal.w);
    return pack_u8(retVal);
}

// Unpack to f32 values
float32_t4 amd_unpack_fp8(amd_fp8_t4_packed value)
{
    uint32_t4 retVal;
    uint4 bytes = unpack_u8u32(value);
    uint4 insts = AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(AmdExtD3DShaderIntrinsicsFloat8CvtOp_FP8_2_F32, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, bytes.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, bytes.y, 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, bytes.z, 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, bytes.w, 0, retVal.w);
    return float32_t4(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z), asfloat(retVal.w));
}

// Produce bf8 packed values from 4 f32 input values, overflow produced Inf
amd_bf8_t4_packed amd_pack_bf8(float32_t4 values)
{
    uint4 retVal;
    uint4 insts = AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_BF8, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, reinterpret_as_uint32(values.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, reinterpret_as_uint32(values.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, reinterpret_as_uint32(values.z), 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, reinterpret_as_uint32(values.w), 0, retVal.w);
    return pack_u8(retVal);
}

// Produce bf8 packed values from 4 f32 input values, saturating overflowed values
amd_bf8_t4_packed amd_pack_sat_bf8(float32_t4 values)
{
    uint4 retVal;
    uint4 insts = AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_BF8, 1);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, reinterpret_as_uint32(values.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, reinterpret_as_uint32(values.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, reinterpret_as_uint32(values.z), 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, reinterpret_as_uint32(values.w), 0, retVal.w);
    return pack_u8(retVal);
}

// Unpack to f32 values
float32_t4 amd_unpack_bf8(amd_bf8_t4_packed value)
{
    uint32_t4 retVal;
    uint4 bytes = unpack_u8u32(value);
    uint4 insts = AmdExtD3DShaderIntrinsics_MakeFloat8ConversionInstructions(AmdExtD3DShaderIntrinsicsFloat8CvtOp_BF8_2_F32, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, bytes.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, bytes.y, 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, bytes.z, 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, bytes.w, 0, retVal.w);
    return float32_t4(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z), asfloat(retVal.w));
}

// [lo, high] Drop high bits
amd_int4_t8_packed amd_pack_s4(int32_t4 a, int32_t4 b)
{
    // Pack odd and even element of amd_int4_t8_packed separately
    uint32_t4 low  = { a.x, a.z, b.x, b.z };
    uint32_t4 high = { a.y, a.w, b.y, b.w };
    return (pack_u8(low) & 0x0f0f0f0f) |
            ((pack_u8(high) & 0x0f0f0f0f) << 4);
}

// [lo, high] Drop high bits
amd_uint4_t8_packed amd_pack_u4(uint32_t4 a, uint32_t4 b)
{
    uint32_t4 low  = { a.x, a.z, b.x, b.z };
    uint32_t4 high = { a.y, a.w, b.y, b.w };
    return (pack_u8(low) & 0x0f0f0f0f) |
            ((pack_u8(high) & 0x0f0f0f0f) << 4);
}

// [lo, high] clamp to [-8, 7]
amd_int4_t8_packed amd_pack_clamp_s4(int32_t4 a, int32_t4 b)
{
    uint32_t4 low =  { clamp(a.x, -8, 7),
                       clamp(a.z, -8, 7),
                       clamp(b.x, -8, 7),
                       clamp(b.z, -8, 7) };
    uint32_t4 high = { clamp(a.y, -8, 7),
                       clamp(a.w, -8, 7),
                       clamp(b.y, -8, 7),
                       clamp(b.w, -8, 7) };
    return (pack_u8(low) & 0x0f0f0f0f) |
            ((pack_u8(high) & 0x0f0f0f0f) << 4);
}

// [lo, high] clamp to [0, 15]
amd_uint4_t8_packed amd_pack_clamp_u4(uint32_t4 a, uint32_t4 b)
{
    uint32_t4 low  = { min(a.x, 15),
                       min(a.z, 15),
                       min(b.x, 15),
                       min(b.z, 15) };
    uint32_t4 high = { min(a.y, 15),
                       min(a.w, 15),
                       min(b.y, 15),
                       min(b.w, 15) };
    return (pack_u8(low) & 0x0f0f0f0f) |
            ((pack_u8(high) & 0x0f0f0f0f) << 4);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInstructions
*
*   Creates WaveMatrixMultiplyAccumulate (WMMA) instruction from supplied wmmaOp, flag and phase.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInstructions(uint wmmaOp, uint flag, uint phase)
{
    uint instruction;
    const uint immed = (wmmaOp << AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_OpsShift) |
                       (flag << AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_FlagShift);

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc, phase, immed);
    return instruction;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding
*
*   Encoding matrix configuration infomation.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(
    uint type, uint format, uint major, uint shape)
{
    return ((format << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_DataFormatFlagShift) |
            (type   << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTypeFlagShift) |
            (major  << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_LayoutFlagShift) |
            (shape  << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_ShapeShift));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions
*
*   Creates uint2 with x/y components containing matrix configuration infomation for load/store instruction.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions(
    uint op, uint2 phase, uint type, uint format, uint major, uint shape)
{
    uint2 instruction;
    uint immed = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, major, shape);

    instruction.x = MakeAmdShaderIntrinsicsInstruction(op, phase.x, 0);
    instruction.y = MakeAmdShaderIntrinsicsInstruction(op, phase.y, immed);

    return instruction;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInputInstructions
*
*   Creates uint4 instructions support input matrix source value.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInputInstructions(uint op, uint phase, uint type)
{
    uint4 instruction;
    uint4 immed;
    const uint2 writeZW = { 0, 1 };
    const uint2 reg = { AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex_FirstReg,
                        AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex_SecondReg };

    immed.x = (writeZW.x << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (reg.x << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift) |
              (type << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagShift);
    immed.y = (writeZW.y << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (reg.x << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift) |
              (type << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagShift);
    immed.z = (writeZW.x << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (reg.y << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift) |
              (type << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagShift);
    immed.w = (writeZW.y << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (reg.y << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift) |
              (type << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagShift);

    instruction.x = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.x);
    instruction.y = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.y);
    instruction.z = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.z);
    instruction.w = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.w);

    return instruction;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixOutputInstructions
*
*   Creates uint4 instructions support output matrix return value.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixOutputInstructions(uint op, uint flag, uint phase)
{
    uint4 instruction;
    uint4 immed;
    const uint4 subloc = { 0, 1, 2, 3 };

    immed.x = (subloc.x << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (flag << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift);
    immed.y = (subloc.y << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (flag << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift);
    immed.z = (subloc.z << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (flag << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift);
    immed.w = (subloc.w << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift) |
              (flag << AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift);

    instruction.x = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.x);
    instruction.y = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.y);
    instruction.z = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.z);
    instruction.w = MakeAmdShaderIntrinsicsInstruction(op, phase, immed.w);

    return instruction;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_InputMatrix
*
*   Save matrix value.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_InputMatrix(
    uint matOpc, uint phase, uint regType, AmdMatrixContainer srcMat)
{
    uint4 insts;
    uint retVal;
    insts = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInputInstructions(matOpc, phase, regType);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.x, srcMat.r[0], srcMat.r[1], retVal);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.y, srcMat.r[2], srcMat.r[3], retVal);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.z, srcMat.r[4], srcMat.r[5], retVal);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(insts.w, srcMat.r[6], srcMat.r[7], retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_OutputMatrix
*
*   Create matrix container.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_OutputMatrix(uint matOpc, uint phase)
{
    uint4 outInsts;
    AmdMatrixContainer AmdMat;
    outInsts = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixOutputInstructions(
                    matOpc, AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex_FirstReg, phase);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.x, 0, 0, AmdMat.r[0]);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.y, 0, 0, AmdMat.r[1]);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.z, 0, 0, AmdMat.r[2]);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.w, 0, 0, AmdMat.r[3]);
    outInsts = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixOutputInstructions(
                    matOpc, AmdExtD3DShaderIntrinsicsWaveMatrixRegIndex_SecondReg, phase);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.x, 0, 0, AmdMat.r[4]);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.y, 0, 0, AmdMat.r[5]);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.z, 0, 0, AmdMat.r[6]);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(outInsts.w, 0, 0, AmdMat.r[7]);

    return AmdMat;
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixMulAcc
*
*   Perform WMMA operation and return the alu result.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_WaveMatrixMulAcc(
    uint wmmaOp, AmdMatrixContainer AmdmatA, AmdMatrixContainer AmdmatB, AmdMatrixContainer AmdmatAccumulator)
{
    AmdMatrixContainer retVal;
    uint inst;
    // input matrix A
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_A_TempReg,
                                          AmdmatA);
    // input matrix B
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_B_TempReg,
                                          AmdmatB);
    // input matrix Accumulator
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_Accumulator_TempReg,
                                          AmdmatAccumulator);
    // generate wmma amdil
    inst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInstructions(
        wmmaOp, AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA, AmdExtD3DShaderIntrinsicsOpcodePhase_1);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, 0, 0, retVal.r[0]);
    // return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);

    return retVal;
}

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixMulAcc
*
*   Perform SWMMA operation and return the alu result.
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_WaveMatrixMulAcc(
    uint swmmaOp, AmdMatrixContainer AmdmatA, AmdMatrixContainer AmdmatB, AmdMatrixContainer AmdmatAccumulator, uint index)
{
    AmdMatrixContainer retVal;
    uint inst;
    // input matrix A
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_A_TempReg,
                                          AmdmatA);
    // input matrix B
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_B_TempReg,
                                          AmdmatB);
    // input matrix Accumulator
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_Accumulator_TempReg,
                                          AmdmatAccumulator);
    // generate swmma amdil
    inst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixInstructions(
        swmmaOp, AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA, AmdExtD3DShaderIntrinsicsOpcodePhase_1);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, 0, index, retVal.r[0]);
    // return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);

    return retVal;
}
#endif

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixLoad
*
*   Read matrix data from the buffer (UAV, SRV, Global) at the specified address.
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   Read matrix data from the buffer UAV at the specified address.
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_WaveMatrixLoad(
    RWByteAddressBuffer uav, uint offset, uint stride, uint type, uint format, uint layout, uint shape)
{
    AmdMatrixContainer retVal;
    uint2 ininst;
    uint loadVal;
    uint tmpVal;
    uint2 phase = {AmdExtD3DShaderIntrinsicsOpcodePhase_0, AmdExtD3DShaderIntrinsicsOpcodePhase_1};
    // phase 0: set offset and stride
    ininst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavLoad, phase, type, format, layout, shape);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(ininst.x, offset, stride, tmpVal);
    loadVal = uav.Load(tmpVal);
    // phase 1: generate matrix_load amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(ininst.y, loadVal, 0, tmpVal);
    // phase 2: return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavLoad,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);

    return retVal;
}

/**
***********************************************************************************************************************
*   Read matrix data from the buffer SRV at the specified address.
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_WaveMatrixLoad(
    ByteAddressBuffer srv, uint offset, uint stride, uint type, uint format, uint layout, uint shape)
{
    AmdMatrixContainer retVal;
    uint2 ininst;
    uint4 outinst;
    uint loadVal;
    uint tmpVal;
    uint2 phase = { AmdExtD3DShaderIntrinsicsOpcodePhase_0, AmdExtD3DShaderIntrinsicsOpcodePhase_1 };
    // phase 0: set offset and stride
    ininst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavLoad, phase, type, format, layout, shape);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(ininst.x, offset, stride, tmpVal);
    loadVal = srv.Load(tmpVal);
    // phase 1: generate matrix_load amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(ininst.y, loadVal, 0, tmpVal);
    // phase 2: return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavLoad,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);

    return retVal;
}

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   Read matrix data from the Global buffer at the specified address.
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_WaveMatrixLoad(
    uint gpuVaLoBits, uint gpuVaHiBits, uint offset, uint stride, uint type, uint format, uint layout, uint shape)
{
    AmdMatrixContainer retVal;
    uint2 ininst;
    uint tmpVal;
    uint2 phase = { AmdExtD3DShaderIntrinsicsOpcodePhase_0, AmdExtD3DShaderIntrinsicsOpcodePhase_1 };
    // phase 0: set offset and stride
    ininst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalLoad, phase, type, format, layout, shape);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(ininst.x, offset, stride, tmpVal);
    // phase 1: generate matrix_load amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(ininst.y, gpuVaLoBits, gpuVaHiBits, tmpVal);
    // phase 2: return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalLoad,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);

    return retVal;
}
#endif

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixStore
*
*   Writes matrix data to the buffer (UAV, Global) at the specified address.
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_WaveMatrixStore(
    RWByteAddressBuffer uav,
    uint offset,
    uint stride,
    AmdMatrixContainer AmdMat,
    uint type,
    uint format,
    uint layout,
    uint shape)
{
    uint retVal;
    uint2 inst;
    uint2 phase = {AmdExtD3DShaderIntrinsicsOpcodePhase_0, AmdExtD3DShaderIntrinsicsOpcodePhase_2};
    // phase 0: set offset and stride
    inst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavStore, phase, type, format, layout, shape);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst.x, offset, stride, retVal);
    uint loadVal = uav.Load(retVal);
    // phase 1: input matrix container
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavStore,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg,
                                          AmdMat);
    // phase 2: generate matrix_store amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst.y, loadVal, 0, retVal);
}

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   Writes matrix data to Global buffer at the specified address.
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_WaveMatrixStore(
    uint gpuVaLoBits,
    uint gpuVaHiBits,
    uint offset,
    uint stride,
    AmdMatrixContainer AmdMat,
    uint type,
    uint format,
    uint layout,
    uint shape)
{
    uint retVal;
    uint2 inst;
    uint2 phase = { AmdExtD3DShaderIntrinsicsOpcodePhase_0, AmdExtD3DShaderIntrinsicsOpcodePhase_2 };
    // phase 0: set offset and stride
    inst = AmdExtD3DShaderIntrinsics_MakeSpecialWaveMatrixPropertyInstructions(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalStore, phase, type, format, layout, shape);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst.x, offset, stride, retVal);
    // phase 1: input matrix container
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalStore,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg,
                                          AmdMat);
    // phase 2: generate matrix_store amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst.y, gpuVaLoBits, gpuVaHiBits, retVal);
}
#endif

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixElementOperation
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveMatrixElementOperation(
    uint opcode,               // fill or extract
    AmdMatrixContainer AmdMat, // destination matrix
    uint index,                // matrix row/column index
    uint value,                // fill data
    uint type,                 // matrix A|B|Accumulator
    uint format,               // element data format {.f32, .f16, .i32, .u32, .i4, .i8, .u4, .u8}
    uint shape                 // matrix shape {16x16, 16x32, 32x16, 64x16}
)
{
    uint retVal;
    // phase 0: input matrix container
    AmdExtD3DShaderIntrinsics_InputMatrix(
        opcode, AmdExtD3DShaderIntrinsicsOpcodePhase_0, AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg, AmdMat);
    // phase 1: generate amdil
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, 0, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(opcode, AmdExtD3DShaderIntrinsicsOpcodePhase_1, matProp);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, index, value, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixElementFill
*
*   AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementFill indicates AmdMat[index] = value
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_WaveMatrixElementFill(
    inout AmdMatrixContainer AmdMat, // destination matrix
    uint index,                      // matrix row/column index
    uint value,                      // fill data
    uint type,                       // matrix A|B|Accumulator
    uint format,                     // element data format {.f32, .f16, .i32, .u32, .i4, .i8, .u4, .u8}
    uint shape
)
{
    uint4 outinst;
    AmdExtD3DShaderIntrinsics_WaveMatrixElementOperation(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementFill, AmdMat, index, value, type, format, shape);
    // phase 2: return destination matrix
    AmdMat = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementFill,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_CopyElementFromSourceMatrix
*
*   Initialize matrix element by other matrix
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_CopyElementFromSourceMatrix(
    AmdMatrixContainer AmdMat,  // source matrix
    uint src_type,              // matrix A|B|Accumulator
    uint src_format,            // element data format {.f32, .f16, .i32, .u32, .i4, .i8, .u4, .u8}
    uint dst_type,              // matrix A|B|Accumulator
    uint dst_format,            // element data format {.f32, .f16, .i32, .u32, .i4, .i8, .u4, .u8}
    uint shape,
    uint sat
)
{
    AmdMatrixContainer retVal;
    // phase 0: input src matrix container
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixCopy,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg,
                                          AmdMat);
    // phase 1: generate amdil
    uint srcProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(src_type, src_format, 0, shape);
    uint dstProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(dst_type, dst_format, 0, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixCopy, AmdExtD3DShaderIntrinsicsOpcodePhase_1, srcProp);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, dstProp, sat, retVal.r[0]);
    // phase 2: return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixCopy,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_2);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveMatrixLength
*
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_WaveMatrixLength(
    uint type,   // matrix A|B|Accumulator
    uint format, // element data format {.f32, .f16, .i32, .u32, .i4, .i8, .u4, .u8}
    uint shape
)
{
    uint retVal;
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, 0, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLength, AmdExtD3DShaderIntrinsicsOpcodePhase_0, matProp);
    // phase 0: generate amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, 0, 0, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DMatrix_ExtractComponent returns AmdMat[index].
***********************************************************************************************************************
*/
template <typename EleType>
EleType AmdExtD3DMatrix_ExtractComponent(AmdMatrixContainer AmdMat, uint index, uint mtype, uint mdfmt, EleType flag, uint shape)
{
    uint uVal = AmdExtD3DShaderIntrinsics_WaveMatrixElementOperation(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementExtract,
                                                                     AmdMat, index, 0, mtype, mdfmt, shape);
    return reinterpret_as_element_type(uVal, mdfmt, flag);
}

/**
***********************************************************************************************************************
*   AmdExtD3DMatrix_InsertComponent performs AmdMat[index] = value, and then return new matrix container.
***********************************************************************************************************************
*/
template <typename EleType>
void AmdExtD3DMatrix_InsertComponent(inout AmdMatrixContainer AmdMat, uint index, EleType data, uint mtype, uint mdfmt, uint shape)
{
    AmdExtD3DShaderIntrinsics_WaveMatrixElementFill(AmdMat, index, reinterpret_as_uint32(data), mtype, mdfmt, shape);
}

/**
***********************************************************************************************************************
*   AmdExtD3DMatrix_MatrixCopy performs AmdMat = AmdMatSrc.
***********************************************************************************************************************
*/
void AmdExtD3DMatrix_MatrixCopy(
    inout AmdMatrixContainer AmdMat, AmdMatrixContainer AmdMatSrc,
    uint src_mtype, uint src_mdfmt, uint dst_mtype, uint dst_mdfmt, uint shape, uint sat)
{
    AmdMat = AmdExtD3DShaderIntrinsics_CopyElementFromSourceMatrix(AmdMatSrc, src_mtype, src_mdfmt, dst_mtype, dst_mdfmt, shape, sat);
}

/**
***********************************************************************************************************************
*   AmdExtD3DMatrix_MatrixFill fill matrix with a scalar value.
***********************************************************************************************************************
*/
void AmdExtD3DMatrix_MatrixFill(inout AmdMatrixContainer AmdMat, uint value, uint mtype, uint mdfmt, uint shape)
{
    uint retVal;
    // phase 0: generate amdil
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(mtype, mdfmt, 0, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixFill, AmdExtD3DShaderIntrinsicsOpcodePhase_0, matProp);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, value, 0, retVal);
    // phase 1: return matrix container
    AmdMat = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixFill,
                                                    AmdExtD3DShaderIntrinsicsOpcodePhase_1);
}

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   AmdExtD3DMatrix_MatrixSparsityIndexLoad return sparsity index.
*
*   Uav/Srv/GroupShared generate dummy address for load or atomic
*   GlobalBuffer        store addr_hi and addr_lo
***********************************************************************************************************************
*/
uint AmdExtD3DMatrix_MatrixSparsityIndexLoadPhase0(uint mem_type, uint addr_lo, uint addr_hi)
{
    uint retVal;
    uint immed = (mem_type << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_IndexMemTypeShift);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_MatrixSparsityIndexLoad, AmdExtD3DShaderIntrinsicsOpcodePhase_0, immed);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, addr_lo, addr_hi, retVal);
    return retVal;
}

uint AmdExtD3DMatrix_MatrixSparsityIndexLoad(
    uint format,    // element format
    uint mem_type,  // memory type
    uint shape,     // matrix shape
    uint offset,    // 32-bit offset
    uint dummy_load // UAV(SRV, LDS) dummy load
)
{
    uint retVal;
    uint immed = ((format   << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_DataFormatFlagShift) |
                  (shape    << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_ShapeShift) |
                  (mem_type << AmdExtD3DShaderIntrinsicsWaveMatrixModifier_IndexMemTypeShift));
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_MatrixSparsityIndexLoad, AmdExtD3DShaderIntrinsicsOpcodePhase_1, immed);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, offset, dummy_load, retVal);
    return retVal;
}
#endif

/**
***********************************************************************************************************************
*   Query WMMA opcode
***********************************************************************************************************************
*/
void AmdExtD3DMatrix_MatrixMultiplyAccumulateOpcode_16x16x16(inout uint ops, uint AmdEleFmtA, uint AmdEleFmtB, uint AmdEleFmtAccumulator)
{
    ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED;

    switch (AmdEleFmtA)
    {
        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16:
                {
                switch (AmdEleFmtAccumulator)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F16_16X16X16_F16;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_F16;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16:
                {
                switch (AmdEleFmtAccumulator)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_BF16_16X16X16_BF16;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF16;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF8_BF8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF8_FP8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_FP8_BF8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_FP8_FP8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_I8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_IU8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_UI8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_U8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_I4;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_IU4;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_UI4;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_U4;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }
}

void AmdExtD3DMatrix_MatrixMultiplyAccumulateOpcode_16x16x32(inout uint ops, uint AmdEleFmtA, uint AmdEleFmtB, uint AmdEleFmtAccumulator)
{
    ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED;

    if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
    {
        if (AmdEleFmtA == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4)
        {
            if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_I4;
            }
            else if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_IU4;
            }
        }
        else if (AmdEleFmtA == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4)
        {
            if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_UI4;
            }
            else if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_U4;
            }
        }
    }
}

#if AMD_AGS_WMMA_INTERNAL
void AmdExtD3DMatrix_SparseMatrixMultiplyAccumulateOpcode_16x16x32(inout uint ops, uint AmdEleFmtA, uint AmdEleFmtB, uint AmdEleFmtAccumulator)
{
    ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED;

    switch (AmdEleFmtA)
    {
        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16:
                {
                switch (AmdEleFmtAccumulator)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F16_16X16X32_F16;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_F16;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16:
                {
                switch (AmdEleFmtAccumulator)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_BF16_16X16X32_BF16;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32:
                        if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF16;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF8_BF8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF8_FP8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_FP8_BF8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_FP8_FP8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_I8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_IU8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_UI8;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_U8;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_I4;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_IU4;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4:
                {
                switch (AmdEleFmtB)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_UI4;
                        }
                        break;

                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4:
                        if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
                        {
                            ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_U4;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }
}

void AmdExtD3DMatrix_SparseMatrixMultiplyAccumulateOpcode_16x16x64(inout uint ops, uint AmdEleFmtA, uint AmdEleFmtB, uint AmdEleFmtAccumulator)
{
    ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED;
    if (AmdEleFmtAccumulator == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32)
    {
        if (AmdEleFmtA == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4)
        {
            if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_I4;
            }
            else if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_IU4;
            }
        }
        else if (AmdEleFmtA == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4)
        {
            if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_UI4;
            }
            else if (AmdEleFmtB == AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4)
            {
                ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_U4;
            }
        }
    }
}
#endif

uint AmdExtD3DMatrix_MatrixShape(uint dimM, uint dimN)
{
    uint shape = AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X16;
    if (dimM == 16 && (dimN == 16 || dimN == 32))
    {
        shape = dimN == 16 ? AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X16 : AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X32;
    }
    else if (dimN == 16 && (dimM == 32 || dimM == 64))
    {
        shape = dimM == 32 ? AmdExtD3DShaderIntrinsicsWaveMatrixShape_32X16 : AmdExtD3DShaderIntrinsicsWaveMatrixShape_64X16;
    }
    return shape;
}

uint AmdExtD3DMatrix_MatrixLayoutInMemory(bool bCol)
{
    return bCol ? AmdExtD3DShaderIntrinsicsWaveMatrixLayout_ColumnMajor :
                  AmdExtD3DShaderIntrinsicsWaveMatrixLayout_RowMajor;
}

/**
***********************************************************************************************************************
*   WaveMatrix Multiply
*
*   Result = A x B + Accumulator
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
*
***********************************************************************************************************************
*/
template <uint matrixDataFmtA, uint matrixDataFmtB, uint matrixDataFmtAccumulator, uint DIMM, uint DIMN, uint DIMK>
AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> AmdWaveMatrixMultiply(
    AmdWaveMatrixA<matrixDataFmtA, DIMM, DIMK> matA,
    AmdWaveMatrixB<matrixDataFmtB, DIMK, DIMN> matB,
    AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> matAccumulator)
{
    AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> matRes = matAccumulator;
    uint ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED;

    if (matA.GetDimm() == 16 && matA.GetDimn() == 16 && matB.GetDimm() == 16 && matB.GetDimn() == 16)
    {
        AmdExtD3DMatrix_MatrixMultiplyAccumulateOpcode_16x16x16(
            ops, matA.GetMatrixDataFmt(), matB.GetMatrixDataFmt(), matAccumulator.GetMatrixDataFmt());
    }
    else if (matA.GetDimm() == 16 && matA.GetDimn() == 32 && matB.GetDimm() == 32 && matB.GetDimn() == 16)
    {
        AmdExtD3DMatrix_MatrixMultiplyAccumulateOpcode_16x16x32(
            ops, matA.GetMatrixDataFmt(), matB.GetMatrixDataFmt(), matAccumulator.GetMatrixDataFmt());
    }

    if (ops != AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED)
    {
        matRes.container = AmdExtD3DShaderIntrinsics_WaveMatrixMulAcc(ops, matA.container, matB.container, matAccumulator.container);
    }

    return matRes;
}

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   Sparse WaveMatrix Multiply
*
*   Result = A x B + Accumulator
*
*   Sparse linear-algebraic matrix multiply of A and B with structural sparsity information taken from Index,
*   followed by component-wise addition of Acc
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmtA, uint matrixDataFmtB, uint matrixDataFmtAccumulator, uint DIMM, uint DIMN, uint DIMX, uint DIMY>
AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> AmdWaveMatrixMultiply(
    AmdWaveMatrixA<matrixDataFmtA, DIMM, DIMX> matA,
    AmdWaveMatrixB<matrixDataFmtB, DIMY, DIMN> matB,
    AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> matAccumulator,
    uint index_sel)
{
    AmdWaveMatrixAccumulator<matrixDataFmtAccumulator, DIMM, DIMN> matRes = matAccumulator;
    uint ops = AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED;

    if (matA.GetDimm() == 16 && matA.GetDimn() == 16 && matB.GetDimm() == 32 && matB.GetDimn() == 16)
    {
        AmdExtD3DMatrix_SparseMatrixMultiplyAccumulateOpcode_16x16x32(
            ops, matA.GetMatrixDataFmt(), matB.GetMatrixDataFmt(), matAccumulator.GetMatrixDataFmt());
    }
    else if (matA.GetDimm() == 16 && matA.GetDimn() == 32 && matB.GetDimm() == 64 && matB.GetDimn() == 16)
    {
        AmdExtD3DMatrix_SparseMatrixMultiplyAccumulateOpcode_16x16x64(
            ops, matA.GetMatrixDataFmt(), matB.GetMatrixDataFmt(), matAccumulator.GetMatrixDataFmt());
    }

    if (ops != AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_NOT_SUPPORTED)
    {
        matRes.container = AmdExtD3DShaderIntrinsics_WaveMatrixMulAcc(
            ops, matA.container, matB.container, matAccumulator.container, index_sel);
    }

    return matRes;
}
#endif

/**
***********************************************************************************************************************
*   Loading matrix object from srv, raw, global buffer or group share.
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Load(
    ByteAddressBuffer srv, uint startOffsetInBytes, uint strideInBytes, bool bColMajor)
{
    uint matrixLayout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    container = AmdExtD3DShaderIntrinsics_WaveMatrixLoad(
        srv, startOffsetInBytes, strideInBytes, matrixType, matrixDataFmt, matrixLayout, shape);
}

template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Load(
    RWByteAddressBuffer uav, uint startOffsetInBytes, uint strideInBytes, bool bColMajor)
{
    uint matrixLayout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    container = AmdExtD3DShaderIntrinsics_WaveMatrixLoad(
        uav, startOffsetInBytes, strideInBytes, matrixType, matrixDataFmt, matrixLayout, shape);
}

#if AMD_AGS_WMMA_INTERNAL
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Load(
    uint addr_lo, uint addr_hi, uint startOffsetInBytes, uint strideInBytes, bool bColMajor)
{
    uint matrixLayout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    container = AmdExtD3DShaderIntrinsics_WaveMatrixLoad(
        addr_lo, addr_hi, startOffsetInBytes, strideInBytes, matrixType, matrixDataFmt, matrixLayout, shape);
}
#endif

/**
***********************************************************************************************************************
*   Storing matrix object to raw buffer, global buffer or group share.
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Store(
    RWByteAddressBuffer uav, uint startOffsetInBytes, uint strideInBytes, bool bColMajor)
{
    uint matrixLayout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint shape = AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X16;
    AmdExtD3DShaderIntrinsics_WaveMatrixStore(
        uav, startOffsetInBytes, strideInBytes, container, matrixType, matrixDataFmt, matrixLayout, shape);
}

#if AMD_AGS_WMMA_INTERNAL
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Store(
    uint addr_lo, uint addr_hi, uint startOffsetInBytes, uint strideInBytes, bool bColMajor)
{
    uint matrixLayout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint shape = AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X16;
    AmdExtD3DShaderIntrinsics_WaveMatrixStore(
        addr_lo, addr_hi, startOffsetInBytes, strideInBytes, container, matrixType, matrixDataFmt, matrixLayout, shape);
}
#endif

#if AMD_AGS_WMMA_INTERNAL
/**
***********************************************************************************************************************
*   Load matrix sparsity index.
*
*   Available if matrix is A.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
uint AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::LoadSparsityIndex(
    ByteAddressBuffer srv, uint startOffsetInBytes)
{
    if (matrixType != AmdExtD3DShaderIntrinsicsWaveMatrixType_A)
    {
        return 0;
    }
    uint loadOffset = AmdExtD3DMatrix_MatrixSparsityIndexLoadPhase0(
        AmdExtD3DShaderIntrinsicsSparsityIndexMem_UavBuffer, 0, 0);
    uint loadVal = srv.Load(loadOffset);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    return AmdExtD3DMatrix_MatrixSparsityIndexLoad(
            matrixDataFmt, AmdExtD3DShaderIntrinsicsSparsityIndexMem_UavBuffer, shape, startOffsetInBytes, loadVal);
}

template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
uint AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::LoadSparsityIndex(
    RWByteAddressBuffer uav, uint startOffsetInBytes)
{
    if (matrixType != AmdExtD3DShaderIntrinsicsWaveMatrixType_A)
    {
        return 0;
    }
    uint loadOffset = AmdExtD3DMatrix_MatrixSparsityIndexLoadPhase0(
        AmdExtD3DShaderIntrinsicsSparsityIndexMem_UavBuffer, 0, 0);
    uint loadVal = uav.Load(loadOffset);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    return AmdExtD3DMatrix_MatrixSparsityIndexLoad(
            matrixDataFmt, AmdExtD3DShaderIntrinsicsSparsityIndexMem_UavBuffer, shape, startOffsetInBytes, loadVal);
}

template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
uint AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::LoadSparsityIndex(
    uint addr_lo, uint addr_hi, uint startOffsetInBytes)
{
    if (matrixType != AmdExtD3DShaderIntrinsicsWaveMatrixType_A)
    {
        return 0;
    }
    AmdExtD3DMatrix_MatrixSparsityIndexLoadPhase0(AmdExtD3DShaderIntrinsicsSparsityIndexMem_GlobalBuffer, addr_lo, addr_hi);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    return AmdExtD3DMatrix_MatrixSparsityIndexLoad(
            matrixDataFmt, AmdExtD3DShaderIntrinsicsSparsityIndexMem_GlobalBuffer, shape, startOffsetInBytes, 0);

}
#endif

/**
***********************************************************************************************************************
*   Read matrix data from group shared memory at the specified address.
*   Lookup LDS logical ID through the dummy refVal.
***********************************************************************************************************************
*/
AmdMatrixContainer AmdExtD3DShaderIntrinsics_WaveMatrixGroupsharedLoad(
    uint refVal, uint dimm, uint dimn, uint type, uint format, bool bColMajor)
{
    AmdMatrixContainer retVal;
    uint tmpVal;
    // phase 1: generate matrix_load amdil
    uint shape = AmdExtD3DMatrix_MatrixShape(dimm, dimn);
    uint layout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, layout, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsLoad, AmdExtD3DShaderIntrinsicsOpcodePhase_1, matProp);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, refVal, tmpVal, tmpVal);
    // phase 2: return destination matrix
    retVal = AmdExtD3DShaderIntrinsics_OutputMatrix(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsLoad, AmdExtD3DShaderIntrinsicsOpcodePhase_2);
    return retVal;
}

/**
***********************************************************************************************************************
*   Writes matrix data to the group shared memory at the specified address.
*   Lookup LDS logical ID through the dummy refVal.
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_WaveMatrixGroupsharedStore(
    uint refVal, uint dimm, uint dimn, AmdMatrixContainer AmdMat, uint type, uint format, bool bColMajor)
{
    uint retVal;
    // phase 1: input matrix container
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg,
                                          AmdMat);
    uint shape = AmdExtD3DMatrix_MatrixShape(dimm, dimn);
    uint layout = AmdExtD3DMatrix_MatrixLayoutInMemory(bColMajor);
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, layout, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore, AmdExtD3DShaderIntrinsicsOpcodePhase_2, matProp);
    // phase 2: generate matrix_store amdil
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, refVal, 0, retVal);
}

/**
***********************************************************************************************************************
*   Initialize the matrix with scalar value.
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Fill(elementType data)
{
    uint uVal = reinterpret_as_uint32(data);
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_MatrixFill(container, uVal, matrixType, matrixDataFmt, shape);
}

/**
***********************************************************************************************************************
*   Number of components of a matrix type accessible to the current thread
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
uint AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Length()
{
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    return AmdExtD3DShaderIntrinsics_WaveMatrixLength(matrixType, matrixDataFmt, shape);
}

/**
***********************************************************************************************************************
*   Return component i value.
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
typename AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::elementType
AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Element(uint i)
{
    elementType element_flag = 0;
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    return AmdExtD3DMatrix_ExtractComponent(container, i, matrixType, matrixDataFmt, element_flag, shape);
}

/**
***********************************************************************************************************************
*   Set data to component i.
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::SetElement(uint i, elementType data)
{
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_InsertComponent(container, i, data, matrixType, matrixDataFmt, shape);
}

/**
***********************************************************************************************************************
*   Copy data from input matrix, this might do type conversion or potential transpose.
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
template <uint srcMatrixDataFmt, uint srcMatrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::Copy(
    AmdWaveMatrix<srcMatrixDataFmt, DIMM, DIMN, srcMatrixType> input)
{
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_MatrixCopy(
        container, input.container, input.GetMatrixType(), input.GetMatrixDataFmt(),
        matrixType, matrixDataFmt, shape, AmdExtD3DShaderIntrinsicsWaveMatrixConversion_SatToFmaxNone);
}

/**
***********************************************************************************************************************
*   Copy data from input matrix, this might do type conversion or potential transpose.
*   Float formats from higher to lower precision will saturate to [MIN|MAX]_F*, otherwise the same as Copy().
*
*   Available in compute shader stage only.
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveMatrix) returned S_OK.
***********************************************************************************************************************
*/
uint AmdExtD3DMatrix_MatrixConversionSaturateToFmax(uint dstFmt, uint srcFmt)
{
    uint sat = AmdExtD3DShaderIntrinsicsWaveMatrixConversion_SatToFmaxNone;

    switch (srcFmt)
    {
        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32:
            {
                switch (dstFmt)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16:
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16:
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                        sat = AmdExtD3DShaderIntrinsicsWaveMatrixConversion_SatToFmax;
                        break;
                    default:
                        break;
                }
            }
            break;
        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16:
        case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16:
            {
                switch (dstFmt)
                {
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8:
                    case AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8:
                        sat = AmdExtD3DShaderIntrinsicsWaveMatrixConversion_SatToFmax;
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
    return sat;
}

template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
template <uint srcMatrixDataFmt, uint srcMatrixType>
void AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::CopySat(
    AmdWaveMatrix<srcMatrixDataFmt, DIMM, DIMN, srcMatrixType> input)
{
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    uint sat = AmdExtD3DMatrix_MatrixConversionSaturateToFmax(matrixDataFmt, input.GetMatrixDataFmt());
    AmdExtD3DMatrix_MatrixCopy(
        container, input.container, input.GetMatrixType(), input.GetMatrixDataFmt(), matrixType, matrixDataFmt, shape, sat);
}

/**
***********************************************************************************************************************
*   AmdExtD3DMatrix_ElementWiseArithmetic
***********************************************************************************************************************
*/
void AmdExtD3DMatrix_ElementWiseArithmetic(
    uint aluOp, inout AmdMatrixContainer dstAmdMat, in AmdMatrixContainer src0AmdMat, in AmdMatrixContainer src1AmdMat, uint type, uint format, uint shape)
{
    // phase 0: store src0
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_A_TempReg,
                                          src0AmdMat);
    // phase 0: store src1
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_B_TempReg,
                                          src1AmdMat);
    // phase 1: generate amdil
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, 0, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic, AmdExtD3DShaderIntrinsicsOpcodePhase_1, matProp);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, aluOp, 0, dstAmdMat.r[0]);
    // phase 2: return result matrix
    dstAmdMat = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic,
                                                       AmdExtD3DShaderIntrinsicsOpcodePhase_2);
}
void AmdExtD3DMatrix_ElementWiseArithmetic(
    uint aluOp, inout AmdMatrixContainer dstAmdMat, in AmdMatrixContainer srcAmdMat, in uint scalar, uint type, uint format, uint shape)
{
    // phase 0: store src0
    AmdExtD3DShaderIntrinsics_InputMatrix(AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic,
                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          AmdExtD3DShaderIntrinsicsWaveMatrixRegType_A_TempReg,
                                          srcAmdMat);
    // phase 1: generate amdil
    uint matProp = AmdExtD3DShaderIntrinsics_MatrixPropertyEncoding(type, format, 0, shape);
    uint inst = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic, AmdExtD3DShaderIntrinsicsOpcodePhase_1, matProp);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(inst, aluOp, scalar, dstAmdMat.r[0]);
    // phase 2: return result matrix
    dstAmdMat = AmdExtD3DShaderIntrinsics_OutputMatrix(AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic,
                                                       AmdExtD3DShaderIntrinsicsOpcodePhase_2);
}

// Operator + overloading
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::operator+(AmdWaveMatrix mat) {
    AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> dst;
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_ElementWiseArithmetic(
        AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Add, dst.container, container, mat.container, matrixType, matrixDataFmt, shape);
    return dst;
}

// Operator - overloading
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::operator-(AmdWaveMatrix mat) {
    AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> dst;
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_ElementWiseArithmetic(
        AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Sub, dst.container, container, mat.container, matrixType, matrixDataFmt, shape);
    return dst;
}

// Operator * overloading
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::operator*(AmdWaveMatrix mat) {
    AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> dst;
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_ElementWiseArithmetic(
        AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Mul, dst.container, container, mat.container, matrixType, matrixDataFmt, shape);
    return dst;
}

// Operator / overloading
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::operator/(AmdWaveMatrix mat) {
    AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> dst;
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_ElementWiseArithmetic(
        AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Div, dst.container, container, mat.container, matrixType, matrixDataFmt, shape);
    return dst;
}

// Operator * overloading
// Matrix times a scalar value
template <uint matrixDataFmt, uint DIMM, uint DIMN, uint matrixType>
AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType>::operator*(elementType scalar) {
    AmdWaveMatrix<matrixDataFmt, DIMM, DIMN, matrixType> dst;
    uint shape = AmdExtD3DMatrix_MatrixShape(DIMM, DIMN);
    AmdExtD3DMatrix_ElementWiseArithmetic(
        AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Times, dst.container, container, reinterpret_as_uint32(scalar), matrixType, matrixDataFmt, shape);
    return dst;
}


#endif // _AMDEXTD3DSHADERINTRINICSMATRIXOPSHELPER_HLSL
