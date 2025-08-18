/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef _AMDEXTD3DSHADERINTRINICS_HLSL
#define _AMDEXTD3DSHADERINTRINICS_HLSL

// Default AMD shader intrinsics designated SpaceId.
#define AmdExtD3DShaderIntrinsicsSpaceId space2147420894

// Dummy UAV used to access shader intrinsics. Applications need to add a root signature entry for this resource in
// order to use shader extensions. Applications may specify an alternate UAV binding by defining
// AMD_EXT_SHADER_INTRINSIC_UAV_OVERRIDE. The application must also call IAmdExtD3DShaderIntrinsics1::SetExtensionUavBinding()
// in order to use an alternate binding. This must be done before creating the root signature and pipeline.
#ifndef AMD_EXT_SHADER_INTRINSIC_UAV_OVERRIDE
RWByteAddressBuffer AmdExtD3DShaderIntrinsicsUAV : register(u0, AmdExtD3DShaderIntrinsicsSpaceId);
#else
#define AmdExtD3DShaderIntrinsicsUAV AMD_EXT_SHADER_INTRINSIC_UAV_OVERRIDE
#endif

/**
***********************************************************************************************************************
*   Definitions to construct the intrinsic instruction composed of an opcode and optional immediate data.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsics_MagicCodeShift   28
#define AmdExtD3DShaderIntrinsics_MagicCodeMask    0xf
#define AmdExtD3DShaderIntrinsics_OpcodePhaseShift 24
#define AmdExtD3DShaderIntrinsics_OpcodePhaseMask  0x3
#define AmdExtD3DShaderIntrinsics_DataShift        8
#define AmdExtD3DShaderIntrinsics_DataMask         0xffff
#define AmdExtD3DShaderIntrinsics_OpcodeShift      0
#define AmdExtD3DShaderIntrinsics_OpcodeMask       0xff

#define AmdExtD3DShaderIntrinsics_MagicCode        0x5


/**
***********************************************************************************************************************
*   Intrinsic opcodes.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsOpcode_Readfirstlane        0x01
#define AmdExtD3DShaderIntrinsicsOpcode_Readlane             0x02
#define AmdExtD3DShaderIntrinsicsOpcode_LaneId               0x03
#define AmdExtD3DShaderIntrinsicsOpcode_Swizzle              0x04
#define AmdExtD3DShaderIntrinsicsOpcode_Ballot               0x05
#define AmdExtD3DShaderIntrinsicsOpcode_MBCnt                0x06
#define AmdExtD3DShaderIntrinsicsOpcode_Min3U                0x07
#define AmdExtD3DShaderIntrinsicsOpcode_Min3F                0x08
#define AmdExtD3DShaderIntrinsicsOpcode_Med3U                0x09
#define AmdExtD3DShaderIntrinsicsOpcode_Med3F                0x0a
#define AmdExtD3DShaderIntrinsicsOpcode_Max3U                0x0b
#define AmdExtD3DShaderIntrinsicsOpcode_Max3F                0x0c
#define AmdExtD3DShaderIntrinsicsOpcode_BaryCoord            0x0d
#define AmdExtD3DShaderIntrinsicsOpcode_VtxParam             0x0e
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved1            0x0f
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved2            0x10
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved3            0x11
#define AmdExtD3DShaderIntrinsicsOpcode_WaveReduce           0x12
#define AmdExtD3DShaderIntrinsicsOpcode_WaveScan             0x13
#define AmdExtD3DShaderIntrinsicsOpcode_LoadDwAtAddr         0x14
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved4            0x15
#define AmdExtD3DShaderIntrinsicsOpcode_IntersectInternal    0x16
#define AmdExtD3DShaderIntrinsicsOpcode_DrawIndex            0x17
#define AmdExtD3DShaderIntrinsicsOpcode_AtomicU64            0x18
#define AmdExtD3DShaderIntrinsicsOpcode_GetWaveSize          0x19
#define AmdExtD3DShaderIntrinsicsOpcode_BaseInstance         0x1a
#define AmdExtD3DShaderIntrinsicsOpcode_BaseVertex           0x1b
#define AmdExtD3DShaderIntrinsicsOpcode_FloatConversion      0x1c
#define AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt           0x1d
#define AmdExtD3DShaderIntrinsicsOpcode_RayTraceHitToken     0x1e
#define AmdExtD3DShaderIntrinsicsOpcode_ShaderClock          0x1f
#define AmdExtD3DShaderIntrinsicsOpcode_ShaderRealtimeClock  0x20
#define AmdExtD3DShaderIntrinsicsOpcode_Halt                 0x21
#define AmdExtD3DShaderIntrinsicsOpcode_IntersectBvhNode     0x22
#define AmdExtD3DShaderIntrinsicsOpcode_BufferStoreByte      0x23
#define AmdExtD3DShaderIntrinsicsOpcode_BufferStoreShort     0x24
#define AmdExtD3DShaderIntrinsicsOpcode_ShaderMarker         0x25
#define AmdExtD3DShaderIntrinsicsOpcode_FloatOpWithRoundMode 0x26
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved5                     0x27
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc              0x28
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavLoad             0x29
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavStore            0x2a
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalLoad          0x2b
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixGlobalStore         0x2c
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsLoad             0x2d
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore            0x2e
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementFill         0x2f
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementExtract      0x30
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLength              0x31
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixCopy                0x32
#define AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixFill                0x33
#define AmdExtD3DShaderIntrinsicsOpcode_MatrixSparsityIndexLoad       0x34
#define AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic   0x35
#define AmdExtD3DShaderIntrinsicsOpcode_Float8Conversion              0x36

/**
***********************************************************************************************************************
*   Intrinsic opcode phases.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsOpcodePhase_0    0x0
#define AmdExtD3DShaderIntrinsicsOpcodePhase_1    0x1
#define AmdExtD3DShaderIntrinsicsOpcodePhase_2    0x2
#define AmdExtD3DShaderIntrinsicsOpcodePhase_3    0x3

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveOp defines for supported operations. Can be used as the parameter for the
*   AmdExtD3DShaderIntrinsicsOpcode_WaveOp intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveOp_AddF 0x01
#define AmdExtD3DShaderIntrinsicsWaveOp_AddI 0x02
#define AmdExtD3DShaderIntrinsicsWaveOp_AddU 0x03
#define AmdExtD3DShaderIntrinsicsWaveOp_MulF 0x04
#define AmdExtD3DShaderIntrinsicsWaveOp_MulI 0x05
#define AmdExtD3DShaderIntrinsicsWaveOp_MulU 0x06
#define AmdExtD3DShaderIntrinsicsWaveOp_MinF 0x07
#define AmdExtD3DShaderIntrinsicsWaveOp_MinI 0x08
#define AmdExtD3DShaderIntrinsicsWaveOp_MinU 0x09
#define AmdExtD3DShaderIntrinsicsWaveOp_MaxF 0x0a
#define AmdExtD3DShaderIntrinsicsWaveOp_MaxI 0x0b
#define AmdExtD3DShaderIntrinsicsWaveOp_MaxU 0x0c
#define AmdExtD3DShaderIntrinsicsWaveOp_And  0x0d    // Reduction only
#define AmdExtD3DShaderIntrinsicsWaveOp_Or   0x0e    // Reduction only
#define AmdExtD3DShaderIntrinsicsWaveOp_Xor  0x0f    // Reduction only

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveOp masks and shifts for opcode and flags
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift 0
#define AmdExtD3DShaderIntrinsicsWaveOp_OpcodeMask  0xff
#define AmdExtD3DShaderIntrinsicsWaveOp_FlagShift   8
#define AmdExtD3DShaderIntrinsicsWaveOp_FlagMask    0xff

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsWaveOp flags for use with AmdExtD3DShaderIntrinsicsOpcode_WaveScan.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsWaveOp_Inclusive   0x01
#define AmdExtD3DShaderIntrinsicsWaveOp_Exclusive   0x02

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsSwizzle defines for common swizzles.  Can be used as the operation parameter for the
*   AmdExtD3DShaderIntrinsics_Swizzle intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsSwizzle_SwapX1      0x041f
#define AmdExtD3DShaderIntrinsicsSwizzle_SwapX2      0x081f
#define AmdExtD3DShaderIntrinsicsSwizzle_SwapX4      0x101f
#define AmdExtD3DShaderIntrinsicsSwizzle_SwapX8      0x201f
#define AmdExtD3DShaderIntrinsicsSwizzle_SwapX16     0x401f
#define AmdExtD3DShaderIntrinsicsSwizzle_ReverseX2   0x041f
#define AmdExtD3DShaderIntrinsicsSwizzle_ReverseX4   0x0c1f
#define AmdExtD3DShaderIntrinsicsSwizzle_ReverseX8   0x1c1f
#define AmdExtD3DShaderIntrinsicsSwizzle_ReverseX16  0x3c1f
#define AmdExtD3DShaderIntrinsicsSwizzle_ReverseX32  0x7c1f
#define AmdExtD3DShaderIntrinsicsSwizzle_BCastX2     0x003e
#define AmdExtD3DShaderIntrinsicsSwizzle_BCastX4     0x003c
#define AmdExtD3DShaderIntrinsicsSwizzle_BCastX8     0x0038
#define AmdExtD3DShaderIntrinsicsSwizzle_BCastX16    0x0030
#define AmdExtD3DShaderIntrinsicsSwizzle_BCastX32    0x0020


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsBarycentric defines for barycentric interpolation mode.  To be used with
*   AmdExtD3DShaderIntrinsicsOpcode_IjBarycentricCoords to specify the interpolation mode.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsBarycentric_LinearCenter    0x1
#define AmdExtD3DShaderIntrinsicsBarycentric_LinearCentroid  0x2
#define AmdExtD3DShaderIntrinsicsBarycentric_LinearSample    0x3
#define AmdExtD3DShaderIntrinsicsBarycentric_PerspCenter     0x4
#define AmdExtD3DShaderIntrinsicsBarycentric_PerspCentroid   0x5
#define AmdExtD3DShaderIntrinsicsBarycentric_PerspSample     0x6
#define AmdExtD3DShaderIntrinsicsBarycentric_PerspPullModel  0x7

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsBarycentric defines for specifying vertex and parameter indices.  To be used as inputs to
*   the AmdExtD3DShaderIntrinsicsOpcode_VertexParameter function
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsBarycentric_Vertex0        0x0
#define AmdExtD3DShaderIntrinsicsBarycentric_Vertex1        0x1
#define AmdExtD3DShaderIntrinsicsBarycentric_Vertex2        0x2

#define AmdExtD3DShaderIntrinsicsBarycentric_Param0         0x00
#define AmdExtD3DShaderIntrinsicsBarycentric_Param1         0x01
#define AmdExtD3DShaderIntrinsicsBarycentric_Param2         0x02
#define AmdExtD3DShaderIntrinsicsBarycentric_Param3         0x03
#define AmdExtD3DShaderIntrinsicsBarycentric_Param4         0x04
#define AmdExtD3DShaderIntrinsicsBarycentric_Param5         0x05
#define AmdExtD3DShaderIntrinsicsBarycentric_Param6         0x06
#define AmdExtD3DShaderIntrinsicsBarycentric_Param7         0x07
#define AmdExtD3DShaderIntrinsicsBarycentric_Param8         0x08
#define AmdExtD3DShaderIntrinsicsBarycentric_Param9         0x09
#define AmdExtD3DShaderIntrinsicsBarycentric_Param10        0x0a
#define AmdExtD3DShaderIntrinsicsBarycentric_Param11        0x0b
#define AmdExtD3DShaderIntrinsicsBarycentric_Param12        0x0c
#define AmdExtD3DShaderIntrinsicsBarycentric_Param13        0x0d
#define AmdExtD3DShaderIntrinsicsBarycentric_Param14        0x0e
#define AmdExtD3DShaderIntrinsicsBarycentric_Param15        0x0f
#define AmdExtD3DShaderIntrinsicsBarycentric_Param16        0x10
#define AmdExtD3DShaderIntrinsicsBarycentric_Param17        0x11
#define AmdExtD3DShaderIntrinsicsBarycentric_Param18        0x12
#define AmdExtD3DShaderIntrinsicsBarycentric_Param19        0x13
#define AmdExtD3DShaderIntrinsicsBarycentric_Param20        0x14
#define AmdExtD3DShaderIntrinsicsBarycentric_Param21        0x15
#define AmdExtD3DShaderIntrinsicsBarycentric_Param22        0x16
#define AmdExtD3DShaderIntrinsicsBarycentric_Param23        0x17
#define AmdExtD3DShaderIntrinsicsBarycentric_Param24        0x18
#define AmdExtD3DShaderIntrinsicsBarycentric_Param25        0x19
#define AmdExtD3DShaderIntrinsicsBarycentric_Param26        0x1a
#define AmdExtD3DShaderIntrinsicsBarycentric_Param27        0x1b
#define AmdExtD3DShaderIntrinsicsBarycentric_Param28        0x1c
#define AmdExtD3DShaderIntrinsicsBarycentric_Param29        0x1d
#define AmdExtD3DShaderIntrinsicsBarycentric_Param30        0x1e
#define AmdExtD3DShaderIntrinsicsBarycentric_Param31        0x1f

#define AmdExtD3DShaderIntrinsicsBarycentric_ComponentX     0x0
#define AmdExtD3DShaderIntrinsicsBarycentric_ComponentY     0x1
#define AmdExtD3DShaderIntrinsicsBarycentric_ComponentZ     0x2
#define AmdExtD3DShaderIntrinsicsBarycentric_ComponentW     0x3

#define AmdExtD3DShaderIntrinsicsBarycentric_ParamShift     0
#define AmdExtD3DShaderIntrinsicsBarycentric_ParamMask      0x1f
#define AmdExtD3DShaderIntrinsicsBarycentric_VtxShift       0x5
#define AmdExtD3DShaderIntrinsicsBarycentric_VtxMask        0x3
#define AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift 0x7
#define AmdExtD3DShaderIntrinsicsBarycentric_ComponentMask  0x3

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsAtomic defines for supported operations. Can be used as the parameter for the
*   AmdExtD3DShaderIntrinsicsOpcode_AtomicU64 intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsAtomicOp_MinU64     0x01
#define AmdExtD3DShaderIntrinsicsAtomicOp_MaxU64     0x02
#define AmdExtD3DShaderIntrinsicsAtomicOp_AndU64     0x03
#define AmdExtD3DShaderIntrinsicsAtomicOp_OrU64      0x04
#define AmdExtD3DShaderIntrinsicsAtomicOp_XorU64     0x05
#define AmdExtD3DShaderIntrinsicsAtomicOp_AddU64     0x06
#define AmdExtD3DShaderIntrinsicsAtomicOp_XchgU64    0x07
#define AmdExtD3DShaderIntrinsicsAtomicOp_CmpXchgU64 0x08

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsFloatConversion defines for supported rounding modes from float to float16 conversions.
*   To be used as an input AmdExtD3DShaderIntrinsicsOpcode_FloatConversion instruction
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16Near    0x01
#define AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16NegInf  0x02
#define AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16PlusInf 0x03

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode defines for supported round modes.
*   To be used as the roundMode parameter for the AmdExtD3DShaderIntrinsicsOpcode_FloatOpWithRoundMode intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TiesToEven     0x0
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardPositive 0x1
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardNegative 0x2
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardZero     0x3

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode defines for supported operations.
*   To be used as the operation parameter for the AmdExtD3DShaderIntrinsicsOpcode_FloatOpWithRoundMode intrinsic.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Add      0x0
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract 0x1
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Multiply 0x2

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode masks and shifts for round mode and operation.
***********************************************************************************************************************
*/
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_ModeShift 0
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_ModeMask  0xf
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_OpShift   4
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_OpMask    0xf

