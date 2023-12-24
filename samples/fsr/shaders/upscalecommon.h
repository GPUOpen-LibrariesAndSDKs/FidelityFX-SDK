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


#if __cplusplus
    #pragma once
#endif // __cplusplus

namespace UpscaleRM
{
enum class UpscaleMethod : uint32_t
{
    Native = 0,
    Point,
    Bilinear,
    Bicubic,

    Count
};
}
#if __cplusplus
struct UpscaleCBData
{
    UpscaleRM::UpscaleMethod Type = UpscaleRM::UpscaleMethod::Point;
    float invRenderWidth;
    float invRenderHeight;
    float invDisplayWidth;
    float invDisplayHeight;
};
#else
cbuffer UpscaleCBData : register(b0)
{
    uint Type : packoffset(c0.x);
    float invRenderWidth : packoffset(c0.y);
    float invRenderHeight : packoffset(c0.z);
    float invDisplayWidth;
    float invDisplayHeight;
}
#endif // __cplusplus
