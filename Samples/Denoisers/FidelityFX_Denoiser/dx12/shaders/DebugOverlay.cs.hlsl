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

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

// DECLARE_CBV_BINDING
// DECLARE_SRV_BINDING
// DECLARE_UAV_BINDING
// DECLARE_SAMPLER_BINDING
#include "Bindings.hlsli"
// GROUP_THREAD_COUNT_X
// GROUP_THREAD_COUNT_Y
// GROUP_THREAD_COUNT_Z
// rr::DebugOverlayConstants
#include "DebugOverlayData.h"
// rr::DispatchInput
#include "Dispatch.hlsli"
// rr::F32x2
// rr::F32x4
// rr::U32x2
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Bindings
//------------------------------------------------------------------------------

DECLARE_CBV_BINDING(0, rr::DebugOverlayConstants, g_constants);

DECLARE_SRV_BINDING(0, Texture2D< rr::F32x4 >, g_debugView);

DECLARE_UAV_BINDING(0, RWTexture2D< rr::F32x4 >, g_output);

DECLARE_SAMPLER_BINDING(0, SamplerState, g_linearSampler);

#define ROOT_SIGNATURE                                                          \
    "RootFlags(0),"                                                             \
    /* ---- CBVs ---- */                                                        \
    "CBV(b0, visibility = SHADER_VISIBILITY_ALL)," /* g_constants */            \
    /* ---- SRVs ---- */                                                        \
    "DescriptorTable("                                                          \
        "visibility = SHADER_VISIBILITY_ALL,"                                   \
        "SRV(t0)"  /* g_debugView */                                            \
    "),"                                                                        \
    /* ---- UAVs ---- */                                                        \
    "DescriptorTable("                                                          \
        "visibility = SHADER_VISIBILITY_ALL,"                                   \
        "UAV(u0)"  /* g_output */                                               \
    "),"                                                                        \
    /* ---- Static samplers ---- */                                             \
    "StaticSampler(s0,"                                                         \
        "filter        = FILTER_MIN_MAG_LINEAR_MIP_POINT,"                      \
        "addressU      = TEXTURE_ADDRESS_CLAMP,"                                \
        "addressV      = TEXTURE_ADDRESS_CLAMP,"                                \
        "addressW      = TEXTURE_ADDRESS_CLAMP,"                                \
        "visibility    = SHADER_VISIBILITY_ALL)"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    void Main(DispatchInput input)
    {
        const U32x2 pixel = input.dispatchThreadId.xy;
        if (any(g_constants.outputTargetSize <= F32x2(pixel)))
        {
            return;
        }

        const F32x2 uv         = (F32x2(pixel) + 0.5f) / g_constants.outputTargetSize;
        const F32x4 foreground = g_debugView.SampleLevel(g_linearSampler, uv, 0.0f);
        const F32x4 background = g_output[pixel];

        g_output[pixel] = F32x4(lerp(background.xyz, foreground.rgb, foreground.a), background.w);
    }
}

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_THREAD_COUNT_X, GROUP_THREAD_COUNT_Y, GROUP_THREAD_COUNT_Z)]
void Main(rr::DispatchInput input)
{
    rr::Main(input);
}