/**
***********************************************************************************************************************
*   MakeAmdShaderIntrinsicsInstruction
*
*   Creates instruction from supplied opcode and immediate data.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint MakeAmdShaderIntrinsicsInstruction(uint opcode, uint opcodePhase, uint immediateData)
{
    return ((AmdExtD3DShaderIntrinsics_MagicCode << AmdExtD3DShaderIntrinsics_MagicCodeShift) |
            (immediateData << AmdExtD3DShaderIntrinsics_DataShift) |
            (opcodePhase << AmdExtD3DShaderIntrinsics_OpcodePhaseShift) |
            (opcode << AmdExtD3DShaderIntrinsics_OpcodeShift));
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ReadfirstlaneF
*
*   Returns the value of float src for the first active lane of the wavefront.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Readfirstlane) returned S_OK.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_ReadfirstlaneF(float src)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Readfirstlane, 0, 0);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), 0, retVal);
    return asfloat(retVal);
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ReadfirstlaneU
*
*   Returns the value of unsigned integer src for the first active lane of the wavefront.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Readfirstlane) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_ReadfirstlaneU(uint src)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Readfirstlane, 0, 0);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src, 0, retVal);
    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Readlane
*
*   Returns the value of float src for the lane within the wavefront specified by laneId.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Readlane) returned S_OK.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_ReadlaneF(float src, uint laneId)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Readlane, 0, laneId);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), 0, retVal);
    return asfloat(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ReadlaneU
*
*   Returns the value of unsigned integer src for the lane within the wavefront specified by laneId.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Readlane) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_ReadlaneU(uint src, uint laneId)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Readlane, 0, laneId);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src, 0, retVal);
    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_LaneId
*
*   Returns the current lane id for the thread within the wavefront.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_LaneId) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_LaneId()
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_LaneId, 0, 0);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);
    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_GetWaveSize
*
*   Returns the wave size for the current shader, including active, inactive and helper lanes.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_GetWaveSize) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_GetWaveSize()
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_GetWaveSize, 0, 0);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);
    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Swizzle
*
*   Generic instruction to shuffle the float src value among different lanes as specified by the operation.
*   Note that the operation parameter must be an immediately specified value not a value from a variable.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Swizzle) returned S_OK.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_SwizzleF(float src, uint operation)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Swizzle, 0, operation);

    uint retVal;
    //InterlockedCompareExchange(AmdExtD3DShaderIntrinsicsUAV[instruction], asuint(src), 0, retVal);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), 0, retVal);
    return asfloat(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_SwizzleU
*
*   Generic instruction to shuffle the unsigned integer src value among different lanes as specified by the operation.
*   Note that the operation parameter must be an immediately specified value not a value from a variable.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Swizzle) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_SwizzleU(uint src, uint operation)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Swizzle, 0, operation);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src, 0, retVal);
    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Ballot
*
*   Given an input predicate returns a bit mask indicating for which lanes the predicate is true.
*   Inactive or non-existent lanes will always return 0.  The number of existent lanes is the wavefront size.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Ballot) returned S_OK.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_Ballot(bool predicate)
{
    uint instruction;

    uint retVal1;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Ballot,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, predicate, 0, retVal1);

    uint retVal2;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Ballot,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_1, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, predicate, 0, retVal2);

    return uint2(retVal1, retVal2);
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_BallotAny
*
*   Convenience routine that uses Ballot and returns true if for any of the active lanes the predicate is true.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Ballot) returned S_OK.
*
***********************************************************************************************************************
*/
bool AmdExtD3DShaderIntrinsics_BallotAny(bool predicate)
{
    uint2 retVal = AmdExtD3DShaderIntrinsics_Ballot(predicate);

    return ((retVal.x | retVal.y) != 0 ? true : false);
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_BallotAll
*
*   Convenience routine that uses Ballot and returns true if for all of the active lanes the predicate is true.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Ballot) returned S_OK.
*
***********************************************************************************************************************
*/
bool AmdExtD3DShaderIntrinsics_BallotAll(bool predicate)
{
    uint2 ballot = AmdExtD3DShaderIntrinsics_Ballot(predicate);

    uint2 execMask = AmdExtD3DShaderIntrinsics_Ballot(true);

    return ((ballot.x == execMask.x) && (ballot.y == execMask.y));
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MBCnt
*
*   Returns the masked bit count of the source register for this thread within all the active threads within a
*   wavefront.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_MBCnt) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_MBCnt(uint2 src)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_MBCnt, 0, 0);

    uint retVal;

    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, src.y, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Min3F
