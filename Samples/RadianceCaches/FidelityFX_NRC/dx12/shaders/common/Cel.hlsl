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

#ifndef SOURCE_CEL
#define SOURCE_CEL

#include "Math.hlsl"
#include "Half.hlsl"

void Tap(in int2 xyScreen, Texture2D<float4> data, out float3 lda, out float3 n,  out float3 L)
{
    float4 texel = data.Load(int3(xyScreen, 0));  
        
    n.xy = UnpackFloat2(texel.x);
    UnpackFloat2(texel.y, n.z, L.x);
    L.yz = UnpackFloat2(texel.z);
    lda = float3(luminance(L), UnpackFloat2(texel.w));
}

float3 Cel(in int2 xyScreen, Texture2D<float4> data)
{   
    //return data.Load(int3(xyScreen, 0)).xyz;
    
    float3 n0, lda0, L0;
    Tap(xyScreen, data, lda0, n0, L0);

    float3 sumL = kZero;
    float sumWeights = 0.0;
    int x = int(xyScreen.x), y = int(xyScreen.y);
    
    #define kInnerRadius 1
    float3 innerFaceL = kZero;
    float3 innerEdgeL = kZero;
    for(int v = -kInnerRadius; v <= kInnerRadius; ++v)
    {
        for(int u = -kInnerRadius; u <= kInnerRadius; ++u)
        {
            if(x + u < 0 || x + u >= int(kViewportRes.x) || y + v < 0 || y + v >= int(kViewportRes.y)) { continue; }           
            
            float3 nk, ldak, Lk;
            Tap(int2(x + u, y + v), data, ldak, nk, Lk);  
            
            float kernelWeight = 1.0 - sqr(length(float2(u, v)) / 3.0);
            if(kernelWeight <= 0.0) { continue; }
            
            #define kNormGain (.3 * float(kInnerRadius))
            #define kNormBias 1.
            float deltaN = dot(normalize(nk), normalize(n0));
            float normWeight = ((deltaN * kNormBias) - (1.0 - kNormGain)) / kNormGain;

            #define kDepthBias (20. * float(kInnerRadius))
            float deltaD = abs(log(1.0 + ldak.y) - log(1.0 + lda0.y));
            float depthWeight = 1.0 - deltaD * float(kDepthBias);

            float weight = saturate(depthWeight) * saturate(normWeight);   
            
            float tone = pow(ldak.x, 2.);
            innerFaceL += kOne * tone * kernelWeight;
            innerEdgeL += kOne * ErfApprox((weight - 0.5) * 4.) * kernelWeight;
            sumWeights += kernelWeight;
        }
    }    
    innerFaceL /= sumWeights;    
    innerEdgeL /= sumWeights;

     float3 outerEdgeL = kZero;
    sumWeights = 0.0;
    #define kOuterRadius 5
    for(int v = -kOuterRadius; v <= kOuterRadius; ++v)
    {
        for(int u = -kOuterRadius; u <= kOuterRadius; ++u)
        {
            if(x + u < 0 || x + u >= int(kViewportRes.x) || y + v < 0 || y + v >= int(kViewportRes.y)) { continue; }           
            
            float3 nk, ldak, Lk;
            Tap(int2(x + u, y + v), data, ldak, nk, Lk);   
            
            float kernelWeight = 1.0- sqr(length(float2(u, v)) / float(sqr(kOuterRadius + 1)));
            if(kernelWeight <= 0.0) { continue; }                     
            
            float deltaD = max(0., abs(log(1.0 + ldak.y) - log(1.0 + lda0.y)) - 0.05);
            float depthWeight = 1.0 - deltaD * float(kDepthBias);
            
            float weight = saturate(depthWeight);
            
            outerEdgeL += weight * kernelWeight;
            sumWeights += kernelWeight;
        }
    }     
    outerEdgeL /= sumWeights;
    outerEdgeL = 1. - (2. * (1. - outerEdgeL));
    outerEdgeL = tanh(5. * mix(-kOne, kOne, saturate(outerEdgeL)));
    
    
    #define kCelBias 0.05
    #define kCelGain 1.
    #define kCelEmitterGain 50.
    
    return asfloat3(lda0.z * kCelGain * max((1. - saturate(innerEdgeL.x)) * lda0.z, (1. - saturate(outerEdgeL.x)))) * L0 + lda0.z * kCelBias;
    //return float3(innerFaceL.x, 
    //           1. - saturate(innerEdgeL.x) * saturate(outerEdgeL.x), 
    //            lda0.z * exp(-sqr(0.3 * lda0.y)));
}

#endif
