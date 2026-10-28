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

#ifndef SOURCE_CAMERAS
#define SOURCE_CAMERAS

#include "Ray.hlsl"
#include "Math.hlsl"

Ray InitCameraRay()
{
    Ray ray;
    ray.tNear = kFltMax;
    ray.weight = kOne;
    ray.compoundWeight = kOne;
    ray.depth = 0;
    ray.flags = kFlagsCausticPath;
    ray.pdf = 1e3;
    
    return ray;
}

Ray CreatePinholeCameraRay(float2 uvScreen, float3 cameraPos, float3 cameraLookAt, float fov)
{       
    Ray ray = InitCameraRay();
    ray.od.o = cameraPos;
    ray.od.d = mul(CreateBasis(normalize(cameraPos - cameraLookAt), float3(0., 1., 0.)), normalize(float3(uvScreen, -1. / tan(toRad(fov)))));
    
    return ray;  
}

Ray CreateOrthographicCameraRay(float2 uv, float2 sensorSize, float3 cameraPos, float3 cameraLookAt)
{    
    #define kOrthoCamLookUp float3(0, 1, 0)
    
    Ray ray = InitCameraRay();
    ray.od.d = normalize(cameraLookAt - cameraPos);
    ray.od.o = cameraPos + mul(CreateBasis(ray.od.d, kOrthoCamLookUp), float3(uv * sensorSize, 0.0));
    
    return ray;
}

#endif