*
*   Returns the minimum value of the three floating point source arguments.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Compare3) returned S_OK.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_Min3F(float src0, float src1, float src2)
{
    uint minimum;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Min3F,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, asuint(src0), asuint(src1), minimum);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Min3F,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, asuint(src2), minimum, minimum);

    return asfloat(minimum);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Min3U
*
*   Returns the minimum value of the three unsigned integer source arguments.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Compare3) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_Min3U(uint src0, uint src1, uint src2)
{
    uint minimum;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Min3U,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, src0, src1, minimum);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Min3U,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, src2, minimum, minimum);

    return minimum;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Med3F
*
*   Returns the median value of the three floating point source arguments.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Compare3) returned S_OK.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_Med3F(float src0, float src1, float src2)
{
    uint median;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Med3F,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, asuint(src0), asuint(src1), median);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Med3F,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, asuint(src2), median, median);

    return asfloat(median);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Med3U
*
*   Returns the median value of the three unsigned integer source arguments.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Compare3) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_Med3U(uint src0, uint src1, uint src2)
{
    uint median;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Med3U,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, src0, src1, median);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Med3U,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, src2, median, median);

    return median;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Max3F
*
*   Returns the maximum value of the three floating point source arguments.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Compare3) returned S_OK.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_Max3F(float src0, float src1, float src2)
{
    uint maximum;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Max3F,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, asuint(src0), asuint(src1), maximum);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Max3F,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, asuint(src2), maximum, maximum);

    return asfloat(maximum);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Max3U
