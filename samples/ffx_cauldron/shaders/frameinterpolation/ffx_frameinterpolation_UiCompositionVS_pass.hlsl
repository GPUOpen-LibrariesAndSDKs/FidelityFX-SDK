// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define FFX_FRAMEINTERPOLATION_BIND_SRV_GAME_MOTION_VECTOR_FIELD_X				0
#define FFX_FRAMEINTERPOLATION_BIND_SRV_GAME_MOTION_VECTOR_FIELD_Y				1
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X		2
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y		3
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW							4
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_CONFIDENCE					5
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_GLOBAL_MOTION				6
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OPTICAL_FLOW_SCENE_CHANGE_DETECTION		7
#define FFX_FRAMEINTERPOLATION_BIND_SRV_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME		8
#define FFX_FRAMEINTERPOLATION_BIND_SRV_DILATED_MOTION_VECTORS					9
#define FFX_FRAMEINTERPOLATION_BIND_SRV_DILATED_DEPTH							10
#define FFX_FRAMEINTERPOLATION_BIND_SRV_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME	11
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID                      12

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

#define FAR_DEPTH 0.5f

static const float4 FullScreenVertsPos[3] = { float4(-1, 1, FAR_DEPTH, 1), float4(3, 1, FAR_DEPTH, 1), float4(-1, -3, FAR_DEPTH, 1) };
// static const float2 FullScreenVertsUVs[3] = { float2(0, 0), float2(2, 0), float2(0, 2) };

struct VERTEX_OUT
{
//    float2 vTexture : TEXCOORD;
    float4 vPosition : SV_POSITION;
};

VERTEX_OUT mainVS(uint vertexId : SV_VertexID)
{
    VERTEX_OUT Output;
    Output.vPosition = FullScreenVertsPos[vertexId];
//    Output.vTexture = FullScreenVertsUVs[vertexId];
    return Output;
}
