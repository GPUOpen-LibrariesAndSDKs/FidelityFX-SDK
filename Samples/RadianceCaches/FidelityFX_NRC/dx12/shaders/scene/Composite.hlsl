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

#include "FrameCtx.hlsl"
#include "Math.hlsl"
#include "RadianceCache.hlsl"

RWTexture2D<float4> accumTexture : register(u0);
RWTexture2D<float4> renderBuffer : register(u1);

RWStructuredBuffer<RadianceCacheInput> cachePredInputBuffer : register(u2);
RWStructuredBuffer<RadianceCacheOutput> cachePredOutputBuffer : register(u3);
RWStructuredBuffer<RadianceCacheInput> cacheTrainInputBuffer : register(u4);
RWStructuredBuffer<RadianceCacheOutput> cacheTrainOutputBuffer : register(u5);
RWBuffer<uint> sampleCounters : register(u6);
RWStructuredBuffer<RenderState> cacheRenderState : register(u7);

[numthreads(8, 8, 1)]
void CSMain(uint3 threadId : SV_DispatchThreadID)
{ 
    if(all(threadId.xy < kViewportRes))
    {
        uint2 p = threadId.xy / 1;
        
        accumTexture[threadId.xy] *= kAccumMotionBlur;
        accumTexture[threadId.xy] += float4(renderBuffer[threadId.xy].xyz, 1);
        //if (threadId.x < int(kViewportRes.x * (0.5 + 0.25 * sin(kTime))))
        if(threadId.x < kSplitScreenPartitionX)
        {            
            accumTexture[threadId.xy] += float4(cachePredOutputBuffer[p.y * kViewportRes.x + p.x].radiance * cacheRenderState[p.y * kViewportRes.x + p.x].weight, 0);
            //outputTexture[threadId.xy].xyz = cacheRenderState[p.y * kViewportRes.x + p.x].weight;

        }

        //outputTexture[threadId.xy].xyz *= pow(cachePredInputBuffer[p.y * kViewportRes.x + p.x].position.x, 2.);
        //outputTexture[threadId.xy] = float4(NormSphericalToNormCartesian(cachePredInputBuffer[p.y * kViewportRes.x + p.x].normal) * 0.5 + 0.5, 1);
        //outputTexture[threadId.xy] = float4(NormSphericalToNormCartesian(cachePredInputBuffer[p.y * kViewportRes.x + p.x].viewDir) * 0.5 + 0.5, 1);        
        //outputTexture[threadId.xy] = float4(cachePredInputBuffer[p.y * kViewportRes.x + p.x].diffuseAlbedo, 1);
        //outputTexture[threadId.xy] = float4(asfloat3(cachePredInputBuffer[p.y * kViewportRes.x + p.x].roughness), 1);
        
        //outputTexture[threadId.xy].xyz = cacheRenderState[p.y * kViewportRes.x + p.x].weight;

        //if(p.y * kViewportRes.x + p.x < kMaxTrainSamples)
        {
        //    outputTexture[threadId.xy] = float4(cacheTrainOutputBuffer[p.y * kViewportRes.x + p.x].radiance, 1);    
            
            //outputTexture[threadId.xy] = float4(NormSphericalToNormCartesian(cacheTrainInputBuffer[p.y * kViewportRes.x + p.x].position), 1);    
            //outputTexture[threadId.xy] = float4(NormSphericalToNormCartesian(cacheTrainInputBuffer[p.y * kViewportRes.x + p.x].normal) * 0.5 + 0.5, 1);    
            //outputTexture[threadId.xy] = float4(NormSphericalToNormCartesian(cacheTrainInputBuffer[p.y * kViewportRes.x + p.x].viewDir) * 0.5 + 0.5, 1);        
            //outputTexture[threadId.xy] = float4(cacheTrainInputBuffer[p.y * kViewportRes.x + p.x].diffuseAlbedo, 1);
            //outputTexture[threadId.xy] = float4(asfloat3(cacheTrainInputBuffer[p.y * kViewportRes.x + p.x].roughness), 1);    
        }
    }
}
