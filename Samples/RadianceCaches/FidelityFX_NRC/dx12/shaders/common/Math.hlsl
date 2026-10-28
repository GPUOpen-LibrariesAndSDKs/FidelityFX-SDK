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

#ifndef SOURCE_MATH
#define SOURCE_MATH

// *******************************************************************************************************
//    Math functions
// *******************************************************************************************************

#define kPi                    3.14159265359
#define kInvPi                 (1.0 / 3.14159265359)
#define kTwoPi                 (2.0 * kPi)
#define kFourPi                (4.0 * kPi)
#define kHalfPi                (0.5 * kPi)
#define kRootPi                1.77245385091
#define kRoot2                 1.41421356237
#define kLog10                 2.30258509299
#define kFltMax                3.402823466e+38
#define kLog2                  0.6931471805
#define kOneThird              (1.0 / 3.0)
#define kIntMax                0x7fffffff
#define kOne                   float3(1, 1, 1)
#define kZero                  float3(0, 0, 0)
#define kRed                   float3(1., 0., 0.)
#define kYellow                float3(1., 1., 0.)
#define kGreen                 float3(0., 1., 0.)
#define kBlue                  float3(0., 0., 1.)
#define kPink                  float3(1., 0., 0.2) 

#define Timecode               float3
#define float5                 float[5]

#define mix(a, b, t)            lerp(a, b, t)
float cubrt(float a)           { return sign(a) * pow(abs(a), 1.0 / 3.0); }
float toRad(float deg)         { return kTwoPi * deg / 360.0; }
float toDeg(float rad)         { return 360.0 * rad / kTwoPi; }
float sqr(float a)             { return a * a; }
float2 sqr(float2 a)           { return a * a; }
float3 sqr(float3 a)           { return a * a; }
float4 sqr(float4 a)           { return a * a; }
int sqr(int a)                 { return a * a; }
int cub(int a)                 { return a * a * a; }
float cub(float a)             { return a * a * a; }
float pow4(float a)            { a *= a; return a * a; }
int mod2(int a, int b)         { return ((a % b) + b) % b; }
float mod2(float a, float b)   { return fmod(fmod(a, b) + b, b); }
float3 mod2(float3 a, float3 b) { return fmod(fmod(a, b) + b, b); }
float length2(float2 v)          { return dot(v, v); }
float length2(float3 v)          { return dot(v, v); }
int sum(int2 a)               { return a.x + a.y; }
float luminance(float3 v)        { return v.x * 0.17691 + v.y * 0.8124 + v.z * 0.01063; }
float mean(float3 v)             { return v.x / 3.0 + v.y / 3.0 + v.z / 3.0; }
float sin01(float a)           { return 0.5 * sin(a) + 0.5; }
float cos01(float a)           { return 0.5 * cos(a) + 0.5; }
float saturate(float a)        { return clamp(a, 0.0, 1.0); }
float2 saturate(float2 a)          { return clamp(a, 0.0, 1.0); }
float3 saturate(float3 a)          { return clamp(a, 0.0, 1.0); }
float4 saturate(float4 a)          { return clamp(a, 0.0, 1.0); }
float maxf3(float3 v)         { return (v.x > v.y) ? ((v.x > v.z) ? v.x : v.z) : ((v.y > v.z) ? v.y : v.z); }
float maxf2(float2 v)         { return (v.x > v.y) ? v.x : v.y; }
int maxi2(int2 v)          { return (v.x > v.y) ? v.x : v.y; }
float minf3(float3 v)         { return (v.x < v.y) ? ((v.x < v.z) ? v.x : v.z) : ((v.y < v.z) ? v.y : v.z); }
int mini3(int3 v)          { return (v.x < v.y) ? ((v.x < v.z) ? v.x : v.z) : ((v.y < v.z) ? v.y : v.z); }
float minf2(float2 v)         { return (v.x < v.y) ? v.x : v.y; }
int maxDim(float3 v)             { return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : ((v.y > v.z) ? 1 : 2); }
int minDim(float3 v)             { return (v.x < v.y) ? ((v.x < v.z) ? 0 : 2) : ((v.y < v.z) ? 1 : 2); }
float sum(float2 v)              { return v.x + v.y; }
float sum(float3 v)              { return v.x + v.y + v.z; }
float sum(float4 v)              { return v.x + v.y + v.z + v.w; }
float max3(float a, float b, float c) { return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c); }
float min3(float a, float b, float c) { return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c); }
void sort(inout float a, inout float b) { if(a > b) { float s = a; a = b; b = s; } }
void sort(inout float2 v) { if(v.x > v.y) { float s = v.x; v.x = v.y; v.y = s; } }
float2 sorted(in float2 v) { return (v.x < v.y) ? v : float2(v.y, v.x); }
void swap(inout float a, inout float b) { float s = a; a = b; b = s; }
void swap(inout int a, inout int b) { int s = a; a = b; b = s; }
float atanh(float x) { return 0.5 * log((1 + x) / (1 - x)); }
float3 atanh(float3 x) { return 0.5 * log((1 + x) / (1 - x)); }
float2 asfloat2(float f) { return f; }
float3 asfloat3(float f) { return f; }
float4 asfloat4(float f) { return f; }

