// AMD Cauldron code
//
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#if __cplusplus
    #pragma once
    #include <shaders/shadercommon.h>
#else
    #include "shadercommon.h"
#endif // __cplusplus

#if __cplusplus
struct TonemapperCBData
{
    mutable float       Exposure = 1.0f;
    mutable uint32_t    ToneMapper = 0;
    float               DisplayMaxLuminance;
    DisplayMode         MonitorDisplayMode;
    Mat4                ContentToMonitorRecMatrix;
};
#else
cbuffer TonemapperCBData : register(b0)
{
    float Exposure                      : packoffset(c0.x);
    uint  ToneMapper                    : packoffset(c0.y);
    float DisplayMaxLuminance           : packoffset(c0.z);
    DisplayMode MonitorDisplayMode      : packoffset(c0.w);
    matrix ContentToMonitorRecMatrix    : packoffset(c1);
}
#endif // __cplusplus
