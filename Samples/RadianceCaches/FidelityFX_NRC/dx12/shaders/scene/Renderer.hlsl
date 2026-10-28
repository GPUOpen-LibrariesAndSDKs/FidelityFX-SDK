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
#include "RadianceCache.hlsl"

RWTexture2D<float4> accumTexture : register(u0);
RWTexture2D<float4> renderBuffer : register(u1);

RWStructuredBuffer<RadianceCacheInput> cachePredInputBuffer : register(u2);
RWStructuredBuffer<RadianceCacheOutput> cachePredOutputBuffer : register(u3);
RWStructuredBuffer<RadianceCacheInput> cacheTrainInputBuffer : register(u4);
RWStructuredBuffer<RadianceCacheOutput> cacheTrainOutputBuffer : register(u5);
RWBuffer<uint> sampleCounters : register(u6);
RWStructuredBuffer<RenderState> cacheRenderState : register(u7);

#include "integrator/PT.hlsl"
#include "Camera.hlsl"
#include "tracable/Primitive.hlsl"
#include "tracable/Boolean.hlsl"
#include "tracable/KIFS.hlsl"
#include "Half.hlsl"
#include "color/Color.hlsl"
#include "texture/Simple.hlsl"

bool Trace(inout Ray ray, out HitCtx hit, RenderCtx renderCtx, PTState pt)
{   
    hit.matID = PT_INVALID_MATERIAL;
    
    //if(!Intersected(RayAABB(ray.od, -0.5, 0.5))) { return false; }
    
    Transform transform;  
    
            
    CSGCtx csg;
    InitCSGCtx(csg);
    
    transform = NonRotatingTransform(kZero, 1.);
    TestBoxCSG(csg, ray, transform, float3(2., 1., 1.), 1, CSG_ADD);    
        
    transform = NonRotatingTransform(float3(0.0, 0.05, 0.05), 1.);
    TestBoxCSG(csg, ray, transform, float3(1.9, 1., 1.), 0, CSG_SUBTRACT);
    
    transform = NonRotatingTransform(float3(0.0, 0.0, 0.0), 1.);
    TestBoxCSG(csg, ray, transform, float3(0.05, 1., 1.), 1, CSG_ADD);    
        
    transform = NonRotatingTransform(float3(0., 0., 0.), 0.2);
    TestSphereCSG(csg, ray, transform, CSG_SUBTRACT);
    
    if(ResolveCSGIntersection(csg, ray, hit) != -1)    
    //Transform transform = CompoundTransform(kZero, kRotXMat3_HalfPi, 1);
    //if(TestMaskedBoxPrimitive(ray, hit, transform, false, 31))
    {
        hit.albedo = kOne;
        hit.alpha = 0.1;
        
        if(hit.objID == 4)
        {
            hit.matID = PT_MAT_LAMBERT; 
            hit.albedo *= mix(0.7, 1., Checkerboard(hit.uv * 10));
            hit.albedo = mix(0., hit.albedo, Grid(hit.uv * 10, 0.05));
        }
        else
        {
            if(hit.objID < 2) 
            { 
                hit.albedo = (kRenderFlags & RENDER_ANIMATE_MATERIALS) ? Hue(renderCtx.time * 0.1 + hit.objID * 0.333) : kRed;
                            
                hit.matID = PT_MAT_ROUGH_SPECULAR;
                hit.alpha = 0.1;
            }
            else
            {
                hit.matID = PT_MAT_LAMBERT; 
                hit.albedo *= 0.5;            
            }            
        }
        //hit.matID = PT_MAT_PERFECT_SPECULAR; 
    }   

    //transform = NonRotatingTransform(float3(cos(renderCtx.time) * 0.3, 0.1 + abs(sin(renderCtx.time * 2.)) * 0.5 - 0.5, sin(-renderCtx.time) * 0.3), 0.1);
    const float t = (kRenderFlags & RENDER_ANIMATE_GEOMETRY) ? renderCtx.time : 0.0;
    
    transform = NonRotatingTransform(float3(sin(kTwoPi * t * 2. * 0.05) * 0.5, 0.1 + abs(cos(kTwoPi * t * 4. * 0.05)) * 0.3 - 0.4, 0.), 0.15);
    if(TestSpherePrimitive(ray, hit, transform, true))
    {
        hit.matID = PT_MAT_PERFECT_DIELECTRIC;
        hit.eta = 1.5;
        hit.albedo = kOne;
    }
    
    transform = NonRotatingTransform(float3(-0.5, -0.25, 0), 0.15);
    uint code = 0u;
    if(TestKIFSPrimitive(ray, hit, transform, renderCtx, code))
    {
        //hit.matID = PT_MAT_LAMBERT;
        hit.matID = PT_MAT_ROUGH_SPECULAR;
        hit.alpha = 0.05;
        //hit.albedo = 0.2;
        hit.albedo = mix(0., 1., UintToFloat01(HashOf(code * 17386, 0x87347u)));
        //hit.albedo = saturate(mix(kOne, Hue(mix(0., 1., UintToFloat01(HashOf(code * 17386, 0x87347u)))), .7));

    }    
    transform = CompoundTransform(float3(0.7, -0.1, 0), float3(t + kHalfPi, 0, 0), 0.35);
    if(TestConePrimitive(ray, hit, transform, 0.3, 1.))
    {
        hit.matID = PT_MAT_ROUGH_SPECULAR;
        hit.alpha = 0.1;
        hit.albedo = kOne;
        //hit.albedo = saturate(mix(kOne, Hue(mix(0., 1., UintToFloat01(HashOf(code * 17386, 0x87347u)))), .7));
    }

    if(TestPlanePrimitive(ray, hit, pt.emitterTrans, true, false))
    {
        hit.matID = PT_MAT_EMITTER; 
    }
    
    // Roughen hits along glossy specular paths to 
    if(kIndirectRoughening > 0 && renderCtx.isTrainingPath)
    {
        hit.alpha = pow(hit.alpha, mix(1., 0.1, sqr(kIndirectRoughening)));
    }

    // Make all materials diffuse Lambert
    //if(hit.matID != PT_INVALID_MATERIAL && hit.matID != PT_MAT_EMITTER)  hit.matID = PT_MAT_LAMBERT;  

    // Flip the normal for back-facing intersections
    if(IsBackfacing(ray)) { hit.n = -hit.n; }

    return hit.matID != PT_INVALID_MATERIAL;
}

