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

#ifndef SOURCE_PCG
#define SOURCE_PCG

/*******************************************************************************************
 
    "Hash Functions for GPU Rendering"
    Jarzynski and Olano
    Journal of Computer Graphics Techniques Vol. 9, No. 3, 2020
    http://jcgt.org/published/0009/03/02/paper.pdf

*******************************************************************************************/

uint4 PCGAdvance(inout RNGCtx rngSeed)
{
    rngSeed = rngSeed * 1664525u + 1013904223u;
    
    rngSeed.x += rngSeed.y*rngSeed.w; 
    rngSeed.y += rngSeed.z*rngSeed.x; 
    rngSeed.z += rngSeed.x*rngSeed.y; 
    rngSeed.w += rngSeed.y*rngSeed.z;
    
    rngSeed ^= rngSeed >> 16u;
    
    rngSeed.x += rngSeed.y*rngSeed.w; 
    rngSeed.y += rngSeed.z*rngSeed.x; 
    rngSeed.z += rngSeed.x*rngSeed.y; 
    rngSeed.w += rngSeed.y*rngSeed.z;
    
    return rngSeed;
}

// Generates a tuple of canonical random number and uses them to sample an input texture
/*float4 Rand4(inout RNGCtx ctx, int2 xy, sampler2D sampler)
{
    return texelFetch(sampler, (xy + int2(PCGAdvance(ctx) >> 16)) % 1024, 0);
}*/

// Seed the PCG hash function with the current frame multipled by a prime
RNGCtx InitRand(uint2 xyFrag, uint frameIdx)
{    
    return uint4(20219u, 7243u, 12547u, 28573u) * HashOf(xyFrag.x, xyFrag.y, frameIdx);
}

// Generates 4 canonical random numbers in the range [0, 1)
float4 Rand4(inout RNGCtx ctx, uint) { return float4(PCGAdvance(ctx)) / float(0xffffffffu); }

// Generates 8 canonical random numbers in the range [0, 1]
void Rand8(inout RNGCtx ctx, uint, out float4 xi[2])
{
    xi[0] = float4(PCGAdvance(ctx)) / float(0xffffffffu);
    xi[1] = float4(PCGAdvance(ctx)) / float(0xffffffffu);
}

#endif // SOURCE_PCG
