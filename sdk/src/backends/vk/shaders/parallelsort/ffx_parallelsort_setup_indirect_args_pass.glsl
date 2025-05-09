// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
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

// Setup Indirect Args Pass
// UAV  0 : ParallelSort_Indirect_Count_Scatter_Args    : rw_count_scatter_args
// UAV  1 : ParallelSort_Indirect_Reduce_Scan_Args      : rw_reduce_scan_args
// CBV  0 : cbParallelSort

#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_KHR_shader_subgroup_arithmetic : require

#define FFX_PARALLELSORT_BIND_UAV_COUNT_SCATTER_ARGS    2000
#define FFX_PARALLELSORT_BIND_UAV_REDUCE_SCAN_ARGS      2001

#define FFX_PARALLELSORT_BIND_CB_PARALLEL_SORT          3000

#include "parallelsort/ffx_parallelsort_callbacks_glsl.h"
#include "parallelsort/ffx_parallelsort_common.h"
#include "parallelsort/ffx_parallelsort_setup_indirect_args.h"

#ifndef FFX_PARALLELSORT_THREAD_GROUP_WIDTH
#define FFX_PARALLELSORT_THREAD_GROUP_WIDTH 1
#endif // #ifndef FFX_PARALLELSORT_THREAD_GROUP_WIDTH

#ifndef FFX_PARALLELSORT_THREAD_GROUP_HEIGHT
#define FFX_PARALLELSORT_THREAD_GROUP_HEIGHT 1
#endif // FFX_PARALLELSORT_THREAD_GROUP_HEIGHT

#ifndef FFX_PARALLELSORT_THREAD_GROUP_DEPTH
#define FFX_PARALLELSORT_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_PARALLELSORT_THREAD_GROUP_DEPTH

#ifndef FFX_PARALLELSORT_NUM_THREADS
#define FFX_PARALLELSORT_NUM_THREADS layout (local_size_x = FFX_PARALLELSORT_THREAD_GROUP_WIDTH, local_size_y = FFX_PARALLELSORT_THREAD_GROUP_HEIGHT, local_size_z = FFX_PARALLELSORT_THREAD_GROUP_DEPTH) in;
#endif // #ifndef FFX_PARALLELSORT_NUM_THREADS

FFX_PARALLELSORT_NUM_THREADS
void main()
{
    FfxParallelSortSetupIndirectArgs(gl_LocalInvocationID.x);
}