*
*   Returns the maximum value of the three unsigned integer source arguments.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Compare3) returned S_OK.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_Max3U(uint src0, uint src1, uint src2)
{
    uint maximum;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Max3U,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, src0, src1, maximum);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Max3U,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, src2, maximum, maximum);

    return maximum;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_IjBarycentricCoords
*
*   Returns the (i, j) barycentric coordinate pair for this shader invocation with the specified interpolation mode at
*   the specified pixel location.  Should not be used for "pull-model" interpolation, PullModelBarycentricCoords should
*   be used instead
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_BaryCoord) returned S_OK.
*
*   Can only be used in pixel shader stages.
*
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_IjBarycentricCoords(uint interpMode)
{
    uint2 retVal;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaryCoord,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           interpMode);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, 0, 0, retVal.x);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaryCoord,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           interpMode);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, retVal.x, 0, retVal.y);

    return float2(asfloat(retVal.x), asfloat(retVal.y));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_PullModelBarycentricCoords
*
*   Returns the (1/W,1/I,1/J) coordinates at the pixel center which can be used for custom interpolation at any
*   location in the pixel.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_BaryCoord) returned S_OK.
*
*   Can only be used in pixel shader stages.
*
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_PullModelBarycentricCoords()
{
    uint3 retVal;

    uint instruction1 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaryCoord,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                           AmdExtD3DShaderIntrinsicsBarycentric_PerspPullModel);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction1, 0, 0, retVal.x);

    uint instruction2 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaryCoord,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                           AmdExtD3DShaderIntrinsicsBarycentric_PerspPullModel);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction2, retVal.x, 0, retVal.y);

    uint instruction3 = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaryCoord,
                                                           AmdExtD3DShaderIntrinsicsOpcodePhase_2,
                                                           AmdExtD3DShaderIntrinsicsBarycentric_PerspPullModel);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction3, retVal.y, 0, retVal.z);

    return float3(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_VertexParameter
*
*   Returns the triangle's parameter information at the specified triangle vertex.
*   The vertex and parameter indices must specified as immediate values.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_VtxParam) returned S_OK.
*
*   Only available in pixel shader stages.
*
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_VertexParameter(uint vertexIdx, uint parameterIdx)
{
    uint4 retVal;
    uint4 instruction;

    instruction.x = MakeAmdShaderIntrinsicsInstruction(
             AmdExtD3DShaderIntrinsicsOpcode_VtxParam,
             AmdExtD3DShaderIntrinsicsOpcodePhase_0,
           ((vertexIdx << AmdExtD3DShaderIntrinsicsBarycentric_VtxShift) |
            (parameterIdx << AmdExtD3DShaderIntrinsicsBarycentric_ParamShift) |
            (AmdExtD3DShaderIntrinsicsBarycentric_ComponentX << AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift)));
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction.x, 0, 0, retVal.x);

    instruction.y = MakeAmdShaderIntrinsicsInstruction(
             AmdExtD3DShaderIntrinsicsOpcode_VtxParam,
             AmdExtD3DShaderIntrinsicsOpcodePhase_0,
           ((vertexIdx << AmdExtD3DShaderIntrinsicsBarycentric_VtxShift) |
            (parameterIdx << AmdExtD3DShaderIntrinsicsBarycentric_ParamShift) |
            (AmdExtD3DShaderIntrinsicsBarycentric_ComponentY << AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift)));
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction.y, 0, 0, retVal.y);

    instruction.z = MakeAmdShaderIntrinsicsInstruction(
             AmdExtD3DShaderIntrinsicsOpcode_VtxParam,
             AmdExtD3DShaderIntrinsicsOpcodePhase_0,
           ((vertexIdx << AmdExtD3DShaderIntrinsicsBarycentric_VtxShift) |
            (parameterIdx << AmdExtD3DShaderIntrinsicsBarycentric_ParamShift) |
            (AmdExtD3DShaderIntrinsicsBarycentric_ComponentZ << AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift)));
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction.z, 0, 0, retVal.z);

    instruction.w = MakeAmdShaderIntrinsicsInstruction(
             AmdExtD3DShaderIntrinsicsOpcode_VtxParam,
             AmdExtD3DShaderIntrinsicsOpcodePhase_0,
           ((vertexIdx << AmdExtD3DShaderIntrinsicsBarycentric_VtxShift) |
            (parameterIdx << AmdExtD3DShaderIntrinsicsBarycentric_ParamShift) |
            (AmdExtD3DShaderIntrinsicsBarycentric_ComponentW << AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift)));
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction.w, 0, 0, retVal.w);

    return float4(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z), asfloat(retVal.w));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_VertexParameterComponent
