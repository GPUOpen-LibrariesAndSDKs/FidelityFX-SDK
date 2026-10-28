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

#ifndef SOURCE_OWEN_SOBOL
#define SOURCE_OWEN_SOBOL

#include "Hash.hlsl"

/*******************************************************************************************************
    
    "Practical Hash-based Owen Scrambling"
    Brent Burley, Walt Disney Animation Studios
    Journal of Computer Graphics Techniques, Vol. 9, No. 4, 2020
    https://jcgt.org/published/0009/04/01/

/*******************************************************************************************************/

static const uint kSobolOffsets[160] = 
{
    0x80000000u, 0x40000000u, 0x20000000u, 0x10000000u,
    0x08000000u, 0x04000000u, 0x02000000u, 0x01000000u,
    0x00800000u, 0x00400000u, 0x00200000u, 0x00100000u,
    0x00080000u, 0x00040000u, 0x00020000u, 0x00010000u,
    0x00008000u, 0x00004000u, 0x00002000u, 0x00001000u,
    0x00000800u, 0x00000400u, 0x00000200u, 0x00000100u,
    0x00000080u, 0x00000040u, 0x00000020u, 0x00000010u,
    0x00000008u, 0x00000004u, 0x00000002u, 0x00000001u,

    0x80000000u, 0xc0000000u, 0xa0000000u, 0xf0000000u,
    0x88000000u, 0xcc000000u, 0xaa000000u, 0xff000000u,
    0x80800000u, 0xc0c00000u, 0xa0a00000u, 0xf0f00000u,
    0x88880000u, 0xcccc0000u, 0xaaaa0000u, 0xffff0000u,
    0x80008000u, 0xc000c000u, 0xa000a000u, 0xf000f000u,
    0x88008800u, 0xcc00cc00u, 0xaa00aa00u, 0xff00ff00u,
    0x80808080u, 0xc0c0c0c0u, 0xa0a0a0a0u, 0xf0f0f0f0u,
    0x88888888u, 0xccccccccu, 0xaaaaaaaau, 0xffffffffu,

    0x80000000u, 0xc0000000u, 0x60000000u, 0x90000000u,
    0xe8000000u, 0x5c000000u, 0x8e000000u, 0xc5000000u,
    0x68800000u, 0x9cc00000u, 0xee600000u, 0x55900000u,
    0x80680000u, 0xc09c0000u, 0x60ee0000u, 0x90550000u,
    0xe8808000u, 0x5cc0c000u, 0x8e606000u, 0xc5909000u,
    0x6868e800u, 0x9c9c5c00u, 0xeeee8e00u, 0x5555c500u,
    0x8000e880u, 0xc0005cc0u, 0x60008e60u, 0x9000c590u,
    0xe8006868u, 0x5c009c9cu, 0x8e00eeeeu, 0xc5005555u,

    0x80000000u, 0xc0000000u, 0x20000000u, 0x50000000u,
    0xf8000000u, 0x74000000u, 0xa2000000u, 0x93000000u,
    0xd8800000u, 0x25400000u, 0x59e00000u, 0xe6d00000u,
    0x78080000u, 0xb40c0000u, 0x82020000u, 0xc3050000u,
    0x208f8000u, 0x51474000u, 0xfbea2000u, 0x75d93000u,
    0xa0858800u, 0x914e5400u, 0xdbe79e00u, 0x25db6d00u,
    0x58800080u, 0xe54000c0u, 0x79e00020u, 0xb6d00050u,
    0x800800f8u, 0xc00c0074u, 0x200200a2u, 0x50050093u,

    0x80000000u, 0x40000000u, 0x20000000u, 0xb0000000u,
    0xf8000000u, 0xdc000000u, 0x7a000000u, 0x9d000000u,
    0x5a800000u, 0x2fc00000u, 0xa1600000u, 0xf0b00000u,
    0xda880000u, 0x6fc40000u, 0x81620000u, 0x40bb0000u,
    0x22878000u, 0xb3c9c000u, 0xfb65a000u, 0xddb2d000u,
    0x78022800u, 0x9c0b3c00u, 0x5a0fb600u, 0x2d0ddb00u,
    0xa2878080u, 0xf3c9c040u, 0xdb65a020u, 0x6db2d0b0u,
    0x800228f8u, 0x400b3cdcu, 0x200fb67au, 0xb00ddb9du
};

uint SobolSample(uint i, uint d)
{
    uint x = 0;
    for (uint b = 0u; b < 32u; ++b)
    {
        x ^= uint((i>>b) & 1u) * kSobolOffsets[d*32u+b];
    }
    return x;
}

// Based on Boost hash_combine: https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
uint BoostHashCombine(uint seed, uint v)
{
    return seed ^ (v + (seed << 6) + (seed >> 2));
}

// Stratified Sampling for Stochastic Transparency: https://users.aalto.fi/~laines9/publications/laine2011egsr_paper.pdf
uint LaineKarrasPermutation(uint x, uint init)
{
    x += init;
    x ^= x * 0x6c50b47cu;
    x ^= x * 0xb82f1e52u;
    x ^= x * 0xc7afe638u;
    x ^= x * 0x8d22f6e6u;
    return x;
}

uint NestedUniformScrambleBase2(uint x, uint seed)
{
    x = RadicalInverse(x);
    x = RadicalInverse(LaineKarrasPermutation(RadicalInverse(x), seed));
    x = RadicalInverse(x);
    return x;
}

float4 Rand4(RNGCtx ctx, uint dim)
{
    uint seed = BoostHashCombine(ctx.x, dim);
    uint index = NestedUniformScrambleBase2(ctx.y, seed); 
    return float4(float(NestedUniformScrambleBase2(SobolSample(index, 0), BoostHashCombine(seed, 0))) / float(0xffffffffu),
                  float(NestedUniformScrambleBase2(SobolSample(index, 1), BoostHashCombine(seed, 1))) / float(0xffffffffu),
                  float(NestedUniformScrambleBase2(SobolSample(index, 2), BoostHashCombine(seed, 2))) / float(0xffffffffu),
                  float(NestedUniformScrambleBase2(SobolSample(index, 3), BoostHashCombine(seed, 3))) / float(0xffffffffu));
}

void Rand8(RNGCtx ctx, uint dim, out float4 xi[2])
{
    uint seed = BoostHashCombine(ctx.x, dim);
    uint index = NestedUniformScrambleBase2(ctx.y, seed);
    xi[0] = float4(float(NestedUniformScrambleBase2(SobolSample(index, 0), BoostHashCombine(seed, 0))) / float(0xffffffffu),
                   float(NestedUniformScrambleBase2(SobolSample(index, 1), BoostHashCombine(seed, 1))) / float(0xffffffffu),
                   float(NestedUniformScrambleBase2(SobolSample(index, 2), BoostHashCombine(seed, 2))) / float(0xffffffffu),
                   float(NestedUniformScrambleBase2(SobolSample(index, 3), BoostHashCombine(seed, 3))) / float(0xffffffffu));
    xi[1] = float4(float(NestedUniformScrambleBase2(SobolSample(index, 4), BoostHashCombine(seed, 4))) / float(0xffffffffu), 
                   0, 
                   0, 
                   0);
}

RNGCtx InitRand(uint2 xyFrag, uint frameIdx)
{    
    RNGCtx ctx;
    ctx.x = 98796235u;
    ctx.y = HashOf(xyFrag.x, xyFrag.y) + frameIdx;
    ctx.zw = uint2(0, 0);
    return ctx;
}

#endif
