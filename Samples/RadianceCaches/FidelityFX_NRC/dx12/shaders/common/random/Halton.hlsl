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

#ifndef SOURCE_HALTON
#define SOURCE_HALTON

// Reverse the bits of 32-bit inteter
uint RadicalInverse(uint i)
{
    i = ((i & 0xffffu) << 16u) | (i >> 16u);
    i = ((i & 0x00ff00ffu) << 8u) | ((i & 0xff00ff00u) >> 8u);
    i = ((i & 0x0f0f0f0fu) << 4u) | ((i & 0xf0f0f0f0u) >> 4u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xccccccccu) >> 2u);    
    i = ((i & 0x55555555u) << 1u) | ((i & 0xaaaaaaaau) >> 1u);        
    return i;
}

// Samples the radix-2 Halton sequence from seed value, i
float HaltonBase2(uint i)
{    
    return float(RadicalInverse(i)) / float(0xffffffffu);
}

float HaltonBase3(uint seed)
{
    uint accum = 0u;
    accum += 1162261467u * (seed % 3u); seed /= 3u;
    accum += 387420489u * (seed % 3u); seed /= 3u;
    accum += 129140163u * (seed % 3u); seed /= 3u;
    accum += 43046721u * (seed % 3u); seed /= 3u;
    accum += 14348907u * (seed % 3u); seed /= 3u;
    accum += 4782969u * (seed % 3u); seed /= 3u;
    accum += 1594323u * (seed % 3u); seed /= 3u;
    accum += 531441u * (seed % 3u); seed /= 3u;
    accum += 177147u * (seed % 3u); seed /= 3u;
    accum += 59049u * (seed % 3u); seed /= 3u;
    accum += 19683u * (seed % 3u); seed /= 3u;
    accum += 6561u * (seed % 3u); seed /= 3u;
    accum += 2187u * (seed % 3u); seed /= 3u;
    accum += 729u * (seed % 3u); seed /= 3u;
    accum += 243u * (seed % 3u); seed /= 3u;
    accum += 81u * (seed % 3u); seed /= 3u;
    accum += 27u * (seed % 3u); seed /= 3u;
    accum += 9u * (seed % 3u); seed /= 3u;
    accum += 3u * (seed % 3u); seed /= 3u;
    return float(accum + seed % 3u) / 3486784400.0f;
} 

float HaltonBase5(uint seed)
{
    uint accum = 0u;
    accum += 244140625u * (seed % 5u); seed /= 5u;
    accum += 48828125u * (seed % 5u); seed /= 5u;
    accum += 9765625u * (seed % 5u); seed /= 5u;
    accum += 1953125u * (seed % 5u); seed /= 5u;
    accum += 390625u * (seed % 5u); seed /= 5u;
    accum += 78125u * (seed % 5u); seed /= 5u;
    accum += 15625u * (seed % 5u); seed /= 5u;
    accum += 3125u * (seed % 5u); seed /= 5u;
    accum += 625u * (seed % 5u); seed /= 5u;
    accum += 125u * (seed % 5u); seed /= 5u;
    accum += 25u * (seed % 5u); seed /= 5u;
    accum += 5u * (seed % 5u); seed /= 5u;
    return float(accum + seed % 5u) / 1220703124.0f;
}

float HaltonBase7(uint seed)
{
    uint accum = 0u;
    accum += 282475249u * (seed % 7u); seed /= 7u;
    accum += 40353607u * (seed % 7u); seed /= 7u;
    accum += 5764801u * (seed % 7u); seed /= 7u;
    accum += 823543u * (seed % 7u); seed /= 7u;
    accum += 117649u * (seed % 7u); seed /= 7u;
    accum += 16807u * (seed % 7u); seed /= 7u;
    accum += 2401u * (seed % 7u); seed /= 7u;
    accum += 343u * (seed % 7u); seed /= 7u;
    accum += 49u * (seed % 7u); seed /= 7u;
    accum += 7u * (seed % 7u); seed /= 7u;
    return float(accum + seed % 7u) / 1977326742.0f;
}

float HaltonBase11(uint seed)
{
    uint accum = 0u;
    accum += 214358881u * (seed % 11u); seed /= 11u;
    accum += 19487171u * (seed % 11u); seed /= 11u;
    accum += 1771561u * (seed % 11u); seed /= 11u;
    accum += 161051u * (seed % 11u); seed /= 11u;
    accum += 14641u * (seed % 11u); seed /= 11u;
    accum += 1331u * (seed % 11u); seed /= 11u;
    accum += 121u * (seed % 11u); seed /= 11u;
    accum += 11u * (seed % 11u); seed /= 11u;
    return float(accum + seed % 11u) / 2357947690.0f;
}

#endif
