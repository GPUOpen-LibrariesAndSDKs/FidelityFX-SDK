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

#pragma once

//------------------------------------------------------------------------------
// Sample Defines
//------------------------------------------------------------------------------

#define PACKED_INSTANCE_SLOT 3
#define PACKED_MATERIAL_SLOT 4
#define PACKED_SURFACE_SLOT  5

#define IBV_BEGIN_SLOT     1010
#define SAMPLER_BEGIN_SLOT 10
#define TEXTURE_BEGIN_SLOT 10
#define VBV_BEGIN_SLOT     21010

#define MAX_IBV_COUNT     20000
#define MAX_SAMPLER_COUNT 20
#define MAX_TEXTURE_COUNT 1000
#define MAX_VBV_COUNT     20000

#ifndef GROUP_THREAD_COUNT_X
#define GROUP_THREAD_COUNT_X 8
#else  // GROUP_THREAD_COUNT_X
#error GROUP_THREAD_COUNT_X cannot be predefined.
#endif // GROUP_THREAD_COUNT_X

#ifndef GROUP_THREAD_COUNT_Y
#define GROUP_THREAD_COUNT_Y 8
#else  // GROUP_THREAD_COUNT_Y
#error GROUP_THREAD_COUNT_Y cannot be predefined.
#endif // GROUP_THREAD_COUNT_Y

#ifndef GROUP_THREAD_COUNT_Z
#define GROUP_THREAD_COUNT_Z 1
#else  // GROUP_THREAD_COUNT_Z
#error GROUP_THREAD_COUNT_Z cannot be predefined.
#endif // GROUP_THREAD_COUNT_Z
