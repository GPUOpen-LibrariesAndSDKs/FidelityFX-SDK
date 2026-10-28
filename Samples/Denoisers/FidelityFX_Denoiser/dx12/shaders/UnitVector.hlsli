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

// rr::SNormToUNorm
// rr::UNormToSNorm
#include "Numeric.hlsli"
// rr::F32x2
// rr::F32x3
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    F32x2 EncodeUnitVector_Octahedron(F32x3 u)
    {
        u.xy /= (abs(u.x) + abs(u.y) + abs(u.z));
        F32x2 k;
        k.x = 0.0f < u.x ? 1.0f : -1.0f;
        k.y = 0.0f < u.y ? 1.0f : -1.0f;
        if (u.z < 0.0f)
        {
            u.xy = (1.0f - abs(u.yx)) * k;
        }
        return SNormToUNorm(u.xy);
    }

    F32x3 DecodeUnitVector_Octahedron(F32x2 uv)
    {
        uv = UNormToSNorm(uv);
        F32x3 u = F32x3(uv, 1.0f - abs(uv.x) - abs(uv.y));
        const F32 t = saturate(-u.z);
        F32x2 k;
        k.x = 0.0f <= u.x ? -t : t;
        k.y = 0.0f <= u.y ? -t : t;
        u.xy += k;
        return normalize(u);
    }
}
