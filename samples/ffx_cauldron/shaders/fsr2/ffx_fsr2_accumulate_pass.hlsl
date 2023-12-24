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


#define FSR2_BIND_SRV_INPUT_EXPOSURE                         0
#define FSR2_BIND_SRV_DILATED_REACTIVE_MASKS                 1
#if FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
#define FSR2_BIND_SRV_DILATED_MOTION_VECTORS                 2
#else
#define FSR2_BIND_SRV_INPUT_MOTION_VECTORS                   2
#endif
#define FSR2_BIND_SRV_INTERNAL_UPSCALED                      3
#define FSR2_BIND_SRV_LOCK_STATUS                            4
#define FSR2_BIND_SRV_PREPARED_INPUT_COLOR                   5
#define FSR2_BIND_SRV_LANCZOS_LUT                            6
#define FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT               7
#define FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS                   8
#define FSR2_BIND_SRV_AUTO_EXPOSURE                          9
#define FSR2_BIND_SRV_LUMA_HISTORY                           10

#define FSR2_BIND_UAV_INTERNAL_UPSCALED                      0
#define FSR2_BIND_UAV_LOCK_STATUS                            1
#define FSR2_BIND_UAV_UPSCALED_OUTPUT                        2
#define FSR2_BIND_UAV_NEW_LOCKS                              3
#define FSR2_BIND_UAV_LUMA_HISTORY                           4

#define FSR2_BIND_CB_FSR2                                    0

#include "fsr2/ffx_fsr2_callbacks_hlsl.h"
#include "fsr2/ffx_fsr2_common.h"
#include "fsr2/ffx_fsr2_sample.h"
#include "fsr2/ffx_fsr2_upsample.h"
#include "fsr2/ffx_fsr2_postprocess_lock_status.h"
#include "fsr2/ffx_fsr2_reproject.h"
#include "fsr2/ffx_fsr2_accumulate.h"

#ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#define FFX_FSR2_THREAD_GROUP_WIDTH 8
#endif // #ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#ifndef FFX_FSR2_THREAD_GROUP_HEIGHT
#define FFX_FSR2_THREAD_GROUP_HEIGHT 8
#endif // FFX_FSR2_THREAD_GROUP_HEIGHT
#ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#define FFX_FSR2_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#ifndef FFX_FSR2_NUM_THREADS
#define FFX_FSR2_NUM_THREADS [numthreads(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT, FFX_FSR2_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFX_FSR2_NUM_THREADS

FFX_PREFER_WAVE64
FFX_FSR2_NUM_THREADS
FFX_FSR2_EMBED_ROOTSIG_CONTENT
void CS(uint2 uGroupId : SV_GroupID, uint2 uGroupThreadId : SV_GroupThreadID)
{
    const uint GroupRows = (uint(DisplaySize().y) + FFX_FSR2_THREAD_GROUP_HEIGHT - 1) / FFX_FSR2_THREAD_GROUP_HEIGHT;
    uGroupId.y = GroupRows - uGroupId.y - 1;

    uint2 uDispatchThreadId = uGroupId * uint2(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT) + uGroupThreadId;

    Accumulate(uDispatchThreadId);
}
