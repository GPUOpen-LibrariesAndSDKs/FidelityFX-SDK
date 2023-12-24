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
#define FFX_FRAMEINTERPOLATION_BIND_SRV_PREV_BB									12
#define FFX_FRAMEINTERPOLATION_BIND_SRV_CURR_BB									13
#define FFX_FRAMEINTERPOLATION_BIND_SRV_OUTPUT									14
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID                      15

#define FFX_FRAMEINTERPOLATION_BIND_UAV_COUNTERS        						0
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_0				1
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_1				2
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_2				3
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_3				4
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_4				5
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_5				6
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_6				7
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_7				8
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_8				9
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_9				10
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_10			11
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_11			12
#define FFX_FRAMEINTERPOLATION_BIND_UAV_INPAINTING_PYRAMID_MIPMAP_12			13

#ifdef FFX_HALF
    #undef FFX_HALF
    #define FFX_HALF 0
#endif

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

#define FI_BIND_CB_INPAINTING_PYRAMID 1
cbuffer cbInpaintingPyramid : FFX_FI_DECLARE_CB(FI_BIND_CB_INPAINTING_PYRAMID)
{
    uint mips;
    uint numWorkGroups;
    uint2 workGroupOffset;
}

//--------------------------------------------------------------------------------------
// Buffer definitions - global atomic counter
//--------------------------------------------------------------------------------------

groupshared uint spdCounter;
groupshared float spdIntermediateR[16][16];
groupshared float spdIntermediateG[16][16];
groupshared float spdIntermediateB[16][16];
groupshared float spdIntermediateA[16][16];

float4 SpdLoadSourceImage(float2 tex, uint slice)
{
    VectorFieldEntry gameMv;
    LoadGameFieldMv(tex, gameMv);

    //rw_debug_out[tex] = float4(abs(gameMv.fMotionVector) * RenderSize(), 0, 1);
    //rw_debug_out[tex] = float4(gameMv.fMotionVector * RenderSize(), gameMv.uDepthPriorityFactor, gameMv.uColorPriorityFactor);

    return float4(gameMv.fMotionVector, gameMv.uDepthPriorityFactor, gameMv.uColorPriorityFactor) * (DisplaySize().x > 0);;
}

float4 SpdLoad(float2 tex, uint slice)
{
    return imgDst5[tex];
}

void SpdStore(int2 pix, float4 outValue, uint index, uint slice)
{
#define STORE(idx)                   \
    if (index == idx)                \
    {                                \
        imgDst##idx[pix] = outValue; \
    }

    STORE(0);
    STORE(1);
    STORE(2);
    STORE(3);
    STORE(4);
    STORE(5);
    STORE(6);
    STORE(7);
    STORE(8);
    STORE(9);
    STORE(10);
    STORE(11);
    STORE(12);
}

void SpdIncreaseAtomicCounter(uint slice)
{
    InterlockedAdd(rw_counters[int2(COUNTER_SPD,0)], 1, spdCounter);
}

uint SpdGetAtomicCounter()
{
    return spdCounter;
}
void SpdResetAtomicCounter(uint slice)
{
    rw_counters[int2(COUNTER_SPD,0)] = 0;
}

float4 SpdLoadIntermediate(uint x, uint y)
{
    return float4(
        spdIntermediateR[x][y],
        spdIntermediateG[x][y],
        spdIntermediateB[x][y],
        spdIntermediateA[x][y]);
}

void SpdStoreIntermediate(uint x, uint y, float4 value)
{
    spdIntermediateR[x][y] = value.x;
    spdIntermediateG[x][y] = value.y;
    spdIntermediateB[x][y] = value.z;
    spdIntermediateA[x][y] = value.w;
}

float4 SpdReduce4(float4 v0, float4 v1, float4 v2, float4 v3)
{
    float4 vec = float4(0,0,0,0);

    float fWeightSum = 0.0f;
#define ADD(SAMPLE) { \
        float fWeight = (SAMPLE.z > 0.0f) * pow(1.0f - SAMPLE.z, 3.0f) /* higher weights on far-away samples */; \
        vec += SAMPLE * fWeight; \
        fWeightSum += fWeight; \
        }

    ADD(v0);
    ADD(v1);
    ADD(v2);
    ADD(v3);

    vec /= (fWeightSum > FSR2_EPSILON) ? fWeightSum : 1.0f;

    return vec;
}

#include "spd/ffx_spd.h"

[numthreads(256, 1, 1)]
void CS(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex)
{
    SpdDownsample(
        uint2(WorkGroupId.xy),
        uint1(LocalThreadIndex),
        uint1(mips),
        uint1(numWorkGroups),
        uint1(WorkGroupId.z),
        uint2(workGroupOffset));
}