float3 ClampRadiance(in float3 L, float threshold)
{    
    //#define kSplatClamp 10.
    const float LMax = maxf3(L);
    return (LMax > threshold) ? (L * threshold / LMax) : L;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 threadId : SV_DispatchThreadID)
{    
    if(all(threadId.xy < kViewportRes))
    {
        RenderCtx renderCtx;
        renderCtx.rng = InitRand(threadId.xy, kFrameIdx);
        renderCtx.pixelIdx = threadId.y * kViewportRes.x + threadId.x;
        renderCtx.finalGatherWeight = 0;
        renderCtx.time = kTime;
        const int splitViewMode = (threadId.x < kSplitScreenPartitionX);
//#if kDisableAnimation == true
//            renderCtx.time = 0.;
//#endif
        //renderCtx.renderMode = (threadId.x < kViewportRes.x / 2) ? PT_MODE_NAIVE : PT_MODE_NEE;
        //renderCtx.renderMode = PT_MODE_NAIVE;
        renderCtx.renderMode = PT_MODE_NEE;
        
        {
            int slotIdx = 0;
            InterlockedAdd(sampleCounters[0], 1, slotIdx);        
        }
        cacheRenderState[renderCtx.pixelIdx].weight = 0;

        //if(renderCtx.pixelIdx < kMaxTrainSamples)
        //    cacheTrainOutputBuffer[renderCtx.pixelIdx].radiance = 0.;
        
        // Determine whether this pixel should be a training path or not
        renderCtx.isTrainingPath = Rand4(renderCtx.rng, 0u).z < (kTrainingRatio / (PT_MAX_PATH_DEPTH - 2)); 
        
        float time = renderCtx.time * .2;
        //float time = 0.;
        //float theta = (0.5 + floor(time) + Smootherstep(min(1., frac(time) * 2.))) * kHalfPi;
        float theta = kPi * 0.5;
        float cameraFoV = 30.;
        float3 cameraLookAt = float3(0., -0.1, 0);
        if (kRenderFlags & RENDER_ANIMATE_CAMERA)
        {
            theta = kHalfPi + sin(renderCtx.time * .3) * 0.4;
            cameraFoV = mix(20., 40., sin01(renderCtx.time * 0.5));
            cameraLookAt.x = mix(-0.5, 0.5, sin01(renderCtx.time * 0.6));
        }       
        
        float3 cameraPos = float3(cos(theta), 0., sin(theta)) * 2.;

        float3 L = 0;
        HitCtx hit;
        PTState pt;
        int pathDepth = 0;
        int totalTrain = 0;
        float cameraVertexSpread = 0., pathSpread = 0.;
        int radianceCacheQueryVertex = INVALID_QUERY_VERTEX;

        int trainIdx[PT_MAX_PATH_DEPTH];
        for(int i = 0; i < PT_MAX_PATH_DEPTH; ++i) { trainIdx[i] = INVALID_TRAINING_IDX; }

        renderCtx.rng = InitRand(threadId.xy, (kRenderFlags & RENDER_LOCK_NOISE) ? 0 : kFrameIdx);

        float2 uvView = ScreenToNormalisedScreen(float2(threadId.xy) + Rand4(renderCtx.rng, 0u).xy, float2(kViewportRes));
        Ray ray = CreatePinholeCameraRay(uvView, cameraPos, cameraLookAt, cameraFoV);

        pt = InitPTState(ray);
            
        float3 lightLookAt = float3(0., -1, 0);
        if (kRenderFlags & RENDER_ANIMATE_LIGHTS)
        {
            lightLookAt.x = sin(renderCtx.time * 0.3) * 2.;
        }
            
        #define kQuadEmitterPos float3(0.5, 0.3, 0.)
        pt.emitterTrans.rot = transpose(CreateBasis(normalize(lightLookAt - kQuadEmitterPos)));
        pt.emitterTrans.trans = kQuadEmitterPos;
        pt.emitterTrans.sca = kQuadEmitterSca;

        #define kMaxIterations (PT_MAX_PATH_DEPTH * 2)
        int rayIdx;
        int c = 0;
        for(rayIdx = 0; rayIdx < kMaxIterations && GetNextRay(pt, ray); ++rayIdx)
        {                       
            //renderCtx.rng = InitRand(threadId.xy, 0);                
            pathDepth = max(pathDepth, ray.depth);
                
            if(Trace(ray, hit, renderCtx, pt))
            {                                                         
                // Radiance cache
                if(!IsNEESample(ray))
                {    
                    if(ray.depth == 0)
                    {
                        // For the camera hit vertex, compute the spread for the heuristic
                        cameraVertexSpread = sqr(ray.tNear) / (1e-10 + kFourPi * -dot(ray.od.d, hit.n));
                    }
                    else if(radianceCacheQueryVertex == INVALID_QUERY_VERTEX)
                    {
                        // Accumulate the path spread based on the previous scattering event. If the ray has been sufficiently scattered, enqueue a cache query
                        pathSpread += sqr(ray.tNear) / (1e-10 + ray.pdf * -dot(ray.od.d, hit.n));
                        if(ray.depth == PT_MAX_PATH_DEPTH - 1 || 
                           (sqrt(pathSpread) / (1e-10 + cameraVertexSpread * kRadianceCacheRaySpreadThreshold)) - 1 > float(HashOf(PointAt(ray))) / float(0xffffffffu))
                        {                               
                            cachePredInputBuffer[renderCtx.pixelIdx] = ConstructRadianceCacheInput(PointAt(ray), hit.n, -ray.od.d, hit.albedo, hit.alpha);
                            radianceCacheQueryVertex = ray.depth;    
                            if(splitViewMode == SPLIT_VIEW_RADIANCE_CACHE && !renderCtx.isTrainingPath) 
                            { 
                                pt.path.weight[ray.depth] = hit.albedo;
                                break;     
                            }
                        }
                    }
                }
                    
                // Invoke the integrator and spawn extant rays
                Integrate(ray, pt, hit, renderCtx, L);
  
                // If we have an indirect sample, create a training slot for it
                if (!IsNEESample(ray) && renderCtx.isTrainingPath && HasEnqueued(pt))
                {
                    uint slotIdx;
                    InterlockedAdd(sampleCounters[1], 1, slotIdx);

                    if(slotIdx < kMaxTrainSamples)
                    {
                        totalTrain++;
                        trainIdx[ray.depth] = slotIdx;
                        cacheTrainInputBuffer[slotIdx] = ConstructRadianceCacheInput(PointAt(ray), hit.n, -ray.od.d, hit.albedo, hit.alpha);
                    }
                    else
                    {
                        // If the buffer is over-full
                        InterlockedAdd(sampleCounters[1], -1, slotIdx);
                    }
                }
            }
            else
            {                    
                EscapeRay(ray, pt, L);
            }             
        }  
                
        // Reconstruct the extant radiance at each path vertex. For training paths, store the result in the output buffer.       
        float3 LSubPath = kZero;
        float3 weight = kOne;
        for(int i = pathDepth; i >= 0; --i)
        {
            LSubPath += pt.path.LExtant[i];    

            if(trainIdx[i] != INVALID_TRAINING_IDX)
            {
                cacheTrainOutputBuffer[trainIdx[i]].radiance = ClampRadiance(LSubPath, 10.);
            }
            
            LSubPath *= pt.path.weight[i];  
        }        
                
        if(splitViewMode == SPLIT_VIEW_REFERENCE)
        {
            weight = kOne;
        }
        else 
        {
            if(radianceCacheQueryVertex != INVALID_QUERY_VERTEX)
            {
                L = 0;
                for(int i = 0; i < radianceCacheQueryVertex; ++i)
                {
                    weight *= pt.path.weight[i];
                    L += pt.path.LExtant[i] * weight;
                }                  
                weight *= pt.path.weight[radianceCacheQueryVertex];
                //L += pt.path.LExtant[radianceCacheQueryVertex] * weight;
            }
            else weight *= 0.;  
        }      
        
        cacheRenderState[renderCtx.pixelIdx].weight = weight;       

        renderBuffer[threadId.xy] = float4(L, 1);      

    }
}
