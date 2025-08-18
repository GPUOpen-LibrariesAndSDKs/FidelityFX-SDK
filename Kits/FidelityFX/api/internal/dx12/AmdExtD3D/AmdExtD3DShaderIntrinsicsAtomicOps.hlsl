/* Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef _AMDEXTD3DSHADERINTRINICSATOMIC_HLSL
#define _AMDEXTD3DSHADERINTRINICSATOMIC_HLSL

#include "AmdExtD3DShaderIntrinsics.hlsl"

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_MakeAtomicInstructions
*
*   Creates uint4 with x/y/z/w components containing phase 0/1/2/3 for atomic instructions.
*   NOTE: This is an internal function and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint4 AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(uint op)
{
    uint4 instructions;
    instructions.x = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_AtomicU64, AmdExtD3DShaderIntrinsicsOpcodePhase_0, op);
    instructions.y = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_AtomicU64, AmdExtD3DShaderIntrinsicsOpcodePhase_1, op);
    instructions.z = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_AtomicU64, AmdExtD3DShaderIntrinsicsOpcodePhase_2, op);
    instructions.w = MakeAmdShaderIntrinsicsInstruction(
        AmdExtD3DShaderIntrinsicsOpcode_AtomicU64, AmdExtD3DShaderIntrinsicsOpcodePhase_3, op);
    return instructions;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicOp
*
*   Creates intrinstic instructions for the specified atomic op.
*   NOTE: These are internal functions and should not be called by the source HLSL shader directly.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicOp(RWByteAddressBuffer uav, uint3 address, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x, address.y, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z, value.x,   retVal.y);
    uav.Store(retVal.x, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,   retVal.y,  retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(RWTexture1D<uint2> uav, uint3 address, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x, address.y, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z, value.x,   retVal.y);
    uav[retVal.x] = retVal.y;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,   retVal.y,  retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(RWTexture2D<uint2> uav, uint3 address, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x, address.y, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z, value.x,   retVal.y);
    uav[uint2(retVal.x, retVal.x)] = retVal.y;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,   retVal.y,  retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(RWTexture3D<uint2> uav, uint3 address, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x, address.y, retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z, value.x,   retVal.y);
    uav[uint3(retVal.x, retVal.x, retVal.x)] = retVal.y;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,   retVal.y,  retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(
    RWByteAddressBuffer uav, uint3 address, uint2 compare_value, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x,       address.y,       retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z,       value.x,         retVal.y);
    uav.Store(retVal.x, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,         compare_value.x, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.w, compare_value.y, retVal.y,        retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(
    RWTexture1D<uint2> uav, uint3 address, uint2 compare_value, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x,       address.y,       retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z,       value.x,         retVal.y);
    uav[retVal.x] = retVal.y;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,         compare_value.x, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.w, compare_value.y, retVal.y,        retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(
    RWTexture2D<uint2> uav, uint3 address, uint2 compare_value, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x,       address.y,       retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z,       value.x,         retVal.y);
    uav[uint2(retVal.x, retVal.x)] = retVal.y;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,         compare_value.x, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.w, compare_value.y, retVal.y,        retVal.y);

    return retVal;
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOp(
    RWTexture3D<uint2> uav, uint3 address, uint2 compare_value, uint2 value, uint op)
{
    uint2 retVal;

    const uint4 instructions = AmdExtD3DShaderIntrinsics_MakeAtomicInstructions(op);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.x, address.x,       address.y,       retVal.x);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.y, address.z,       value.x,         retVal.y);
    uav[uint3(retVal.x, retVal.x, retVal.x)] = retVal.y;
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.z, value.y,         compare_value.x, retVal.y);
    AmdExtD3DShaderIntrinsicsUAV.InterlockedCompareExchange(instructions.w, compare_value.y, retVal.y,        retVal.y);

    return retVal;
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicMinU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic minimum of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicMinU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MinU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicMinU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MinU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicMinU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MinU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicMinU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MinU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicMaxU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic maximum of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicMaxU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MaxU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicMaxU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MaxU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicMaxU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MaxU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicMaxU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_MaxU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicAndU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic AND of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicAndU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AndU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicAndU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AndU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicAndU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AndU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicAndU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AndU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicOrU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic OR of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicOrU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_OrU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOrU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_OrU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOrU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_OrU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicOrU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_OrU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicXorU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic XOR of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicXorU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XorU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicXorU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XorU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicXorU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XorU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicXorU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XorU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicAddU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic add of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicAddU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AddU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicAddU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AddU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicAddU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AddU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicAddU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_AddU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicXchgU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic exchange of value with the UAV at address, returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicXchgU64(RWByteAddressBuffer uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicXchgU64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicXchgU64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicXchgU64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_XchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), value, op);
}

/**
***********************************************************************************************************************
*   AmdExtD3DShaderIntrinsics_AtomicCmpXchgU64
*
*   The following functions are available if CheckSupport(AmdExtD3DShaderIntrinsicsOpcode_AtomicU64) returned S_OK.
*
*   Performs 64-bit atomic compare of comparison value with UAV at address, stores value if values match,
*   returns the original value.
*
*   Available in all shader stages.
*
***********************************************************************************************************************
*/
uint2 AmdExtD3DShaderIntrinsics_AtomicCmpXchgU64(
    RWByteAddressBuffer uav, uint address, uint2 compare_value, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_CmpXchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), compare_value, value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicCmpXchgU64(
    RWTexture1D<uint2> uav, uint address, uint2 compare_value, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_CmpXchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address, 0, 0), compare_value, value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicCmpXchgU64(
    RWTexture2D<uint2> uav, uint2 address, uint2 compare_value, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_CmpXchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, 0), compare_value, value, op);
}

uint2 AmdExtD3DShaderIntrinsics_AtomicCmpXchgU64(
    RWTexture3D<uint2> uav, uint3 address, uint2 compare_value, uint2 value)
{
    const uint op = AmdExtD3DShaderIntrinsicsAtomicOp_CmpXchgU64;
    return AmdExtD3DShaderIntrinsics_AtomicOp(uav, uint3(address.x, address.y, address.z), compare_value, value, op);
}
#endif // _AMDEXTD3DSHADERINTRINICSATOMIC_HLSL
