/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsInfo structure that defines various support levels by the ASIC
*    for shader instrinsics extension.
***********************************************************************************************************************
*/
struct AmdExtD3DShaderIntrinsicsInfo
{
    unsigned int waveSize;
    unsigned int minWaveSize;
    unsigned int maxWaveSize;
    unsigned int reserved[5];
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsics instruction encoding
*    [07:00] Instruction opcode               (AmdExtD3DShaderIntrinsicsOpcode)
*    [23:08] Opcode-specific data
*    [25:24] Opcode phase
*    [27:26] Reserved
*    [31:28] Shader intrinsics magic code     (AmdExtD3DShaderIntrinsics_MagicCode)
*
***********************************************************************************************************************
*/
const unsigned int AmdExtD3DShaderIntrinsics_MagicCodeShift   = 28;
const unsigned int AmdExtD3DShaderIntrinsics_MagicCodeMask    = 0xf;
const unsigned int AmdExtD3DShaderIntrinsics_OpcodePhaseShift = 24;
const unsigned int AmdExtD3DShaderIntrinsics_OpcodePhaseMask  = 0x3;
const unsigned int AmdExtD3DShaderIntrinsics_DataShift        = 8;
const unsigned int AmdExtD3DShaderIntrinsics_DataMask         = 0xffff;
const unsigned int AmdExtD3DShaderIntrinsics_OpcodeShift      = 0;
const unsigned int AmdExtD3DShaderIntrinsics_OpcodeMask       = 0xff;


/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsics_MagicCode
*    Used to distinguish 32 bit imm dst in a imm_atomic_cmp_exch as an intrinsic instruction
*
***********************************************************************************************************************
*/
const unsigned int AmdExtD3DShaderIntrinsics_MagicCode = 0x5;


/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsOpcode enumeration
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsOpcode
{
    AmdExtD3DShaderIntrinsicsOpcode_Readfirstlane        = 0x1,
    AmdExtD3DShaderIntrinsicsOpcode_Readlane             = 0x2,
    AmdExtD3DShaderIntrinsicsOpcode_LaneId               = 0x3,
    AmdExtD3DShaderIntrinsicsOpcode_Swizzle              = 0x4,
    AmdExtD3DShaderIntrinsicsOpcode_Ballot               = 0x5,
    AmdExtD3DShaderIntrinsicsOpcode_MBCnt                = 0x6,
    AmdExtD3DShaderIntrinsicsOpcode_Min3U                = 0x7,
    AmdExtD3DShaderIntrinsicsOpcode_Min3F                = 0x8,
    AmdExtD3DShaderIntrinsicsOpcode_Med3U                = 0x9,
    AmdExtD3DShaderIntrinsicsOpcode_Med3F                = 0xa,
    AmdExtD3DShaderIntrinsicsOpcode_Max3U                = 0xb,
    AmdExtD3DShaderIntrinsicsOpcode_Max3F                = 0xc,
    AmdExtD3DShaderIntrinsicsOpcode_BaryCoord            = 0xd,
    AmdExtD3DShaderIntrinsicsOpcode_VtxParam             = 0xe,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved1            = 0xf,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved2            = 0x10,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved3            = 0x11,
    AmdExtD3DShaderIntrinsicsOpcode_WaveReduce           = 0x12,
    AmdExtD3DShaderIntrinsicsOpcode_WaveScan             = 0x13,
    AmdExtD3DShaderIntrinsicsOpcode_LoadDwordAtAddr      = 0x14,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved4            = 0x15,
    AmdExtD3DShaderIntrinsicsOpcode_IntersectInternal    = 0x16,
    AmdExtD3DShaderIntrinsicsOpcode_DrawIndex            = 0x17,
    AmdExtD3DShaderIntrinsicsOpcode_AtomicU64            = 0x18,
    AmdExtD3DShaderIntrinsicsOpcode_GetWaveSize          = 0x19,
    AmdExtD3DShaderIntrinsicsOpcode_BaseInstance         = 0x1a,
    AmdExtD3DShaderIntrinsicsOpcode_BaseVertex           = 0x1b,
    AmdExtD3DShaderIntrinsicsOpcode_FloatConversion      = 0x1c,
    AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt           = 0x1d,
    AmdExtD3DShaderIntrinsicsOpcode_RayTraceHitToken     = 0x1e,
    AmdExtD3DShaderIntrinsicsOpcode_ShaderClock          = 0x1f,
    AmdExtD3DShaderIntrinsicsOpcode_ShaderRealtimeClock  = 0x20,
    AmdExtD3DShaderIntrinsicsOpcode_Halt                 = 0x21,
    AmdExtD3DShaderIntrinsicsOpcode_IntersectBvhNode     = 0x22,
    AmdExtD3DShaderIntrinsicsOpcode_BufferStoreByte      = 0x23,
    AmdExtD3DShaderIntrinsicsOpcode_BufferStoreShort     = 0x24,
    AmdExtD3DShaderIntrinsicsOpcode_ShaderMarker         = 0x25,
    AmdExtD3DShaderIntrinsicsOpcode_FloatOpWithRoundMode = 0x26,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved5            = 0x27,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc            = 0x28,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavLoad           = 0x29,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavStore          = 0x2a,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalLoad        = 0x2b,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalStore       = 0x2c,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsLoad           = 0x2d,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore          = 0x2e,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementFill       = 0x2f,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementExtract    = 0x30,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLength            = 0x31,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixCopy              = 0x32,
    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixFill              = 0x33,
    AmdExtD3DShaderIntrinsicsOpcode_MatrixSparsityIndexLoad     = 0x34,
    AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic = 0x35,
    AmdExtD3DShaderIntrinsicsOpcode_Float8Conversion            = 0x36,
    AmdExtD3DShaderIntrinsicsOpcode_BuiltIn1                    = 0x37,
    AmdExtD3DShaderIntrinsicsOpcode_BuiltInArg                  = 0x38,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved6                   = 0x39,
    AmdExtD3DShaderIntrinsicsOpcode_Reserved7                   = 0x3a,    
    AmdExtD3DShaderIntrinsicsOpcode_LoadByteAtAddr              = 0x3b,
    AmdExtD3DShaderIntrinsicsOpcode_LastValidOpcode             = 0x3b,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsOpcodePhase enumeration
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsOpcodePhase
{
    AmdExtD3DShaderIntrinsicsOpcodePhase_0   = 0x0,
    AmdExtD3DShaderIntrinsicsOpcodePhase_1   = 0x1,
    AmdExtD3DShaderIntrinsicsOpcodePhase_2   = 0x2,
    AmdExtD3DShaderIntrinsicsOpcodePhase_3   = 0x3,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsBarycentric enumeration to specify the interplation mode
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsBarycentric
{
    AmdExtD3DShaderIntrinsicsBarycentric_LinearCenter   = 0x1,
    AmdExtD3DShaderIntrinsicsBarycentric_LinearCentroid = 0x2,
    AmdExtD3DShaderIntrinsicsBarycentric_LinearSample   = 0x3,
    AmdExtD3DShaderIntrinsicsBarycentric_PerspCenter    = 0x4,
    AmdExtD3DShaderIntrinsicsBarycentric_PerspCentroid  = 0x5,
    AmdExtD3DShaderIntrinsicsBarycentric_PerspSample    = 0x6,
    AmdExtD3DShaderIntrinsicsBarycentric_PerspPullModel = 0x7,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsWaveOp enumeration to specify the wave operation
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsWaveOp
{
    AmdExtD3DShaderIntrinsicsWaveOp_AddF = 0x01,
    AmdExtD3DShaderIntrinsicsWaveOp_AddI = 0x02,
    AmdExtD3DShaderIntrinsicsWaveOp_AddU = 0x03,
    AmdExtD3DShaderIntrinsicsWaveOp_MulF = 0x04,
    AmdExtD3DShaderIntrinsicsWaveOp_MulI = 0x05,
    AmdExtD3DShaderIntrinsicsWaveOp_MulU = 0x06,
    AmdExtD3DShaderIntrinsicsWaveOp_MinF = 0x07,
    AmdExtD3DShaderIntrinsicsWaveOp_MinI = 0x08,
    AmdExtD3DShaderIntrinsicsWaveOp_MinU = 0x09,
    AmdExtD3DShaderIntrinsicsWaveOp_MaxF = 0x0a,
    AmdExtD3DShaderIntrinsicsWaveOp_MaxI = 0x0b,
    AmdExtD3DShaderIntrinsicsWaveOp_MaxU = 0x0c,
    AmdExtD3DShaderIntrinsicsWaveOp_And  = 0x0d,
    AmdExtD3DShaderIntrinsicsWaveOp_Or   = 0x0e,
    AmdExtD3DShaderIntrinsicsWaveOp_Xor  = 0x0f,
};

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required for WaveOps
***********************************************************************************************************************
*/
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift      = 0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_OpcodeMask       = 0xff;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_FlagShift        = 8;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_FlagMask         = 0xff;

// Following flags only apply to AmdExtD3DShaderIntrinsicsSupport_WaveScan
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_Inclusive        = 0x1;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_Exclusive        = 0x2;

// Following flags only apply to AmdExtD3DShaderIntrinsicsSupport_WaveReduce
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSizeNone  = 0x0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize1     = 0x1;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize2     = 0x2;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize4     = 0x3;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize8     = 0x4;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize16    = 0x5;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize32    = 0x6;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveOp_ClusterSize64    = 0x7;

/**
***********************************************************************************************************************
* @brief
*    AmdExtShaderIntrinsicsSupport enumeration used to check the opcode type supported by extension.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsSupport
{
    AmdExtD3DShaderIntrinsicsSupport_Readfirstlane        = 0x1,
    AmdExtD3DShaderIntrinsicsSupport_Readlane             = 0x2,
    AmdExtD3DShaderIntrinsicsSupport_LaneId               = 0x3,
    AmdExtD3DShaderIntrinsicsSupport_Swizzle              = 0x4,
    AmdExtD3DShaderIntrinsicsSupport_Ballot               = 0x5,
    AmdExtD3DShaderIntrinsicsSupport_MBCnt                = 0x6,
    AmdExtD3DShaderIntrinsicsSupport_Compare3             = 0x7,
    AmdExtD3DShaderIntrinsicsSupport_Barycentrics         = 0x8,
    AmdExtD3DShaderIntrinsicsSupport_WaveReduce           = 0x9,
    AmdExtD3DShaderIntrinsicsSupport_WaveScan             = 0xA,
    AmdExtD3DShaderIntrinsicsSupport_LoadDwordAtAddr      = 0xB,
    AmdExtD3DShaderIntrinsicsSupport_Reserved1            = 0xC,
    AmdExtD3DShaderIntrinsicsSupport_IntersectInternal    = 0xD,
    AmdExtD3DShaderIntrinsicsSupport_DrawIndex            = 0xE,
    AmdExtD3DShaderIntrinsicsSupport_AtomicU64            = 0xF,
    AmdExtD3DShaderIntrinsicsSupport_BaseInstance         = 0x10,
    AmdExtD3DShaderIntrinsicsSupport_BaseVertex           = 0x11,
    AmdExtD3DShaderIntrinsicsSupport_FloatConversion      = 0x12,
    AmdExtD3DShaderIntrinsicsSupport_GetWaveSize          = 0x13,
    AmdExtD3DShaderIntrinsicsSupport_ReadlaneAt           = 0x14,
    AmdExtD3DShaderIntrinsicsSupport_RayTraceHitToken     = 0x15,
    AmdExtD3DShaderIntrinsicsSupport_ShaderClock          = 0x16,
    AmdExtD3DShaderIntrinsicsSupport_ShaderRealtimeClock  = 0x17,
    AmdExtD3DShaderIntrinsicsSupport_Halt                 = 0x18,
    AmdExtD3DShaderIntrinsicsSupport_IntersectBvhNode     = 0x19,
    AmdExtD3DShaderIntrinsicsSupport_BufferStoreByte      = 0x1A,
    AmdExtD3DShaderIntrinsicsSupport_BufferStoreShort     = 0x1B,
    AmdExtD3DShaderIntrinsicsSupport_ShaderMarker         = 0x1C,
    AmdExtD3DShaderIntrinsicsSupport_FloatOpWithRoundMode = 0x1D,
    AmdExtD3DShaderIntrinsicsSupport_Reserved2            = 0x1E,
    AmdExtD3DShaderIntrinsicsSupport_WaveMatrix           = 0x1F,
    AmdExtD3DShaderIntrinsicsSupport_Float8Conversion     = 0x20,
    AmdExtD3DShaderIntrinsicsSupport_Builtins             = 0x21,
    AmdExtD3DShaderIntrinsicsSupport_LoadByteAtAddr       = 0x22,
};

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required to reference a vertex parameter
***********************************************************************************************************************
*/
const unsigned int AmdExtD3DShaderIntrinsicsBarycentric_ParamShift     = 0;
const unsigned int AmdExtD3DShaderIntrinsicsBarycentric_ParamMask      = 0x1f;
const unsigned int AmdExtD3DShaderIntrinsicsBarycentric_VtxShift       = 0x5;
const unsigned int AmdExtD3DShaderIntrinsicsBarycentric_VtxMask        = 0x3;
const unsigned int AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift = 0x7;
const unsigned int AmdExtD3DShaderIntrinsicsBarycentric_ComponentMask  = 0x3;

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsAtomicOp enumeration to specify the atomic operation
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsAtomicOp
{
    AmdExtD3DShaderIntrinsicsAtomicOp_MinU64     = 0x01,
    AmdExtD3DShaderIntrinsicsAtomicOp_MaxU64     = 0x02,
    AmdExtD3DShaderIntrinsicsAtomicOp_AndU64     = 0x03,
    AmdExtD3DShaderIntrinsicsAtomicOp_OrU64      = 0x04,
    AmdExtD3DShaderIntrinsicsAtomicOp_XorU64     = 0x05,
    AmdExtD3DShaderIntrinsicsAtomicOp_AddU64     = 0x06,
    AmdExtD3DShaderIntrinsicsAtomicOp_XchgU64    = 0x07,
    AmdExtD3DShaderIntrinsicsAtomicOp_CmpXchgU64 = 0x08,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsFloatConversionOp enumeration to specify the type and rounding mode of
*    float to float16 conversion
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsFloatConversionOp
{
    AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16Near    = 0x01,
    AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16NegInf  = 0x02,
    AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16PlusInf = 0x03,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode enumeration for supported round modes.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsFloatOpWithRoundModeMode
{
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TiesToEven     = 0x0,
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardPositive = 0x1,
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardNegative = 0x2,
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardZero     = 0x3,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode enumeration for supported float operations.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsFloatOpWithRoundModeOp
{
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Add      = 0x0,
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract = 0x1,
    AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Multiply = 0x2,
};

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required for AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode
***********************************************************************************************************************
*/
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_ModeShift = 0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_ModeMask  = 0xf;
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_OpShift   = 4;
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_OpMask    = 0xf;

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsFloat8CvtOp enumeration to specify the conversion operation
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsFloat8CvtOp
{
    AmdExtD3DShaderIntrinsicsFloat8CvtOp_FP8_2_F32 = 0x0,
    AmdExtD3DShaderIntrinsicsFloat8CvtOp_BF8_2_F32 = 0x1,
    AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_FP8 = 0x2,
    AmdExtD3DShaderIntrinsicsFloat8CvtOp_F32_2_BF8 = 0x3,
};

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required for AmdExtD3DShaderIntrinsicsFloat8Conversion
***********************************************************************************************************************
*/
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloat8Conversion_CvtOpShift = 0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloat8Conversion_CvtOpMask  = 0xff;
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloat8Conversion_SatShift   = 8;
constexpr unsigned int AmdExtD3DShaderIntrinsicsFloat8Conversion_SatMask    = 0x1;

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat enumeration for supported matrix element data format.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsWaveMatrixOpDataFormat
{
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I4   = 0x0,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U4   = 0x1,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I8   = 0x2,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U8   = 0x3,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16  = 0x4,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF16 = 0x5,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32  = 0x6,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_I32  = 0x7,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_U32  = 0x8,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_BF8  = 0x9,
    AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8  = 0xa,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsWaveMatrixType enumeration for supported wave matrix type.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsWaveMatrixOpMatrixType
{
    AmdExtD3DShaderIntrinsicsWaveMatrixType_A            = 0x0,
    AmdExtD3DShaderIntrinsicsWaveMatrixType_B            = 0x1,
    AmdExtD3DShaderIntrinsicsWaveMatrixType_Accumulator  = 0x2,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsWaveMatrixMatrixShape enumeration for supported wave matrix shape.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsWaveMatrixOpMatrixShape
{
    AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X16 = 0x0,
    AmdExtD3DShaderIntrinsicsWaveMatrixShape_32X16 = 0x1,
    AmdExtD3DShaderIntrinsicsWaveMatrixShape_16X32 = 0x2,
    AmdExtD3DShaderIntrinsicsWaveMatrixShape_64X16 = 0x3,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode enumeration to specify the wmma inst opcode.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsWaveMatrixOpcode
{
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_BF16_16X16X16_BF16     = 0x0,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F16_16X16X16_F16       = 0x1,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF16      = 0x2,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF8_BF8   = 0x3,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_BF8_FP8   = 0x4,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_F16       = 0x5,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_FP8_BF8   = 0x6,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_F32_16X16X16_FP8_FP8   = 0x7,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_I4        = 0x8,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_U4        = 0x9,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_IU4       = 0xa,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_UI4       = 0xb,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_I8        = 0xc,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_U8        = 0xd,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_IU8       = 0xe,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X16_UI8       = 0xf,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_I4        = 0x10,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_U4        = 0x11,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_IU4       = 0x12,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_WMMA_I32_16X16X32_UI4       = 0x13,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_BF16_16X16X32_BF16    = 0x14,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F16_16X16X32_F16      = 0x15,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF16     = 0x16,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF8_BF8  = 0x17,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_BF8_FP8  = 0x18,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_F16      = 0x19,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_FP8_BF8  = 0x1a,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_F32_16X16X32_FP8_FP8  = 0x1b,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_I4       = 0x1c,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_U4       = 0x1d,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_IU4      = 0x1e,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_UI4      = 0x1f,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_I8       = 0x20,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_U8       = 0x21,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_IU8      = 0x22,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X32_UI8      = 0x23,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_I4       = 0x24,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_U4       = 0x25,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_IU4      = 0x26,
    AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_SWMMA_I32_16X16X64_UI4      = 0x27,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsWaveMatrixRegType enumeration to specify the temp register.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsWaveMatrixRegType
{
    AmdExtD3DShaderIntrinsicsWaveMatrixRegType_RetVal_Reg          = 0x0,
    AmdExtD3DShaderIntrinsicsWaveMatrixRegType_A_TempReg           = 0x1,
    AmdExtD3DShaderIntrinsicsWaveMatrixRegType_B_TempReg           = 0x2,
    AmdExtD3DShaderIntrinsicsWaveMatrixRegType_Accumulator_TempReg = 0x3,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsMatrixElementWiseOp enumeration to specify the element-wise operation
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsMatrixElementWiseOp
{
    AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Add   = 0x1,
    AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Sub   = 0x2,
    AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Mul   = 0x3,
    AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Div   = 0x4,
    AmdExtD3DShaderIntrinsicsMatrixElementWiseOp_Times = 0x5,
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsSparsityIndexMem enumeration is used to specify where to read sparsity indexes.
***********************************************************************************************************************
*/
enum AmdExtD3DShaderIntrinsicsSparsityIndexMem
{
    AmdExtD3DShaderIntrinsicsSparsityIndexMem_UavBuffer    = 0x0,
    AmdExtD3DShaderIntrinsicsSparsityIndexMem_GroupShared  = 0x1,
    AmdExtD3DShaderIntrinsicsSparsityIndexMem_GlobalBuffer = 0x2,
};

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required for AmdExtD3DShaderIntrinsicsWaveMatrixOpcode
***********************************************************************************************************************
*/
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_OpsShift  = 0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_OpsMask   = 0x7f;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_FlagShift = 15;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixOpcode_FlagMask  = 0x1;

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required for AmdExtD3DShaderIntrinsicsWaveMatrixOpInOut
***********************************************************************************************************************
*/
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelShift        = 0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixInOut_ChannelMask         = 0xf;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagShift  = 4;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixInOut_SecondRegFlagMask   = 0xf;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagShift = 8;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixInOut_MatRegTypeFlagMask  = 0xff;

/**
***********************************************************************************************************************
* @brief
*    Shifts and masks for the arguments required for AmdExtD3DShaderIntrinsicsWaveMatrixModifier
***********************************************************************************************************************
*/
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_DataFormatFlagShift = 0;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_DataFormatFlagMask  = 0xf;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTypeFlagShift = 4;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTypeFlagMask  = 0x7;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_LayoutFlagShift     = 7;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_LayoutFlagMask      = 0x1;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_ShapeShift          = 8;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_ShapeMask           = 0x7;
// Following flags only apply to AmdExtD3DShaderIntrinsicsOpcode_WaveMatrix*Load
// and AmdExtD3DShaderIntrinsicsOpcode_WaveMatrix*Store
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTileShift     = 11;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_MatrixTileMask      = 0x1;
// Following flags only apply to AmdExtD3DShaderIntrinsicsOpcode_MatrixSparsityIndexLoad
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_IndexMemTypeShift   = 14;
constexpr unsigned int AmdExtD3DShaderIntrinsicsWaveMatrixModifier_IndexMemTypeMask    = 0x3;

// AMD shader intrinsics designated SpaceId.  Denotes Texture3D resource and static sampler used in conjuction with
// instrinsic instructions.
// Applications should use this value for D3D12_ROOT_DESCRIPTOR::RegisterSpace and
// D3D12_STATIC_SAMPLER_DESC::RegisterSpace when creating root descriptor entries for shader instrinsic Texture3D and
// static sampler.
// NOTE: D3D reserves SpaceIds in range 0xFFFFFFF0 - 0xFFFFFFFF
const unsigned int AmdExtD3DShaderIntrinsicsSpaceId = 0x7FFF0ADE; // 2147420894
