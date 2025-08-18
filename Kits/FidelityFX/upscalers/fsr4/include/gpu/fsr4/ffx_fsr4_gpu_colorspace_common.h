// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
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

#define FFX_MLSR_COLORSPACE_LINEAR     0
#define FFX_MLSR_COLORSPACE_NON_LINEAR 1
#define FFX_MLSR_COLORSPACE_SRGB       2
#define FFX_MLSR_COLORSPACE_PQ         3

float3 ApplySRGB(float3 x)
{
#if __HLSL_VERSION >= 2021
    // Approximately pow(x, 1.0 / 2.2)
    return select(x < 0.0031308, 12.92 * x, 1.055 * pow(x, 1.0 / 2.4) - 0.055);
#else
    return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
#endif
}

float3 RemoveSRGB(float3 x)
{
#if __HLSL_VERSION >= 2021
    // Approximately pow(x, 2.2)
    return select(x < 0.04045, x / 12.92, pow((x + 0.055) / 1.055, 2.4));
#else
    return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
#endif
}

float3 ApplyPQ(float3 fRgb)
{
    float paper_white = 300.0;
    float nit_max = 10000.0;
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;

    float scale = paper_white / nit_max;

    float3 Lp = pow(max(fRgb * scale, 1e-10f), m1);
    float3 fRgb_tonemapped = pow((c1 + c2 * Lp) / (1 + c3 * Lp), m2);
    //no NaN management for now!!!

    return max(fRgb_tonemapped, 0);
}

float3 RemovePQ(float3 fRgb)
{
    float paper_white = 300.0;
    float nit_max = 10000.0;
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;

    float3 Np = pow(fRgb, 1 / m2);
    float3 fRgb_linear = pow(max(Np - c1, 0) / (c2 - c3 * Np), 1 / m1);

    fRgb_linear *= nit_max / paper_white;

    fRgb_linear = max(fRgb_linear, 0);
    return fRgb_linear;
}

float3 ApplyMuLaw(float3 fRgb)
{
#if FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_NON_LINEAR
    return fRgb;
#else
# if FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_SRGB
        fRgb = RemoveSRGB(fRgb);
# elif FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_PQ
        fRgb = RemovePQ(fRgb);
# endif
    float3 fRgb_tonemapped = 0.1174f * log(1.f + 150.f * fRgb);

    return max(fRgb_tonemapped, 0);
 #endif
}

float3 RemoveMuLaw(float3 fRgb)
{
#if FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_NON_LINEAR
    return fRgb;
#else
    
    float3 fRgb_out = (exp(8.51788f * fRgb) - 1.f) / 150.f;
    fRgb_out = max(fRgb_out, 0);
    
#if FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_SRGB
    fRgb_out = ApplySRGB(fRgb_out);
#elif FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_PQ
    fRgb_out = ApplyPQ(fRgb_out);
#endif
    return fRgb_out;
#endif
}