*
*   Returns the triangle's parameter information at the specified triangle vertex and component.
*   The vertex, parameter and component indices must be specified as immediate values.
*
*   Available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_VtxParam) returned S_OK.
*
*   Only available in pixel shader stages.
*
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_VertexParameterComponent(uint vertexIdx, uint parameterIdx, uint componentIdx)
{
    uint retVal;
    uint instruction =
        MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_VtxParam,
                                           AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                          ((vertexIdx << AmdExtD3DShaderIntrinsicsBarycentric_VtxShift) |
                                           (parameterIdx << AmdExtD3DShaderIntrinsicsBarycentric_ParamShift) |
                                           (componentIdx << AmdExtD3DShaderIntrinsicsBarycentric_ComponentShift)));
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);

    return asfloat(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveReduce) returned S_OK.
*
*   Performs reduction operation on wavefront (thread group) data.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : float
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, float src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);
    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), 0, retVal);

    return asfloat(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : float2
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, float2 src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    uint2 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.y), 0, retVal.y);

    return float2(asfloat(retVal.x), asfloat(retVal.y));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : float3
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, float3 src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    uint3 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.z), 0, retVal.z);

    return float3(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : float4
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, float4 src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    uint4 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.z), 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.w), 0, retVal.w);

    return float4(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z), asfloat(retVal.w));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : int
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, int src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    int retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src, 0, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : int2
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, int2 src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    int2 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.y, 0, retVal.y);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : int3
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, int3 src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    int3 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.y, 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.z, 0, retVal.z);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveReduce : int4
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveReduce(uint waveOp, int4 src, uint clusterSize)
{
    const uint waveReduceOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                              (clusterSize << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveReduce,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveReduceOp);

    int4 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.y, 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.z, 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.w, 0, retVal.w);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_WaveScan) returned S_OK.
