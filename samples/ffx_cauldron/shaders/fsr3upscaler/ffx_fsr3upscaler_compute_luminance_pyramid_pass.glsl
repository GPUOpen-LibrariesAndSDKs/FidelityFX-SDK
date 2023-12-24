// This file is part of the FidelityFX SDK.
//
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_samplerless_texture_functions : require

#define FSR3UPSCALER_BIND_SRV_INPUT_COLOR                     0

#define FSR3UPSCALER_BIND_UAV_SPD_GLOBAL_ATOMIC            2001
#define FSR3UPSCALER_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE     2002
#define FSR3UPSCALER_BIND_UAV_EXPOSURE_MIP_5               2003
#define FSR3UPSCALER_BIND_UAV_AUTO_EXPOSURE                2004

#define FSR3UPSCALER_BIND_CB_FSR3UPSCALER                          3000
#define FSR3UPSCALER_BIND_CB_SPD                           3001

#include "fsr3upscaler/ffx_fsr3upscaler_callbacks_glsl.h"
#include "fsr3upscaler/ffx_fsr3upscaler_common.h"
#include "fsr3upscaler/ffx_fsr3upscaler_compute_luminance_pyramid.h"

#ifndef FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH
#define FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH 256
#endif // #ifndef FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH
#ifndef FFX_FSR3UPSCALER_THREAD_GROUP_HEIGHT
#define FFX_FSR3UPSCALER_THREAD_GROUP_HEIGHT 1
#endif // #ifndef FFX_FSR3UPSCALER_THREAD_GROUP_HEIGHT
#ifndef FFX_FSR3UPSCALER_THREAD_GROUP_DEPTH
#define FFX_FSR3UPSCALER_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_FSR3UPSCALER_THREAD_GROUP_DEPTH
#ifndef FFX_FSR3UPSCALER_NUM_THREADS
#define FFX_FSR3UPSCALER_NUM_THREADS layout (local_size_x = FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH, local_size_y = FFX_FSR3UPSCALER_THREAD_GROUP_HEIGHT, local_size_z = FFX_FSR3UPSCALER_THREAD_GROUP_DEPTH) in;
#endif // #ifndef FFX_FSR3UPSCALER_NUM_THREADS

FFX_FSR3UPSCALER_NUM_THREADS
void main()
{
    ComputeAutoExposure(gl_WorkGroupID.xyz, gl_LocalInvocationIndex);
}
