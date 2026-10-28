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
// Sample Includes
//------------------------------------------------------------------------------

// rr::F32
// rr::F32x3
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    F32x3 Hue(F32 h)
    {
        const F32 r =        abs(h * 6.0f - 3.0f) - 1.0f;
        const F32 g = 2.0f - abs(h * 6.0f - 2.0f);
        const F32 b = 2.0f - abs(h * 6.0f - 4.0f);

        return saturate(F32x3(r, g, b));
    }

    F32x3 HsvToRgb(F32x3 hsv)
    {
        return ((Hue(hsv.x) - 1.0f) * hsv.y + 1.0f) * hsv.z;
    }
}
