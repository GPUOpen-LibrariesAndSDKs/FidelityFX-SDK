// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
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

#ifndef SOURCE_FRAMECTX
#define SOURCE_FRAMECTX

#ifdef __cplusplus
namespace hlsl
{
using uint = uint32_t;
using uint2 = DirectX::XMUINT2;
struct FrameCtx
{
#else
cbuffer FrameCtx : register(b0)
{
#endif

    float kTime;
    int kFrameIdx;
    uint2 kViewportRes;
    uint kMaxTrainSamples;
    float kTrainingRatio;
    uint kRenderFlags;
    int kSplitScreenPartitionX;
    float kAccumMotionBlur;
    float kIndirectRoughening;
    
#ifdef __cplusplus
};
}// namespace hlsl 
#else
}
#endif

#define RENDER_DISABLE_ANIMATION   (1 << 0)
#define RENDER_LOCK_NOISE          (1 << 1)
#define RENDER_ANIMATE_CAMERA      (1 << 2)
#define RENDER_ANIMATE_GEOMETRY    (1 << 3)
#define RENDER_ANIMATE_MATERIALS   (1 << 4)
#define RENDER_ANIMATE_LIGHTS      (1 << 5)

#define SPLIT_VIEW_REFERENCE       0
#define SPLIT_VIEW_RADIANCE_CACHE  1

#endif
