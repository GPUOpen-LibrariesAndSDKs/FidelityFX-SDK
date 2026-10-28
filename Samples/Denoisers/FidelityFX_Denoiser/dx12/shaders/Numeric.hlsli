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
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    static const F32 g_epsilon  = 1e-4f;

    static const F32 g_pi       = 3.141592653589793f;
    static const F32 g_inv_pi   = 0.3183098861837907f;
    static const F32 g_inv_2pi  = 0.50f * g_inv_pi;
    static const F32 g_pi_div_2 = 0.50f * g_pi;
    static const F32 g_pi_div_4 = 0.25f * g_pi;

    static const F32 g_fp16_max = 65504.0f;

    template< typename T >
    T Square(T x)
    {
        return x * x;
    }

    template< typename F >
    F SNormToUNorm(F x)
    {
        // [-1,1] -> [0,1]
        return 0.5 * x + 0.5;
    }

    template< typename F >
    F UNormToSNorm(F x)
    {
        // [0,1] -> [-1,1]
        return 2.0 * x - 1.0;
    }
}
