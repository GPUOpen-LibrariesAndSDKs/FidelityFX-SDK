// This file is part of the FidelityFX SDK.
//
// Copyright © 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Pull in framework defines
#include "surfacerendercommon.h"
#include "shadercommon.h"

//////////////////////////////////////////////////////////////////////////
// Constant Buffers
//////////////////////////////////////////////////////////////////////////
cbuffer CBSceneInformation : register(b0)
{
    SceneInformation SceneInfo;
};
cbuffer CBInstanceInformation : register(b1)
{
    InstanceInformation InstanceInfo;
};
cbuffer CBTextureIndices : register(b2)
{
    TextureIndices Textures;
}

//------------------------------------------------------------------------
// Useful functions
matrix GetWorldMatrix()
{
    return InstanceInfo.WorldTransform;
}

matrix GetCameraViewProj()
{
    return SceneInfo.CameraInfo.ViewProjectionMatrix;
}

matrix GetPrevWorldMatrix()
{
    return InstanceInfo.PrevWorldTransform;
}

matrix GetPrevCameraViewProj()
{
    return SceneInfo.CameraInfo.PrevViewProjectionMatrix;
}

//--------------------------------------------------------------------------------------
// Surface input parameter combinations
//--------------------------------------------------------------------------------------
struct VS_SURFACE_INPUT
{
    // Always have a vertex position
    float3 Position     : POSITION;

    #ifdef HAS_NORMAL
        float3 Normal       :    NORMAL;
    #endif // HAS_NORMAL

    #ifdef HAS_TANGENT
        float4 Tangent      :    TANGENT;
    #endif // HAS_TANGENT

    #ifdef HAS_TEXCOORD_0
        float2 UV0          :    TEXCOORD0;
    #endif // HAS_TEXCOORD_0

    #ifdef HAS_TEXCOORD_1
        float2 UV1          :    TEXCOORD1;
    #endif // HAS_TEXCOORD_1

    #ifdef HAS_COLOR_0
        float4 Color0       :    COLOR0;
    #endif // HAS_COLOR_0

        // Skinning related information
    #ifdef HAS_WEIGHTS_0
        float4 Weights0     :    WEIGHTS0;
    #endif // HAS_WEIGHTS_0

    #ifdef HAS_WEIGHTS_1
        float4 Weights1     :    WEIGHTS1;
    #endif // HAS_WEIGHTS_1

    #ifdef HAS_JOINTS_0
        uint4 Joints0       :    JOINTS0;
    #endif // HAS_JOINTS_0

    #ifdef HAS_JOINTS_1
        uint4 Joints1       :    JOINTS1;
    #endif // HAS_JOINTS_1
};

//--------------------------------------------------------------------------------------
// MainVS
//--------------------------------------------------------------------------------------
VS_SURFACE_OUTPUT MainVS(VS_SURFACE_INPUT surfaceInput)
{
    VS_SURFACE_OUTPUT output;


// TODO: Add support for skinning
// #ifdef HAS_WEIGHTS_0
//     matrix skinningMatrix;
//     skinningMatrix = GetCurrentSkinningMatrix(input.Weights0, input.Joints0);
// #ifdef HAS_WEIGHTS_1
//     skinningMatrix += GetCurrentSkinningMatrix(input.Weights1, input.Joints1);
// #endif
// #else
//     matrix skinningMatrix =
//     {
//         { 1, 0, 0, 0 },
//         { 0, 1, 0, 0 },
//         { 0, 0, 1, 0 },
//         { 0, 0, 0, 1 }
//     };
// #endif
//     matrix transMatrix = mul(GetWorldMatrix(), skinningMatrix);

    // Transform geometry
    matrix worldTransform = GetWorldMatrix();

    float3 worldPos = mul(worldTransform,      float4(surfaceInput.Position, 1)).xyz;

#ifdef HAS_WORLDPOS
    output.WorldPos = worldPos;
#endif // HAS_WORLDPOS

    output.Position = mul(GetCameraViewProj(), float4(worldPos, 1));


// #ifdef HAS_WEIGHTS_0
//     matrix prevSkinningMatrix;
//     prevSkinningMatrix = GetPreviousSkinningMatrix(input.Weights0, input.Joints0);
// #ifdef HAS_WEIGHTS_1
//     prevSkinningMatrix += GetPreviousSkinningMatrix(input.Weights1, input.Joints1);
// #endif
// #else
//     matrix prevSkinningMatrix =
//     {
//         { 1, 0, 0, 0 },
//         { 0, 1, 0, 0 },
//         { 0, 0, 1, 0 },
//         { 0, 0, 0, 1 }
//     };
// #endif


//     transMatrix = mul(GetWorldMatrix(), skinningMatrix);
//     matrix prevTransMatrix = mul(GetPrevWorldMatrix(), prevSkinningMatrix);

#ifdef HAS_MOTION_VECTORS
    output.CurPosition = output.Position;

    const float4 worldPrevPos = mul(GetPrevWorldMatrix(),    float4(surfaceInput.Position, 1));
    output.PrevPosition =       mul(GetPrevCameraViewProj(), worldPrevPos);
#endif // HAS_MOTION_VECTORS


#ifdef HAS_NORMAL
    // Note - this assumes we have no scaling in our transformation.
    output.Normal = normalize(mul(worldTransform, float4(surfaceInput.Normal, 0)).xyz);
#endif

#ifdef HAS_TANGENT
    output.Tangent = normalize(mul(worldTransform, float4(surfaceInput.Tangent.xyz, 0)).xyz);
    output.Binormal = cross(output.Normal, output.Tangent) * surfaceInput.Tangent.w;
#endif

#ifdef HAS_COLOR_0
    output.Color0 = surfaceInput.Color0;
#endif

#ifdef HAS_TEXCOORD_0
    output.UV0 = surfaceInput.UV0;
#endif

#ifdef HAS_TEXCOORD_1
    output.UV1 = surfaceInput.UV1;
#endif

    return output;
}

