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
#include "Bindings.hlsli"
// GROUP_THREAD_COUNT_X
// GROUP_THREAD_COUNT_Y
// GROUP_THREAD_COUNT_Z
#include "DenoisingData.h"
// rr::DispatchInput
#include "Dispatch.hlsli"
// rr::PackedCamera
// rr::PackedFrame
#include "PackedData.h"
// rr::NdcSpaceToViewSpace
// rr::NdcSpaceToWorldSpace
// rr::ScreenSpaceToNdcSpace
#include "Transformation.hlsli"
// rr::F32
// rr::F32x2
// rr::F32x3
// rr::F32x4
// rr::U32x2
#include "Types.h"

//------------------------------------------------------------------------------
// Sample Bindings
//------------------------------------------------------------------------------

DECLARE_CBV_BINDING(0, rr::PackedFrame, g_frame);

DECLARE_SRV_BINDING(0, Texture2D< rr::F32 >,   g_depth);

DECLARE_UAV_BINDING(0, RWTexture2D< rr::F32 >,   g_linearDepth);

DECLARE_UAV_BINDING(1, RWTexture2D< rr::F32x4 >, g_denoisedDirectSpecular);
DECLARE_UAV_BINDING(2, RWTexture2D< rr::F32x4 >, g_denoisedDirectDiffuse);
DECLARE_UAV_BINDING(3, RWTexture2D< rr::F32x4 >, g_denoisedIndirectSpecular);
DECLARE_UAV_BINDING(4, RWTexture2D< rr::F32x4 >, g_denoisedIndirectDiffuse);
DECLARE_UAV_BINDING(5, RWTexture2D< rr::F32 >,   g_denoisedDominantLightVisibility);
DECLARE_UAV_BINDING(6, RWTexture2D< rr::F32 >,   g_denoisedAmbientOcclusion);
DECLARE_UAV_BINDING(7, RWTexture2D< rr::F32 >,   g_denoisedSpecularOcclusion);

#define ROOT_SIGNATURE                                                         \
    "RootFlags(0),"                                                            \
    /* ---- CBVs ---- */                                                       \
    "CBV(b0, visibility = SHADER_VISIBILITY_ALL)," /* g_frame */               \
    /* ---- SRVs ---- */                                                       \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        "SRV(t0)" /* g_depth */                                                \
    "),"                                                                       \
    /* ---- UAVs ---- */                                                       \
    "DescriptorTable("                                                         \
        "visibility = SHADER_VISIBILITY_ALL,"                                  \
        "UAV(u0)," /* g_linearDepth */                                         \
        "UAV(u1)," /* g_denoisedDirectSpecular */                              \
        "UAV(u2)," /* g_denoisedDirectDiffuse */                               \
        "UAV(u3)," /* g_denoisedIndirectSpecular */                            \
        "UAV(u4)," /* g_denoisedIndirectDiffuse */                             \
        "UAV(u5)," /* g_denoisedDominantLightVisibility */                     \
        "UAV(u6)," /* g_denoisedAmbientOcclusion */                            \
        "UAV(u7)"  /* g_denoisedSpecularOcclusion */                           \
    ")"

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    void Main(DispatchInput input)
    {
        const U32x2 pixel = input.dispatchThreadId.xy;
        if (any(g_frame.renderSize <= pixel))
        {
            return;
        }

        const F32 depth   = g_depth[pixel];
        const F32x2 uv    = (F32x2(pixel) + 0.5f) * g_frame.invRenderSize;
        const F32x2 pNdc  = ScreenSpaceToNdcSpace(uv);
        const F32x3 pView = NdcSpaceToViewSpace(F32x3(pNdc, depth), g_frame.camera.clipToView);

        // Prepare the denoiser's inputs.
        // Signed linear depth
        g_linearDepth[pixel]                     = pView.z;

        g_denoisedDirectSpecular[pixel]          = 0.0f;
        g_denoisedDirectDiffuse[pixel]           = 0.0f;
        g_denoisedIndirectSpecular[pixel]        = 0.0f;
        g_denoisedIndirectDiffuse[pixel]         = 0.0f;
        g_denoisedDominantLightVisibility[pixel] = 0.0f;
        g_denoisedAmbientOcclusion[pixel]        = 0.0f;
        g_denoisedSpecularOcclusion[pixel]       = 0.0f;
    }
}

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_THREAD_COUNT_X, GROUP_THREAD_COUNT_Y, GROUP_THREAD_COUNT_Z)]
void Main(rr::DispatchInput input)
{
    rr::Main(input);
}
