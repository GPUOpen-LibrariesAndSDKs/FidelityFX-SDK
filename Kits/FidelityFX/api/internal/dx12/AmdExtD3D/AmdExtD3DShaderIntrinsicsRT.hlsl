/* Copyright (c) 2020-2025 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef _AMDEXTD3DSHADERINTRINICSRT_HLSL
#define _AMDEXTD3DSHADERINTRINICSRT_HLSL

/**
***********************************************************************************************************************
* @file  AmdExtD3DShaderIntrinsicsRT.hlsl
* @brief
*    AMD D3D Shader Intrinsic RT API include file.
*    This include file contains the Shader Intrinsics RT definitions (structures, enums, constant)
*    shared between the driver and the application.
***********************************************************************************************************************
*/

//=====================================================================================================================
struct AmdExtRtHitToken
{
    uint dword[2];
};

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsRT structure when included in a Ray Tracing payload will indicate to the driver
*    that the dwords are already supplied in AmdExtRtHitTokenIn and only requires a call to intersect
*    ray, bypassing the traversal of the acceleration structure.
***********************************************************************************************************************
*/
struct AmdExtRtHitTokenIn : AmdExtRtHitToken { };

/**
***********************************************************************************************************************
* @brief
*    AmdExtD3DShaderIntrinsicsRT structure when included in a Ray Tracing payload will indicate to the driver
*    that the dwords must be patched into the payload after traversal.  The application can store this
*    data in a buffer which can then be used for hit group sorting so shading divergence can be avoided.
***********************************************************************************************************************
*/
struct AmdExtRtHitTokenOut : AmdExtRtHitToken { };

/**
***********************************************************************************************************************
* @brief
*    Group shared memory reserved for temprary storage of hit tokens. Not intended to touched by the app shader.
*    Application shader must only use the extension functions defined below to access the hit tokens
*
***********************************************************************************************************************
*/
groupshared AmdExtRtHitToken AmdHitToken;

/**
***********************************************************************************************************************
* @brief
*    Accessor function to obtain the hit tokens from the last call to TraceRays(). The data returned by this
*    function only guarantees valid values for the last call to TraceRays() prior to calling this function.
*
***********************************************************************************************************************
*/
uint2 AmdGetLastHitToken()
{
    return uint2(AmdHitToken.dword[0], AmdHitToken.dword[1]);
}

/**
***********************************************************************************************************************
* @brief
*    This function initialises hit tokens for subsequent TraceRays() call. Note, any TraceRay() that intends to use
*    these hit tokens must include this function call in the same basic block. Applications can use a convenience macro
*    defined below to enforce that.
*
***********************************************************************************************************************
*/
void AmdSetHitToken(uint2 token)
{
    AmdHitToken.dword[0] = token.x;
    AmdHitToken.dword[1] = token.y;
}

/**
***********************************************************************************************************************
* @brief
*    Convenience macro for calling TraceRays that uses the hit token
*
***********************************************************************************************************************
*/
#define AmdTraceRay(accelStruct,                    \
                    rayFlags,                       \
                    instanceInclusionMask,          \
                    rayContributionToHitGroupIndex, \
                    geometryMultiplier,             \
                    missShaderIndex,                \
                    ray,                            \
                    payload,                        \
                    token)                          \
AmdSetHitToken(token);                              \
TraceRay(accelStruct,                               \
         rayFlags,                                  \
         instanceInclusionMask,                     \
         rayContributionToHitGroupIndex,            \
         geometryMultiplier,                        \
         missShaderIndex,                           \
         ray,                                       \
         payload);                                  \
#endif // _AMDEXTD3DSHADERINTRINICSRT_HLSL