*
*   Performs scan operation on wavefront (thread group) data.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : float
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, float src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags  << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);
    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), 0, retVal);

    return asfloat(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : float2
***********************************************************************************************************************
*/
float2 AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, float2 src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    uint2 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.y), 0, retVal.y);

    return float2(asfloat(retVal.x), asfloat(retVal.y));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : float3
***********************************************************************************************************************
*/
float3 AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, float3 src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    uint3 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.z), 0, retVal.z);

    return float3(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : float4
***********************************************************************************************************************
*/
float4 AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, float4 src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    uint4 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.z), 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src.w), 0, retVal.w);

    return float4(asfloat(retVal.x), asfloat(retVal.y), asfloat(retVal.z), asfloat(retVal.w));
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : int
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, int src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    int retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src, 0, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : int2
***********************************************************************************************************************
*/
int2 AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, int2 src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    int2 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.y, 0, retVal.y);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : int3
***********************************************************************************************************************
*/
int3 AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, int3 src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    int3 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.y, 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.z, 0, retVal.z);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_WaveScan : int4
***********************************************************************************************************************
*/
int4 AmdExtD3DShaderIntrinsics_WaveScan(uint waveOp, uint flags, int4 src)
{
    const uint waveScanOp = (waveOp << AmdExtD3DShaderIntrinsicsWaveOp_OpcodeShift) |
                            (flags << AmdExtD3DShaderIntrinsicsWaveOp_FlagShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_WaveScan,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          waveScanOp);

    int4 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.x, 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.y, 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.z, 0, retVal.z);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src.w, 0, retVal.w);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_LoadDwordAtAddr
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_LoadDwAtAddr) returned S_OK.
*
*   Loads a DWORD from GPU memory from a given 64-bit GPU VA and 32-bit offset.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_LoadDwordAtAddr
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_LoadDwordAtAddr(uint gpuVaLoBits, uint gpuVaHiBits, uint offset)
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_LoadDwAtAddr,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, gpuVaLoBits, gpuVaHiBits, retVal);

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_LoadDwAtAddr,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                     0);

    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, offset, 0, retVal);

    return retVal;
}

