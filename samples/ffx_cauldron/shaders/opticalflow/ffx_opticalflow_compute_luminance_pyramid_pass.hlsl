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


#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT                0
#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT_LEVEL_1        1
#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT_LEVEL_2        2
#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT_LEVEL_3        3
#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT_LEVEL_4        4
#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT_LEVEL_5        5
#define FFX_OPTICALFLOW_BIND_UAV_OPTICAL_FLOW_INPUT_LEVEL_6        6
//#define FFX_OPTICALFLOW_BIND_UAV_SPD_GLOBAL_ATOMIC                 7

#define FFX_OPTICALFLOW_BIND_CB_COMMON                             0
#define FFX_OPTICALFLOW_BIND_CB_SPD                                1

#include "opticalflow/ffx_opticalflow_callbacks_hlsl.h"

cbuffer cbSPD : FFX_OPTICALFLOW_DECLARE_CB(FFX_OPTICALFLOW_BIND_CB_SPD) {

    uint    mips;
    uint    numWorkGroups;
    uint2   workGroupOffset;
    uint    numWorkGroupOpticalFlowInputPyramid;
    uint    pad0_;
    uint    pad1_;
    uint    pad2_;
};

uint NumWorkGroups()
{
    return numWorkGroupOpticalFlowInputPyramid;
}

void SPD_SetMipmap(int2 iPxPos, int index, float value)
{
    switch (index)
    {
    case 0: 
        rw_optical_flow_input_level_1[iPxPos] = value;
        break;
    case 1: 
        rw_optical_flow_input_level_2[iPxPos] = value;
        break;
    case 2: 
        rw_optical_flow_input_level_3[iPxPos] = value;
        break;
    case 3: 
        rw_optical_flow_input_level_4[iPxPos] = value;
        break;
    case 4:
        rw_optical_flow_input_level_5[iPxPos] = value;
        break;
    case 5:
        rw_optical_flow_input_level_6[iPxPos] = value;
        break;
    }
}

void SPD_IncreaseAtomicCounter(inout uint spdCounter)
{
    // only required when L7, L8, ... need to be computed
    // but for now only 7 levels pyramid are used
    //InterlockedAdd(rw_spd_global_atomic[min16int2(0,0)], 1, spdCounter);
}

void SPD_ResetAtomicCounter()
{
    // only required when L7, L8, ... need to be computed
    // but for now only 7 levels pyramid are used
    //rw_spd_global_atomic[min16int2(0,0)] = 0;
}

#include "opticalflow/ffx_opticalflow_compute_luminance_pyramid.h"

#ifndef FFX_OPTICALFLOW_THREAD_GROUP_WIDTH
#define FFX_OPTICALFLOW_THREAD_GROUP_WIDTH 256
#endif // #ifndef FFX_OPTICALFLOW_THREAD_GROUP_WIDTH
#ifndef FFX_OPTICALFLOW_THREAD_GROUP_HEIGHT
#define FFX_OPTICALFLOW_THREAD_GROUP_HEIGHT 1
#endif // #ifndef FFX_OPTICALFLOW_THREAD_GROUP_HEIGHT
#ifndef FFX_OPTICALFLOW_THREAD_GROUP_DEPTH
#define FFX_OPTICALFLOW_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_OPTICALFLOW_THREAD_GROUP_DEPTH
#ifndef FFX_OPTICALFLOW_NUM_THREADS
#define FFX_OPTICALFLOW_NUM_THREADS [numthreads(FFX_OPTICALFLOW_THREAD_GROUP_WIDTH, FFX_OPTICALFLOW_THREAD_GROUP_HEIGHT, FFX_OPTICALFLOW_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFX_OPTICALFLOW_NUM_THREADS

FFX_OPTICALFLOW_NUM_THREADS
FFX_OPTICALFLOW_EMBED_CB2_ROOTSIG_CONTENT
void CS(int2 iGlobalId   : SV_DispatchThreadID,
        int2 iGroupId    : SV_GroupID,
        int  iLocalIndex : SV_GroupIndex)
{
    ComputeOpticalFlowInputPyramid(iGroupId, iLocalIndex);

}
