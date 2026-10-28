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

#ifndef SOURCE_TRANSFORM
#define SOURCE_TRANSFORM

#include "Math.hlsl"

struct Transform
{
    float3 trans;
    float3x3 rot;
    float sca;
};

float3x3 Identity()
{
    return float3x3(float3(1.0, 0.0, 0.0), 
                    float3(0.0, 1.0, 0.0), 
                    float3(0.0, 0.0, 1.0));
}

float3x3 ScaleMat3(float scale)
{
    return float3x3(float3(1.0 / scale, 0.0, 0.0),
                    float3(0.0, 1.0 / scale, 0.0),
                    float3(0.0, 0.0, 1.0 / scale));
}

float3x3 RotXMat3(float theta)
{
    float cosTheta = cos(theta), sinTheta = sin(theta);
    return float3x3(float3(1.0, 0.0, 0.0),
                    float3(0.0, cosTheta, -sinTheta),
                    float3(0.0, sinTheta, cosTheta));
}

float3x3 RotYMat3(float theta)
{
    float cosTheta = cos(theta), sinTheta = sin(theta);
    return float3x3(float3(cosTheta, 0.0, sinTheta),
                    float3(0.0, 1.0, 0.0),
                    float3(-sinTheta, 0.0, cosTheta));
}

#define kRotXMat3_HalfPi    float3x3(1, 0, 0, 0, 0, -1, 0, 1, 0)
#define kRotXMat3_NegHalfPi float3x3(1, 0, 0, 0, 0, 1, 0, -1, 0)
#define kRotYMat3_HalfPi    float3x3(0, 0, 1, 0, 1, 0, -1, 0, 0)
#define kRotYMat3_NegHalfPi float3x3(0, 0, 1, 0, 1, 0, -1, 0, 0)
#define kRotZMat3_HalfPi    float3x3(0, 0, 1, 0, 1, 0, -1, 0, 0)
#define kRotZMat3_NegHalfPi float3x3(0, 0, -1, 0, 1, 0, 1, 0, 0)

float3x3 RotZMat3(float theta)
{
    float cosTheta = cos(theta), sinTheta = sin(theta);
    return float3x3(float3(cosTheta, -sinTheta, 0.0),
                    float3(sinTheta, cosTheta, 0.0),
                    float3(0.0, 0.0, 1.0));
}

float2x2 RotMat2(float theta)
{
    float cosTheta = cos(theta), sinTheta = sin(theta);
    return float2x2(sinTheta, cosTheta, -cosTheta, sinTheta);
}

Transform CompoundTransform(float3 trans, float3 rot, float scale)
{
    Transform t;
    t.rot = Identity();
    t.sca = scale;
    t.trans = trans;

    if (rot.x != 0.0) { t.rot = mul(t.rot, RotXMat3(rot.x)); }
    if (rot.y != 0.0) { t.rot = mul(t.rot, RotYMat3(rot.y)); }
    if (rot.z != 0.0) { t.rot = mul(t.rot, RotZMat3(rot.z)); }
    
    return t;
}

Transform CompoundTransform(float3 trans, float3x3 rot, float scale)
{
    Transform t;
    t.rot = rot;
    t.sca = scale;
    t.trans = trans;    
    return t;
}

Transform NonRotatingTransform(float3 trans, float scale)
{
    Transform t;
    t.rot = Identity();
    t.sca = scale;
    t.trans = trans;
    return t;
}

Transform IdentityTransform()
{
    Transform t;
    t.rot = Identity();
    t.sca = 1.0;
    t.trans = kZero;
    return t;
}

float2 ScreenToNormalisedScreen(float2 p, float2 res)
{   
    return (p - res * 0.5) / float(res.y); 
}

// Fast construction of orthonormal basis using quarternions to avoid expensive normalisation and branching 
// From Duf et al's technical report https://graphics.pixar.com/library/OrthonormalB/paper.pdf, inspired by
// Frisvad's original paper: http://orbit.dtu.dk/files/126824972/onb_frisvad_jgt2012_v2.pdf
float3x3 CreateBasis(float3 n)
{
    float s = Sign(n.z);
    float a = -1.0 / (s + n.z);
    float b = n.x * n.y * a;
    
    return transpose(float3x3(float3(1.0f + s * n.x * n.x * a, s * b, -s * n.x),
                float3(b, s + n.y * n.y * a, -n.y),
                n));
}

float3x3 CreateBasis(float3 n, float3 up)
{
    float3 tangent = normalize(cross(n, up));
    float3 cotangent = cross(tangent, n);

    return transpose(float3x3(tangent, cotangent, n));
}

#endif