float Smoothstep(float t) { return t * t * (3.0 - 2.0 * t); }
float Smoothstep(float a, float b, float t) { return mix(a, b, t * t * (3.0 - 2.0 * t)); }
float Smootherstep(float t) { return t * t * t * (t * (6. * t - 15.) + 10.); }
float Smootherstep(float a, float b, float t) { return mix(a, b, t * t * t * (t * (6. * t - 15.) + 10.)); }

// Transposed matrix multiplication. Makes porting from GLSL easier.
#define mulT(a, b) mul(b, a)

float saw(float a)             
{ 
    a = fmod(a / kPi, 2.);
    return (1. - (2. * abs(frac(a) - 0.5))) * -(floor(a) * 2. - 1.);
}

float cosaw(float a) { return saw(a + kHalfPi); }

float saw01(float a) { return saw(a) * 0.5 + 0.5; }
float cosaw01(float a) { return saw(a + kHalfPi) * 0.5 + 0.5; }

float3 safeAtan(float3 a, float3 b)
{
    float3 r;
    #define kAtanEpsilon 1e-10
    r.x = (abs(a.x) < kAtanEpsilon && abs(b.x) < kAtanEpsilon) ? 0.0 : atan2(a.x, b.x); 
    r.y = (abs(a.y) < kAtanEpsilon && abs(b.y) < kAtanEpsilon) ? 0.0 : atan2(a.y, b.y); 
    r.z = (abs(a.z) < kAtanEpsilon && abs(b.z) < kAtanEpsilon) ? 0.0 : atan2(a.z, b.z); 
    return r;
}

float3 SafeNormalize(float3 v, float3 n)
{
    float len = length(v);
    return (len > 1e-10) ? (v / len) : n;
}

float2 SafeNormalize(float2 v) { return v / (1e-10 + length(v)); }
float3 SafeNormalize(float3 v) { return v / (1e-10 + length(v)); }
float4 SafeNormalize(float4 v) { return v / (1e-10 + length(v)); }

float3 SafeNormaliseTexel(float4 t)
{
    return t.xyz / max(1e-15, t.w);
}

float4 Sign(float4 v)
{
    return step(0.0, v) * 2.0 - 1.0;
}

float Sign(float v)
{
    return step(0.0, v) * 2.0 - 1.0;
}

bool IsNan( float val )
{
    return ( val < 0.0 || 0.0 < val || val == 0.0 ) ? false : true;
}

bool3 IsNan( float3 val )
{
    return bool3( ( val.x < 0.0 || 0.0 < val.x || val.x == 0.0 ) ? false : true, 
                  ( val.y < 0.0 || 0.0 < val.y || val.y == 0.0 ) ? false : true, 
                  ( val.z < 0.0 || 0.0 < val.z || val.z == 0.0 ) ? false : true);
}

bool4 IsNan( float4 val )
{
    return bool4( ( val.x < 0.0 || 0.0 < val.x || val.x == 0.0 ) ? false : true, 
                  ( val.y < 0.0 || 0.0 < val.y || val.y == 0.0 ) ? false : true, 
                  ( val.z < 0.0 || 0.0 < val.z || val.z == 0.0 ) ? false : true,
                  ( val.w < 0.0 || 0.0 < val.w || val.w == 0.0 ) ? false : true);
}

float Sigmoid(float v) { return 1. / (1. + exp(-(v))); }

#define SignedGamma(v, gamma) (sign(v) * pow(abs(v), gamma))

bool QuadraticSolve(float a, float b, float c, out float t0, out float t1)
{
    float b2ac4 = b * b - 4.0 * a * c;
    if(b2ac4 < 0.0) 
    { 
        return false; 
    }
    else
    {
        float sqrtb2ac4 = sqrt(b2ac4);
        t0 = (-b + sqrtb2ac4) / (2.0 * a);
        t1 = (-b - sqrtb2ac4) / (2.0 * a);
        return true;
    }
}

// Closed-form approxiation of the error function.
// See 'Uniform Approximations for Transcendental Functions', Winitzki 2003, https://doi.org/10.1007/3-540-44839-X_82
float ErfApprox(float x)
{    
     float a = 8.0 * (kPi - 3.0) / (3.0 * kPi * (4.0 - kPi));
     return sign(x) * sqrt(1.0 - exp(-(x * x) * (4.0 / kPi + a * x * x) / (1.0 + a * x * x)));
}

float UintToFloat01(uint i)
{
    return float(i) / float(0xffffffffu);
}

float UintToFloat01(uint i, int  bits)
{
    return float(i & ((1u << bits) - 1u)) / float(((1u << bits) - 1u));
}

float Sigmoid(float x, float d)
{
    return 1. / (1. + exp(-((2.*x-1.) * d)));
}

int GetSplitScreenPartition()
{
    //return SPLIT_VIEW_RADIANCE_CACHE;
    //return SPLIT_VIEW_REFERENCE;
    return int(kViewportRes.x * (0.5 + 0.25 * sin(kTime)));
    //return p.x < kViewportRes.x / 2;
}

#endif // SOURCE_MATH

#ifndef SOURCE_DEBUG_VALUES
#define SOURCE_DEBUG_VALUES

static float3 gDebug;

#endif