// Flags for the IntersectBvhNode flags argument
#define AmdExtIntersectBvhNodeFlag_BoxSortEnable      1
#define AmdExtIntersectBvhNodeFlag_ReturnBarycentrics 2

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_IntersectCommon
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_IntersectCommon(
    in uint   opCode,
    in uint2  nodePointer,
    in float  rayExtent,
    in float3 rayOrigin,
    in float3 rayDir,
    in float3 rayInvDir,
    in uint   flags,
    in uint   boxExpansionUlp)
{
    // Phase 0 - input values
    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(opCode, AmdExtD3DShaderIntrinsicsOpcodePhase_0, 0);

    const uint inputDataCount = 14;
    uint inputData[inputDataCount] = {
        nodePointer.x,
        nodePointer.y,
        asuint(rayExtent),
        asuint(rayOrigin.x),
        asuint(rayOrigin.y),
        asuint(rayOrigin.z),
        asuint(rayDir.x),
        asuint(rayDir.y),
        asuint(rayDir.z),
        asuint(rayInvDir.x),
        asuint(rayInvDir.y),
        asuint(rayInvDir.z),
        asuint(flags),
        asuint(boxExpansionUlp),
    };

    uint dummyOutput;

    // This [unroll] is necessary to ensure the index passed to InterlockedCompareExchange is a constant in the DXIL.
    [unroll]
    for (uint inputIndex = 0; inputIndex < inputDataCount; inputIndex++)
    {
        AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction,
                                                                inputIndex,
                                                                inputData[inputIndex],
                                                                dummyOutput);
    }

    // Phase 1 - output values
    instruction = MakeAmdShaderIntrinsicsInstruction(opCode, AmdExtD3DShaderIntrinsicsOpcodePhase_1, 0);

    const uint outputDataCount = 4;
    uint outputData[outputDataCount];

    [unroll]
    for (uint outputIndex = 0; outputIndex < outputDataCount; outputIndex++)
    {
        AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, outputIndex, 0, outputData[outputIndex]);
    }

    return uint4(outputData[0], outputData[1], outputData[2], outputData[3]);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_IntersectInternal
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_IntersectInternal(
    in uint2  nodePointer,
    in float  rayExtent,
    in float3 rayOrigin,
    in float3 rayDir,
    in float3 rayInvDir,
    in uint   flags,
    in uint   boxExpansionUlp)
{
    return AmdExtD3DShaderIntrinsics_IntersectCommon(AmdExtD3DShaderIntrinsicsOpcode_IntersectInternal,
                                                     nodePointer,
                                                     rayExtent,
                                                     rayOrigin,
                                                     rayDir,
                                                     rayInvDir,
                                                     flags,
                                                     boxExpansionUlp);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_IntersectBvhNode
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_IntersectBvhNode) returned
*   S_OK.
*
*   Intersects a ray with a HW-format BVH node.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_IntersectBvhNode(
    in uint2  nodePointer,
    in float  rayExtent,
    in float3 rayOrigin,
    in float3 rayDir,
    in float3 rayInvDir,
    in uint   flags,
    in uint   boxExpansionUlp)
{
    return AmdExtD3DShaderIntrinsics_IntersectCommon(AmdExtD3DShaderIntrinsicsOpcode_IntersectBvhNode,
                                                     nodePointer,
                                                     rayExtent,
                                                     rayOrigin,
                                                     rayDir,
                                                     rayInvDir,
                                                     flags,
                                                     boxExpansionUlp);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_GetDrawIndex
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_DrawIndex) returned S_OK.
*
*   Returns the 0-based draw index in an indirect draw. Always returns 0 for direct draws.
*
*   Available in vertex shader stage only.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_GetDrawIndex()
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_DrawIndex,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_GetBaseInstance
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_BaseInstance) returned S_OK.
*
*   Returns the StartInstanceLocation parameter passed to direct or indirect drawing commands.
*
*   Available in vertex shader stage only.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_GetBaseInstance()
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaseInstance,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_GetBaseVertex
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_BaseVertex) returned S_OK.
*
*   For non-indexed draw commands, returns the StartVertexLocation parameter. For indexed draw commands, returns the
*   BaseVertexLocation parameter.
*
*   Available in vertex shader stage only.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_GetBaseVertex()
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BaseVertex,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ReadlaneAt : uint
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_ReadlaneAt) returned S_OK.
*
*   Returns the value of the source for the given lane index within the specified wave.  The lane index
*   can be non-uniform across the wave.
*
***********************************************************************************************************************
*/
uint AmdExtD3DShaderIntrinsics_ReadlaneAt(uint src, uint laneId)
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, src, laneId, retVal);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ReadlaneAt : int
***********************************************************************************************************************
*/
int AmdExtD3DShaderIntrinsics_ReadlaneAt(int src, uint laneId)
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), laneId, retVal);

    return asint(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ReadlaneAt : float
***********************************************************************************************************************
*/
float AmdExtD3DShaderIntrinsics_ReadlaneAt(float src, uint laneId)
{
    uint retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src), laneId, retVal);

    return asfloat(retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ConvertF32toF16
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_FloatConversion) returned
*   S_OK.
*
*   Converts 32bit floating point numbers into 16bit floating point number using a specified rounding mode
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ConvertF32toF16 - helper to convert f32 to f16 number
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_ConvertF32toF16(in uint convOp, in float3 val)
{
    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_FloatConversion,
                                                          AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                          convOp);

    uint3 retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(val.x), 0, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(val.y), 0, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(val.z), 0, retVal.z);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ConvertF32toF16Near - convert f32 to f16 number using nearest rounding mode
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_ConvertF32toF16Near(in float3 inVec)
{
    return AmdExtD3DShaderIntrinsics_ConvertF32toF16(AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16Near, inVec);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ConvertF32toF16Near - convert f32 to f16 number using -inf rounding mode
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_ConvertF32toF16NegInf(in float3 inVec)
{
    return AmdExtD3DShaderIntrinsics_ConvertF32toF16(AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16NegInf, inVec);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ConvertF32toF16Near - convert f32 to f16 number using +inf rounding mode
***********************************************************************************************************************
*/
uint3 AmdExtD3DShaderIntrinsics_ConvertF32toF16PosInf(in float3 inVec)
{
    return AmdExtD3DShaderIntrinsics_ConvertF32toF16(AmdExtD3DShaderIntrinsicsFloatConversionOp_FToF16PlusInf, inVec);
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ShaderClock
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_ShaderClock) returned
*   S_OK.
*
*   Returns the current value of the timestamp clock. The value monotonically increments and will wrap after it
*   exceeds the maximum representable value. The units are not defined and need not be constant, and the value
*   is not guaranteed to be dynamically uniform across a single draw or dispatch.
*
*   The function serves as a code motion barrier. Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ShaderClock
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_ShaderClock()
{
    uint2 retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ShaderClock,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal.x);

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ShaderClock,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal.y);

    return retVal;
}


