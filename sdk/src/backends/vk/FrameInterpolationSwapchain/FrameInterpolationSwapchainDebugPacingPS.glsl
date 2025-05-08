// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
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

#version 450

layout(push_constant) uniform uBlock
{
    uint frameIndex;
};

layout(location = 0) out vec4 outColor;

void main()
{
    float a = frameIndex & 1;
    float b = (~frameIndex) & 1;

    if (gl_FragCoord.x < 16.0)
    {
        // Alternate between magenta and green 
        outColor = vec4(b, a, b, 1.0f);
    }
    else if (gl_FragCoord.x < 32.0)
    {
        // Alternate between black and white
        outColor =  vec4(a, a, a, 1.0);
    }
    else
    {
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
