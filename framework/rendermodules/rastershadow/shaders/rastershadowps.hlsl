// Portions Copyright 2023 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This shader code was ported from https://github.com/KhronosGroup/glTF-WebGL-PBR
// All credits should go to his original author.

#include "surfacerendercommon.h"

//////////////////////////////////////////////////////////////////////////
// Resources
//////////////////////////////////////////////////////////////////////////

Texture2D    AllTextures[] : register(t0);
SamplerState AllSamplers[] : register(s0);

cbuffer CBSceneInformation : register(b0)
{
    SceneInformation SceneInfo;
};
cbuffer CBInstanceInformation : register(b1)
{
    InstanceInformation InstanceInfo;
};
cbuffer CBTextureIndices : register(b2)
{
    TextureIndices Textures;
}

void DiscardPixelIfAlphaCutOff(VS_SURFACE_OUTPUT Input)
{
#if defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
    float4 BaseColorAlpha = GetBaseColorAlpha(Input, InstanceInfo.MaterialInfo, Textures, AllTextures, AllSamplers, SceneInfo.MipLODBias);
    if (BaseColorAlpha.a < DEF_alphaCutoff)
        discard;
#endif
}

void MainPS(VS_SURFACE_OUTPUT SurfaceInput)
{
    DiscardPixelIfAlphaCutOff(SurfaceInput);
}