/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ShaderRealtimeClock
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_ShaderRealtimeClock)
*   returned S_OK.
*
*   Returns a value representing the real-time clock that is globally coherent by all invocations on the GPU.
*   The units are not defined and the value will wrap after exceeding the maximum representable value.
*
*   The function serves as a code motion barrier. Available in all shader stages.
*
***********************************************************************************************************************
*/

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ShaderRealtimeClock
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_ShaderRealtimeClock()
{
    uint2 retVal;

    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ShaderRealtimeClock,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_0,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal.x);

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ShaderRealtimeClock,
                                                     AmdExtD3DShaderIntrinsicsOpcodePhase_1,
                                                     0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal.y);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_Halt
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_Halt) returned S_OK.
*
*   Halts shader execution.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_Halt()
{
    uint retVal;
    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_Halt, 0, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);
}

/**
*************************************************************************************************************
*   AmdDxExtShaderIntrinsics_BufferStoreByte
*
*   Writes one byte (low order 8 bits of value) to the buffer UAV at the specified address.
*
*   The function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_BufferStoreByte) returned S_OK.
*
*   Available in all shader stages.
*
*************************************************************************************************************
*/
void AmdDxExtShaderIntrinsics_BufferStoreByte(RWByteAddressBuffer uav, uint address, uint value)
{
    uint retVal;
    uint instruction;

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BufferStoreByte,
        AmdExtD3DShaderIntrinsicsOpcodePhase_0, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, address, value, retVal);
    uav.Store(address, retVal);

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BufferStoreByte,
        AmdExtD3DShaderIntrinsicsOpcodePhase_1, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);
}

/**
*************************************************************************************************************
*   AmdDxExtShaderIntrinsics_BufferStoreShort
*
*   Writes one short (low order 16 bits of value) to the buffer UAV at the specified address.
*
*   The function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_BufferStoreShort) returned S_OK.
*
*   Available in all shader stages.
*
*************************************************************************************************************
*/
void AmdDxExtShaderIntrinsics_BufferStoreShort(RWByteAddressBuffer uav, uint address, uint value)
{
    uint retVal;
    uint instruction;

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BufferStoreShort,
        AmdExtD3DShaderIntrinsicsOpcodePhase_0, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, address, value, retVal);
    uav.Store(address, retVal);

    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_BufferStoreShort,
        AmdExtD3DShaderIntrinsicsOpcodePhase_1, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, 0, 0, retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_ShaderMarker
*
*   The following function is available if CheckSupport(AmdExtD3DShaderIntrinsicsSupport_ShaderMarker) returned S_OK.
*
*   Inserts custom user data marker.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
void AmdExtD3DShaderIntrinsics_ShaderMarker(in uint data)
{
    uint retVal;
    uint instruction;
    instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_ShaderMarker, 0, 0);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, data, 0, retVal);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode) returned
*   S_OK.
*
*   Performs add, subtract or multiply based on specified operation on 32bit floating point numbers and returns a 32bit
*   floating point number that is rounded to nearest, towards positive infinity, towards negative infinity or towards
*   zero based on specified round mode.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/

float AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(uint roundMode, uint operation, float src0, float src1)
{
    const uint immediateData = (operation << AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_OpShift) |
                               (roundMode << AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_ModeShift);

    uint instruction = MakeAmdShaderIntrinsicsInstruction(AmdExtD3DShaderIntrinsicsOpcode_FloatOpWithRoundMode,
                                                          0,
                                                          immediateData);

    uint retVal;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instruction, asuint(src0), asuint(src1), retVal);

    return asfloat(retVal);
}

float2 AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(uint roundMode, uint operation, float2 src0, float2 src1)
{
    float2 retVal;
    retVal.x = AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(roundMode, operation, src0.x, src1.x);
    retVal.y = AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(roundMode, operation, src0.y, src1.y);

    return retVal;
}

float3 AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(uint roundMode, uint operation, float3 src0, float3 src1)
{
    float3 retVal;
    retVal.x = AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(roundMode, operation, src0.x, src1.x);
    retVal.y = AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(roundMode, operation, src0.y, src1.y);
    retVal.z = AmdExtD3DShaderIntrinsics_FloatOpWithRoundMode(roundMode, operation, src0.z, src1.z);

    return retVal;
}

#endif // _AMDEXTD3DSHADERINTRINICS_HLSL
