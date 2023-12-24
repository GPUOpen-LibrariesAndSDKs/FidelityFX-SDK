// This file is part of the FidelityFX SDK.
//
// Copyright © 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "fullscreen.hlsl"

Texture2D<float4> TextureImg : register(t0);

float4 CopyTextureToSwapChainPS(VertexOut VertexIn) : SV_Target
{
    float4 color = TextureImg[VertexIn.PosOut.xy];
    return color;
}

Texture2D<float4> inputTexture  : register(t0);
SamplerState linearSampler      : register(s0);
float4 CopyTexturePS(VertexOut input) : SV_Target
{
    float2 texCoord = input.UVOut;
    return inputTexture.Sample(linearSampler, texCoord);
}
