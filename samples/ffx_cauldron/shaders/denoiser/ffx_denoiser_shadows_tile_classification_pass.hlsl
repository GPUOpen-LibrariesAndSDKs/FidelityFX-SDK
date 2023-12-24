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


#define FFX_WAVE 1

#define DENOISER_SHADOWS_BIND_SRV_DEPTH                     0
#define DENOISER_SHADOWS_BIND_SRV_VELOCITY                  1
#define DENOISER_SHADOWS_BIND_SRV_NORMAL                    2
#define DENOISER_SHADOWS_BIND_SRV_HISTORY                   3
#define DENOISER_SHADOWS_BIND_SRV_PREVIOUS_DEPTH            4
#define DENOISER_SHADOWS_BIND_SRV_PREVIOUS_MOMENTS          5

#define DENOISER_SHADOWS_BIND_UAV_TILE_METADATA             0
#define DENOISER_SHADOWS_BIND_UAV_REPROJECTION_RESULTS      1
#define DENOISER_SHADOWS_BIND_UAV_CURRENT_MOMENTS           2
#define DENOISER_SHADOWS_BIND_UAV_RAYTRACER_RESULT          3

#define DENOISER_SHADOWS_BIND_CB1_DENOISER_SHADOWS          0

#include "denoiser/ffx_denoiser_shadows_callbacks_hlsl.h"
#include "denoiser/ffx_denoiser_shadows_tileclassification.h"

#ifndef FFX_DENOISER_SHADOWS_THREAD_GROUP_WIDTH
#define FFX_DENOISER_SHADOWS_THREAD_GROUP_WIDTH 8
#endif // #ifndef FFX_DENOISER_SHADOWS_THREAD_GROUP_WIDTH
#ifndef FFX_DENOISER_SHADOWS_THREAD_GROUP_HEIGHT
#define FFX_DENOISER_SHADOWS_THREAD_GROUP_HEIGHT 8
#endif // FFX_DENOISER_SHADOWS_THREAD_GROUP_HEIGHT
#ifndef FFX_DENOISER_SHADOWS_THREAD_GROUP_DEPTH
#define FFX_DENOISER_SHADOWS_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_DENOISER_SHADOWS_THREAD_GROUP_DEPTH
#ifndef FFX_DENOISER_SHADOWS_NUM_THREADS
#define FFX_DENOISER_SHADOWS_NUM_THREADS [numthreads(FFX_DENOISER_SHADOWS_THREAD_GROUP_WIDTH, FFX_DENOISER_SHADOWS_THREAD_GROUP_HEIGHT, FFX_DENOISER_SHADOWS_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFX_DENOISER_SHADOWS_NUM_THREADS

FFX_PREFER_WAVE64
FFX_DENOISER_SHADOWS_NUM_THREADS
FFX_DENOISER_SHADOWS_EMBED_TILE_CLASSIFICATION_ROOTSIG_CONTENT
void CS(uint group_index : SV_GroupIndex, uint2 gid : SV_GroupID)
{
    FFX_DNSR_Shadows_TileClassification(group_index, gid);
}
