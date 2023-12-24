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

#define FSR3UPSCALER_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH      0
#define FSR3UPSCALER_BIND_SRV_DILATED_MOTION_VECTORS                1
#define FSR3UPSCALER_BIND_SRV_DILATED_DEPTH                         2
#define FSR3UPSCALER_BIND_SRV_REACTIVE_MASK                         3
#define FSR3UPSCALER_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK     4
#define FSR3UPSCALER_BIND_SRV_PREPARED_INPUT_COLOR                  5
#define FSR3UPSCALER_BIND_SRV_PREVIOUS_DILATED_MOTION_VECTORS       6
#define FSR3UPSCALER_BIND_SRV_INPUT_MOTION_VECTORS                  7
#define FSR3UPSCALER_BIND_SRV_INPUT_COLOR                           8
#define FSR3UPSCALER_BIND_SRV_INPUT_DEPTH                           9
#define FSR3UPSCALER_BIND_SRV_INPUT_EXPOSURE                        10

#define FSR3UPSCALER_BIND_UAV_DEPTH_CLIP                          2011
#define FSR3UPSCALER_BIND_UAV_DILATED_REACTIVE_MASKS              2012
#define FSR3UPSCALER_BIND_UAV_PREPARED_INPUT_COLOR                2013

#define FSR3UPSCALER_BIND_CB_FSR3UPSCALER                                 3000

#include "fsr3upscaler/ffx_fsr3upscaler_callbacks_glsl.h"
#include "fsr3upscaler/ffx_fsr3upscaler_common.h"
#include "fsr3upscaler/ffx_fsr3upscaler_sample.h"
#include "fsr3upscaler/ffx_fsr3upscaler_depth_clip.h"

#ifndef FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH
#define FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH 8
#endif // #ifndef FFX_FSR3UPSCALER_THREAD_GROUP_WIDTH
#ifndef FFX_FSR3UPSCALER_THREAD_GROUP_HEIGHT
#define FFX_FSR3UPSCALER_THREAD_GROUP_HEIGHT 8
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
	DepthClip(ivec2(gl_GlobalInvocationID.xy));
}
