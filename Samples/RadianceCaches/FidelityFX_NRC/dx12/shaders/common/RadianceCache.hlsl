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

#ifndef SOURCE_RADIANCE_CACHE
#define SOURCE_RADIANCE_CACHE

#include "Math.hlsl"

#define kRadianceCacheBoundLower float3(-1, -0.5, -0.5)
#define kRadianceCacheBoundUpper float3(1., 0.5, 0.5)
#define kRadianceCacheBoundPadding 0.05
#define kRadianceCacheRaySpreadThreshold 0.03

#define INVALID_QUERY_VERTEX -1
#define INVALID_TRAINING_IDX 0xffffffffu

struct RadianceCacheInput
{
	float3 position;
	float2 normal;
	float2 viewDir;
	float3 diffuseAlbedo;
	float  roughness;
};

struct RadianceCacheOutput
{
	float3 radiance;
};

struct RenderState
{
	float3 weight;
};

struct PixelInfo
{
    float3 weight;
	uint slotIdx;
	float3 radiance;
	uint debugBounceCount;
};

float3 NormSphericalToNormCartesian(float2 s)
{
    s.x *= kPi;
    s.y = (s.y - 0.5) * kTwoPi;    
    float sinTheta = sin(s.x);
    
    return float3(cos(s.y) * sinTheta, sin(s.y) * sinTheta, cos(s.x));
}

float2 NormCartesianToNormSpherical(float3 c)
{
	float theta = acos(c.z) / kHalfPi;
    theta = (theta < 1) ? sqrt(theta) : 2 - sqrt(2 - theta);
    return float2(theta * 0.5, atan2(c.y, c.x) / kTwoPi + 0.5);
}

float3 NormalizeCachePosition(float3 p) 
{	
    return (((p - kRadianceCacheBoundLower) / (kRadianceCacheBoundUpper - kRadianceCacheBoundLower) - 0.5)) / (1 + kRadianceCacheBoundPadding) + 0.5;
}	

RadianceCacheInput ConstructRadianceCacheInput(float3 position, float3 normal, float3 viewDir, float3 albedo, float roughness)
{
	RadianceCacheInput i = { 
		NormalizeCachePosition(position),
		NormCartesianToNormSpherical(normal), 
		NormCartesianToNormSpherical(viewDir), 
		albedo,
		roughness
	};
	return i;
}

#endif
