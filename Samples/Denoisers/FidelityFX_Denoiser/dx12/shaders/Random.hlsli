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
// rr::U32
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    U32 XxHash(U32 state)
    {
        const U32 PRIME32_2 = 2246822519u;
        const U32 PRIME32_3 = 3266489917u;
        const U32 PRIME32_4 = 668265263u;
        const U32 PRIME32_5 = 374761393u;
        U32 h = state + PRIME32_5;
        h = PRIME32_4 * ((h << 17u) | (h >> (32u - 17u)));
        h = PRIME32_2 * (h ^ (h >> 15u));
        h = PRIME32_3 * (h ^ (h >> 13u));
        return h ^ (h >> 16u);
    }

    U32 XxHash(U32 x, U32 y, U32 t)
    {
        const U32 PRIME32_2 = 2246822519u;
        const U32 PRIME32_3 = 3266489917u;
        const U32 PRIME32_4 = 668265263u;
        const U32 PRIME32_5 = 374761393u;
        U32 h = x + PRIME32_5 + t * PRIME32_3;
        h = PRIME32_4 * ((h << 17u) | (h >> (32u - 17u)));
        h += y * PRIME32_3;
        h = PRIME32_4 * ((h << 17u) | (h >> (32u - 17u)));
        h = PRIME32_2 * (h ^ (h >> 15u));
        h = PRIME32_3 * (h ^ (h >> 13u));
        return h ^ (h >> 16u);
    }

    U32 SeedRngState(U32 x, U32 y, U32 t)
    {
        return XxHash(x, y, t);
    }

    F32 UniformU32ToUNorm(U32 u)
    {
        // IEEE-754: 2^-32 = 0x2F800000
        return F32(u) * asfloat(0x2F800000u);
    }

    F32 UniformUNorm(inout U32 state)
    {
        state = XxHash(state);
        return UniformU32ToUNorm(state);
    }
}
