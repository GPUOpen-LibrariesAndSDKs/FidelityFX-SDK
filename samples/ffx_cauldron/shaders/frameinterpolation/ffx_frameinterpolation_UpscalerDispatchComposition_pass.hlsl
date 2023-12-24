// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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

Texture2D<float4> rInputs[] : register(t0);
RWTexture2D<float4> rwOutput : register(u0);

cbuffer cbFI : register(b0)
{
    int2    renderSize;
    int2    displaySize;

    float2  displaySizeRcp;
    float   cameraNear;
    float   cameraFar;

    int2    upscalerTargetSize;
    int     Mode;
    int     Reset;

    float4  fDeviceToViewDepth;

    float   deltaTime;
    float2  motionVectorScale;
    float   HUDLessAttachedFactor;

    float2  opticalFlowScale;
    int     opticalFlowBlockSize;
    int     reserved_0;

    int     opticalFlowHalfResMode;
    int2    maxRenderSize;
    int     NumInstances;
    
    int2    interpolationRectBase;
    int2    interpolationRectSize;
}

[numthreads(8,8,1)]
void CS(uint2 dtID : SV_DispatchThreadID)
{
    int instanceIndex = -1;

    int instanceWidth = displaySize.x / NumInstances;
    if (abs((dtID.x / instanceWidth) * instanceWidth - dtID.x) > 1)
    {
        instanceIndex = dtID.x / instanceWidth;
    }

    float3 color = 0;

    if (instanceIndex >= 0) {
        color = rInputs[instanceIndex][dtID].rgb;
    }

    rwOutput[dtID] = float4(color, 1);
}
