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

// rr::UNormToSNorm
#include "Numeric.hlsli"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x3x3
// rr::F32x4
// rr::F32x4x4
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    F32x3x3 CreateRotation3x3(F32 radAngle, F32x3 axis)
    {
        F32 c, s;
        sincos(radAngle, s, c);

        // right-handed -> left-handed
        s = -s;

        const F32 t = 1.0f - c;
        const F32 x = axis.x;
        const F32 y = axis.y;
        const F32 z = axis.z;

        return F32x3x3(t * x * x + c,
                       t * x * y - s * z,
                       t * x * z + s * y,
                       t * x * y + s * z,
                       t * y * y + c,
                       t * y * z - s * x,
                       t * x * z - s * y,
                       t * y * z + s * x,
                       t * z * z + c);
    }

    F32x2 ScreenSpaceToNdcSpace(F32x2 uv)
    {
        return UNormToSNorm(F32x2(uv.x, 1.0f - uv.y));
    }

    F32x3 NdcSpaceToViewSpace(F32x3 pNdc, F32x4x4 clipToView)
    {
        const F32x4 pClip = F32x4(pNdc.xyz, 1.0f);
        const F32x4 pView = mul(clipToView, pClip);
        // Perspective divide
        return pView.xyz / pView.w;
    }

    F32x3 NdcSpaceToWorldSpace(F32x3 pNdc, F32x4x4 clipToWorld)
    {
        const F32x4 pClip  = F32x4(pNdc.xyz, 1.0f);
        const F32x4 pWorld = mul(clipToWorld, pClip);
        // Perspective divide
        return pWorld.xyz / pWorld.w;
    }
}
