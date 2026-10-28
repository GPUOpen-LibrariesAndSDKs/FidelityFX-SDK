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

#ifndef SOURCE_HALF
#define SOURCE_HALF

uint FloatToHalfBits(float f)
{
    uint u = asuint(f);

    // Handle zero as special case
    if (u == 0u)
    {
        return 0u;
    }
    else
    {
        uint expo = (u >> 23) & 0xffu;
        if (expo < 127u - 15u) { expo = 0u; } // Underflow
        else if (expo > 127u + 16u) { expo = 31u; } // Overflow
        else { expo = ((u >> 23) & 0xffu) + 15u - 127u; } // Biased exponent

        // Composite
        return ((u >> 16) & (1u << 15)) |  // Sign bit
            ((u & ((1u << 23) - 1u)) >> 13) | // Fraction
            ((expo & ((1u << 5) - 1u)) << 10); // Exponent
    }
}

float HalfBitsToFloat(uint u)
{
    if (u == 0u) { return 0.; }

    uint v = ((u & (1u << 15)) << 16) | // Sign bit 
             ((u & ((1u << 10u) - 1u)) << 13) | // Fraction
             ((((u >> 10) & ((1u << 5) - 1u)) + 127u - 15u) << 23); // Exponent

    return asfloat(v);
}

float PackFloat2(float a, float b) { return asfloat(FloatToHalfBits(a) | (FloatToHalfBits(b) << 16)); } 
float PackFloat2(float2 v) { return asfloat(FloatToHalfBits(v.x) | (FloatToHalfBits(v.y) << 16)); } 

float2 UnpackFloat2(float f)
{
    return float2(HalfBitsToFloat(asuint(f) & 0xffffu), HalfBitsToFloat(asuint(f) >> 16));
} 

void UnpackFloat2(in float f, out float a, out float b)
{
    a = HalfBitsToFloat(asuint(f) & 0xffffu);
    b = HalfBitsToFloat(asuint(f) >> 16);
} 

#endif
