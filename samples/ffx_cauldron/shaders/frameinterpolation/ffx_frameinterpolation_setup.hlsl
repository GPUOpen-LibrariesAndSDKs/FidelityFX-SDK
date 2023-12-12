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
#define FFX_FRAMEINTERPOLATION_BIND_SRV_CURR_BB                                 12
#define FFX_FRAMEINTERPOLATION_BIND_SRV_CURR_BB_DOWN                            13
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPAINTING_PYRAMID                      14

#define FFX_FRAMEINTERPOLATION_BIND_UAV_GAME_MOTION_VECTOR_FIELD_X				0
#define FFX_FRAMEINTERPOLATION_BIND_UAV_GAME_MOTION_VECTOR_FIELD_Y				1
#define FFX_FRAMEINTERPOLATION_BIND_UAV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X		2
#define FFX_FRAMEINTERPOLATION_BIND_UAV_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y		3
#define FFX_FRAMEINTERPOLATION_BIND_UAV_DISOCCLUSION_MASK						4
#define FFX_FRAMEINTERPOLATION_BIND_UAV_CURR_BB_DOWN                            5
#define FFX_FRAMEINTERPOLATION_BIND_UAV_COUNTERS                                6

#include "frameinterpolation/ffx_frameinterpolation_defines.h"

[numthreads(8, 8, 1)]
void CS(uint2 dtID : SV_DispatchThreadID) {

    if(all(dtID == 0)) {
        if(Reset() || HasSceneChanged()) {
            rw_counters[int2(COUNTER_FRAME_INDEX_SINCE_LAST_RESET,0)] = 0;
        } else {
            rw_counters[int2(COUNTER_FRAME_INDEX_SINCE_LAST_RESET,0)]++;
        }
    }

    float2 fDilatedMotionVector = r_dilated_motion_vectors[dtID];
    float  fDilatedDepth = r_dilated_depth[dtID];

#if 1
    FfxFloat32x2 fOfVector = SampleOpticalFlowMotionVector(dtID);
#else
    FfxInt32x2 fOpticalFlowSize = (1.0f / opticalFlowScale) / FfxFloat32x2(opticalFlowBlockSize.xx);
    float2 s = DisplaySize() / RenderSize();
    FfxFloat32x2 fOfVector = r_optical_flow[(dtID * s) / opticalFlowBlockSize] * opticalFlowScale;
#endif

    //fOfVector = 0;

#if 0

    float2 fMotionVector = fDilatedMotionVector;
    float2 fMotionVectorHalf = fMotionVector * 0.5f;


    float2 fUv = (dtID + float(0.5)) / RenderSize();
    bool bNegOutside = any((fUv - fMotionVector) < 0.0) || any((fUv - fMotionVector) > 1.0f);
    bool bPosOutside = any((fUv + fMotionVector) < 0.0) || any((fUv + fMotionVector) > 1.0f);

    bool isBorder = (bNegOutside || bPosOutside);

    rw_currBB_downsampled[dtID] = r_currBB.SampleLevel(s_LinearClamp, fUv, 0);

    //fMotionVectorHalf *= isBorder;

    //rw_debug_out[dtID] = float4(0, fMotionVectorHalf * RenderSize(), 1);

    //rw_debug_out[dtID] = float4(0, isBorder, 0, 1);

    //rw_debug_out[dtID] = float4(fDilatedMotionVector * RenderSize(), 0, 1);
    //return;

    uint maxVal = 0xFFFF;
    uint offset = 0;
    uint uPriorityFactor = 0;

    uint2 packedVector = PackVectorFieldEntries(uPriorityFactor, fMotionVectorHalf);

    rw_game_motion_vector_field_x[dtID] = packedVector.x;
    rw_game_motion_vector_field_y[dtID] = packedVector.y;
#else
    rw_game_motion_vector_field_x[dtID] = 0;
    rw_game_motion_vector_field_y[dtID] = 0;
#endif

    // TEMP, what to fill these with?
    rw_optical_flow_motion_vector_field_x[dtID] = 0;
    rw_optical_flow_motion_vector_field_y[dtID] = 0;

    rw_disocclusion_mask[dtID] = 0;

    //rw_debug_out[dtID] = float4(fOfVector * RenderSize(), 0, 1);

    //rw_debug_out[dtID] = float4(rw_dilated_motion_vectors[dtID] * DisplaySize(), 0, 1);
}
