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
// rr::F32x2
// rr::F32x3
// rr::F32x3x3
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    F32x2 Orthonormal(F32x2 n)
    {
        return F32x2(-n.y, n.x);
    }

    F32x3 Orthonormal(F32x3 n)
    {
        // [Source]
        // T. Duff, J. Burgess, P. Christensen, C. Hery, A. Kensler, M. Liani,
        // R. Villemin: Building an Orthonormal Basis, Revisited, Journal of
        // Computer Graphics Techniques 6, 1, 2017.

        const F32 s  = (0.0f > n.z) ? -1.0f : 1.0f; // copysignf
        const F32 a0 = -1.0f / (s + n.z);
        const F32 a1 = n.x * n.y * a0;

        const F32x3 t = { 1.0f + s * n.x * n.x * a0, s * a1, -s * n.x };

        return t;
    }

    F32x3x3 OrthonormalBasis(F32x3 n)
    {
        // [Source]
        // T. Duff, J. Burgess, P. Christensen, C. Hery, A. Kensler, M. Liani,
        // R. Villemin: Building an Orthonormal Basis, Revisited, Journal of
        // Computer Graphics Techniques 6, 1, 2017.

        const F32 s  = (0.0f > n.z) ? -1.0f : 1.0f; // copysignf
        const F32 a0 = -1.0f / (s + n.z);
        const F32 a1 = n.x * n.y * a0;

        const F32x3 t = { 1.0f + s * n.x * n.x * a0, s * a1, -s * n.x };
        const F32x3 b = { a1, s + n.y * n.y * a0, -n.y };

        // Left-handed basis
        return F32x3x3(t, -b, n);
    }
}
