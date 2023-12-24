// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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


#include "ffx_sssr_resources.h"

#if defined(FFX_GPU)
#include "ffx_core.h"
#endif // #if defined(FFX_GPU)

#if defined(FFX_GPU)
#ifndef FFX_PREFER_WAVE64
#define FFX_PREFER_WAVE64
#endif // #if defined(FFX_GPU)

#if defined(SSSR_BIND_CB_SSSR)
    layout (set = 0, binding = SSSR_BIND_CB_SSSR, std140) uniform cbSSSR_t
    {
        FfxFloat32Mat4  invViewProjection;
        FfxFloat32Mat4  projection;
        FfxFloat32Mat4  invProjection;
        FfxFloat32Mat4  viewMatrix;
        FfxFloat32Mat4  invView;
        FfxFloat32Mat4  prevViewProjection;
        FfxUInt32x2     renderSize;
        FfxFloat32x2    inverseRenderSize;
        FfxFloat32      normalsUnpackMul;
        FfxFloat32      normalsUnpackAdd;
        FfxUInt32       roughnessChannel;
        FfxBoolean      isRoughnessPerceptual;
        FfxFloat32      iblFactor;
        FfxFloat32      temporalStabilityFactor;
        FfxFloat32      depthBufferThickness;
        FfxFloat32      roughnessThreshold;
        FfxFloat32      varianceThreshold;
        FfxUInt32       frameIndex;
        FfxUInt32       maxTraversalIntersections;
        FfxUInt32       minTraversalOccupancy;
        FfxUInt32       mostDetailedMip;
        FfxUInt32       samplesPerQuad;
        FfxUInt32       temporalVarianceGuidedTracingEnabled;
    } cbSSSR;

FfxFloat32Mat4 InvViewProjection()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.invViewProjection;
#else
    return FfxFloat32Mat4(0.0f);
#endif
}

FfxFloat32Mat4 Projection()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.projection;
#else
    return FfxFloat32Mat4(0.0f);
#endif
}

FfxFloat32Mat4 InvProjection()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.invProjection;
#else
    return FfxFloat32Mat4(0.0f);
#endif
}

FfxFloat32Mat4 ViewMatrix()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.viewMatrix;
#else
    return FfxFloat32Mat4(0.0f);
#endif
}

FfxFloat32Mat4 InvView()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.invView;
#else
    return FfxFloat32Mat4(0.0f);
#endif
}

FfxFloat32Mat4 PrevViewProjection()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.prevViewProjection;
#else
    return FfxFloat32Mat4(0.0f);
#endif
}

FfxFloat32 NormalsUnpackMul()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.normalsUnpackMul;
#else
    return 0.0f;
#endif
}

FfxFloat32 NormalsUnpackAdd()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.normalsUnpackAdd;
#else
    return 0.0f;
#endif
}

FfxUInt32 RoughnessChannel()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.roughnessChannel;
#else
    return 0;
#endif
}

FfxBoolean IsRoughnessPerceptual()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.isRoughnessPerceptual;
#else
    return false;
#endif
}

FfxUInt32x2 RenderSize()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.renderSize;
#else
    return FfxUInt32x2(0);
#endif
}

FfxFloat32x2 InverseRenderSize()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.inverseRenderSize;
#else
    return FfxFloat32x2(0.0f);
#endif
}

FfxFloat32 IBLFactor()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.iblFactor;
#else
    return 0.0f;
#endif
}

FfxFloat32 TemporalStabilityFactor()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.temporalStabilityFactor;
#else
    return 0.0f;
#endif
}

FfxFloat32 DepthBufferThickness()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.depthBufferThickness;
#else
    return 0.0f;
#endif
}

FfxFloat32 RoughnessThreshold()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.roughnessThreshold;
#else
    return 0.0f;
#endif
}

FfxFloat32 VarianceThreshold()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.varianceThreshold;
#else
    return 0.0f;
#endif
}

FfxUInt32 FrameIndex()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.frameIndex;
#else
    return 0;
#endif
}

FfxUInt32 MaxTraversalIntersections()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.maxTraversalIntersections;
#else
    return 0;
#endif
}

FfxUInt32 MinTraversalOccupancy()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.minTraversalOccupancy;
#else
    return 0;
#endif
}

FfxUInt32 MostDetailedMip()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.mostDetailedMip;
#else
    return 0;
#endif
}

FfxUInt32 SamplesPerQuad()
{
#if defined SSSR_BIND_CB_SSSR
    return cbSSSR.samplesPerQuad;
#else
    return 0;
#endif
}

FfxBoolean TemporalVarianceGuidedTracingEnabled()
{
#if defined SSSR_BIND_CB_SSSR
    return FfxBoolean(cbSSSR.temporalVarianceGuidedTracingEnabled);
#else
    return false;
#endif
}

#endif // #if defined(SSSR_BIND_CB_SSSR)

layout (set = 0, binding = 1000) uniform sampler s_EnvironmentMapSampler;
layout (set = 0, binding = 1001) uniform sampler s_LinearSampler;

// SRVs
#if defined SSSR_BIND_SRV_INPUT_COLOR
    layout (set = 0, binding = SSSR_BIND_SRV_INPUT_COLOR)                   uniform texture2D r_input_color;
#endif
#if defined SSSR_BIND_SRV_INPUT_DEPTH
    layout (set = 0, binding = SSSR_BIND_SRV_INPUT_DEPTH)                   uniform texture2D r_input_depth;
#endif
#if defined SSSR_BIND_SRV_DEPTH_HIERARCHY
    layout (set = 0, binding = SSSR_BIND_SRV_DEPTH_HIERARCHY)               uniform texture2D r_depth_hierarchy;
#endif
#if defined SSSR_BIND_SRV_INPUT_NORMAL
    layout (set = 0, binding = SSSR_BIND_SRV_INPUT_NORMAL)                  uniform texture2D r_input_normal;
#endif
#if defined SSSR_BIND_SRV_INPUT_MATERIAL_PARAMETERS
    layout (set = 0, binding = SSSR_BIND_SRV_INPUT_MATERIAL_PARAMETERS)     uniform texture2D r_input_material_parameters;
#endif
#if defined SSSR_BIND_SRV_INPUT_ENVIRONMENT_MAP
    layout (set = 0, binding = SSSR_BIND_SRV_INPUT_ENVIRONMENT_MAP)         uniform textureCube r_input_environment_map;
#endif
#if defined SSSR_BIND_SRV_RADIANCE
    layout (set = 0, binding = SSSR_BIND_SRV_RADIANCE)                      uniform texture2D r_radiance;
#endif
#if defined SSSR_BIND_SRV_RADIANCE_HISTORY
    layout (set = 0, binding = SSSR_BIND_SRV_RADIANCE_HISTORY)              uniform texture2D r_radiance_history;
#endif
#if defined SSSR_BIND_SRV_VARIANCE
    layout (set = 0, binding = SSSR_BIND_SRV_VARIANCE)                      uniform texture2D r_variance;
#endif
#if defined SSSR_BIND_SRV_EXTRACTED_ROUGHNESS
    layout (set = 0, binding = SSSR_BIND_SRV_EXTRACTED_ROUGHNESS)           uniform texture2D r_extracted_roughness;
#endif
#if defined SSSR_BIND_SRV_SOBOL_BUFFER
    layout (set = 0, binding = SSSR_BIND_SRV_SOBOL_BUFFER)                  uniform utexture2D r_sobol_buffer;
#endif
#if defined SSSR_BIND_SRV_SCRAMBLING_TILE_BUFFER
    layout (set = 0, binding = SSSR_BIND_SRV_SCRAMBLING_TILE_BUFFER)        uniform utexture2D r_scrambling_tile_buffer;
#endif
#if defined SSSR_BIND_SRV_BLUE_NOISE_TEXTURE
    layout (set = 0, binding = SSSR_BIND_SRV_BLUE_NOISE_TEXTURE)            uniform texture2D r_blue_noise_texture;
#endif
#if defined SSSR_BIND_SRV_INPUT_BRDF_TEXTURE
    layout (set = 0, binding = SSSR_BIND_SRV_INPUT_BRDF_TEXTURE)            uniform texture2D r_input_brdf_texture;
#endif

// UAVs
#if defined SSSR_BIND_UAV_OUTPUT
        layout (set = 0, binding = SSSR_BIND_UAV_OUTPUT, rgba32f)           uniform image2D rw_output;
#endif
#if defined SSSR_BIND_UAV_RADIANCE
        layout (set = 0, binding = SSSR_BIND_UAV_RADIANCE, rgba32f)         uniform image2D rw_radiance;
#endif
#if defined SSSR_BIND_UAV_VARIANCE
        layout (set = 0, binding = SSSR_BIND_UAV_VARIANCE, r32f)            uniform image2D rw_variance;
#endif
#if defined SSSR_BIND_UAV_RAY_LIST
        layout (set = 0, binding = SSSR_BIND_UAV_RAY_LIST, std430)          buffer rw_ray_list_t
        {
            FfxUInt32 data[];
        } rw_ray_list; 
#endif
#if defined SSSR_BIND_UAV_DENOISER_TILE_LIST
        layout (set = 0, binding = SSSR_BIND_UAV_DENOISER_TILE_LIST, std430) buffer rw_denoiser_tile_list_t
        {
            FfxUInt32 data[];
        } rw_denoiser_tile_list;
#endif
#if defined SSSR_BIND_UAV_RAY_COUNTER
        layout (set = 0, binding = SSSR_BIND_UAV_RAY_COUNTER, std430) buffer rw_ray_counter_t
        {
            FfxUInt32 data[];
        } rw_ray_counter;
#endif
#if defined SSSR_BIND_UAV_INTERSECTION_PASS_INDIRECT_ARGS
        layout (set = 0, binding = SSSR_BIND_UAV_INTERSECTION_PASS_INDIRECT_ARGS, std430) buffer rw_intersection_pass_indirect_args_t
        {
            FfxUInt32 data[];
        } rw_intersection_pass_indirect_args; 
#endif
#if defined SSSR_BIND_UAV_EXTRACTED_ROUGHNESS
        layout (set = 0, binding = SSSR_BIND_UAV_EXTRACTED_ROUGHNESS, r32f) uniform image2D rw_extracted_roughness;
#endif
#if defined SSSR_BIND_UAV_BLUE_NOISE_TEXTURE
        layout (set = 0, binding = SSSR_BIND_UAV_BLUE_NOISE_TEXTURE, rg32f) uniform image2D rw_blue_noise_texture;
#endif
#if defined SSSR_BIND_UAV_DEPTH_HIERARCHY
        layout (set = 0, binding = SSSR_BIND_UAV_DEPTH_HIERARCHY, r32f)     uniform image2D rw_depth_hierarchy[13];
#endif

// SPD UAV
#if defined SSSR_BIND_UAV_SPD_GLOBAL_ATOMIC
        layout (set = 0, binding = SSSR_BIND_UAV_SPD_GLOBAL_ATOMIC, std430) buffer rw_spd_global_atomic_t
        {
            FfxUInt32 data[];
        } rw_spd_global_atomic;
#endif

#if FFX_HALF

FfxFloat16x4 SpdLoadH(FfxInt32x2 coordinate, FfxUInt32 slice)
{
#if defined (SSSR_BIND_UAV_DEPTH_HIERARCHY) 
    return FfxFloat16x4(imageLoad(rw_depth_hierarchy[6], coordinate).x);   // 5 -> 6 as we store a copy of the depth buffer at index 0
#else
    return FfxFloat16x4(0.0f);
#endif
}

FfxFloat16x4 SpdLoadSourceImageH(FfxInt32x2 coordinate, FfxUInt32 slice)
{
#if defined (SSSR_BIND_SRV_INPUT_DEPTH) 
    return FfxFloat16x4(texelFetch(r_input_depth, coordinate, 0).x);
#else
    return FfxFloat16x4(0.0f);
#endif
}

void SpdStoreH(FfxInt32x2 pix, FfxFloat16x4 outValue, FfxUInt32 coordinate, FfxUInt32 slice)
{
#if defined (SSSR_BIND_UAV_DEPTH_HIERARCHY) 
    imageStore(rw_depth_hierarchy[coordinate + 1], pix, FfxFloat16x4(outValue.x));    // + 1 as we store a copy of the depth buffer at index 0
#endif
}

#endif // #if defined(FFX_HALF)

FfxFloat32x3 FFX_SSSR_LoadWorldSpaceNormal(FfxInt32x2 pixel_coordinate)
{
#if defined(SSSR_BIND_SRV_INPUT_NORMAL)
    // Normals are 
    return normalize(NormalsUnpackMul() * texelFetch(r_input_normal, pixel_coordinate, 0).xyz + NormalsUnpackAdd());
#else
    return FfxFloat32x3(0.0f);
#endif
}

FfxFloat32 FFX_SSSR_LoadDepth(FfxInt32x2 pixel_coordinate, FfxInt32 mip)
{
#if defined(SSSR_BIND_SRV_DEPTH_HIERARCHY) 
    return texelFetch(r_depth_hierarchy, pixel_coordinate, mip).x;
#else
    return 0.0f;
#endif
}

FfxFloat32x2 FFX_SSSR_SampleRandomVector2D(FfxUInt32x2 pixel)
{
#if defined(SSSR_BIND_SRV_BLUE_NOISE_TEXTURE) 
    return texelFetch(r_blue_noise_texture, FfxInt32x2(pixel.xy % 128), 0).xy;
#else
    return FfxFloat32x2(0.0f);
#endif
}

FfxFloat32x3 FFX_SSSR_SampleEnvironmentMap(FfxFloat32x3 direction, FfxFloat32 roughness)
{
#if defined(SSSR_BIND_SRV_INPUT_ENVIRONMENT_MAP)
    FfxInt32x2 cubeSize = textureSize(r_input_environment_map, 0);
    FfxInt32 maxMipLevel = FfxInt32(log2(FfxFloat32(cubeSize.x > 0 ? cubeSize.y : 1)));
    FfxFloat32 lod = clamp(roughness * FfxFloat32(maxMipLevel), 0.0, FfxFloat32(maxMipLevel));
    return textureLod(samplerCube(r_input_environment_map, s_EnvironmentMapSampler), direction, lod).xyz * IBLFactor();
#else
    return FfxFloat32x3(0.0f);
#endif
}

void IncrementRayCounter(FfxUInt32 value, FFX_PARAMETER_OUT FfxUInt32 original_value)
{
#if defined (SSSR_BIND_UAV_RAY_COUNTER) 
    original_value = atomicAdd(rw_ray_counter.data[0], value);
#endif
}

void IncrementDenoiserTileCounter(FFX_PARAMETER_OUT FfxUInt32 original_value)
{
#if defined (SSSR_BIND_UAV_RAY_COUNTER) 
    original_value = atomicAdd(rw_ray_counter.data[2], 1);
#endif
}

FfxUInt32 PackRayCoords(FfxUInt32x2 ray_coord, FfxBoolean copy_horizontal, FfxBoolean copy_vertical, FfxBoolean copy_diagonal)
{
    FfxUInt32 ray_x_15bit = ray_coord.x & 32767;    // 0b111111111111111
    FfxUInt32 ray_y_14bit = ray_coord.y & 16383;    // 0b11111111111111;
    FfxUInt32 copy_horizontal_1bit = copy_horizontal ? 1 : 0;
    FfxUInt32 copy_vertical_1bit = copy_vertical ? 1 : 0;
    FfxUInt32 copy_diagonal_1bit = copy_diagonal ? 1 : 0;

    FfxUInt32 packed = (copy_diagonal_1bit << 31) | (copy_vertical_1bit << 30) | (copy_horizontal_1bit << 29) | (ray_y_14bit << 15) | (ray_x_15bit << 0);
    return packed;
}

void StoreRay(FfxInt32 index, FfxUInt32x2 ray_coord, FfxBoolean copy_horizontal, FfxBoolean copy_vertical, FfxBoolean copy_diagonal)
{
#if defined (SSSR_BIND_UAV_RAY_LIST) 
    FfxUInt32 packedRayCoords = PackRayCoords(ray_coord, copy_horizontal, copy_vertical, copy_diagonal); // Store out pixel to trace
    rw_ray_list.data[index] = packedRayCoords;
#endif
}

void StoreDenoiserTile(FfxInt32 index, FfxUInt32x2 tile_coord)
{
#if defined (SSSR_BIND_UAV_DENOISER_TILE_LIST) 
    rw_denoiser_tile_list.data[index] = ((tile_coord.y & 0xffffu) << 16) | ((tile_coord.x & 0xffffu) << 0); // Store out pixel to trace
#endif
}

FfxBoolean IsReflectiveSurface(FfxUInt32x2 pixel_coordinate, FfxFloat32 roughness)
{
#if defined (SSSR_BIND_SRV_DEPTH_HIERARCHY) 
#if FFX_SSSR_OPTION_INVERTED_DEPTH
    const FfxFloat32 far_plane = 0.0f;
    return texelFetch(r_depth_hierarchy, FfxInt32x2(pixel_coordinate), 0).r > far_plane;
#else //  FFX_SSSR_OPTION_INVERTED_DEPTH
    const FfxFloat32 far_plane = 1.0f;
    return texelFetch(r_depth_hierarchy, FfxInt32x2(pixel_coordinate), 0).r < far_plane;
#endif //  FFX_SSSR_OPTION_INVERTED_DEPTH
#else
    return FFX_FALSE;
#endif
}

void StoreExtractedRoughness(FfxUInt32x2 coordinate, FfxFloat32 roughness)
{
#if defined (SSSR_BIND_UAV_EXTRACTED_ROUGHNESS) 
    imageStore(rw_extracted_roughness, FfxInt32x2(coordinate), FfxFloat32x4(roughness));
#endif
}

FfxFloat32 LoadRoughnessFromMaterialParametersInput(FfxUInt32x3 coordinate)
{
#if defined (SSSR_BIND_SRV_INPUT_MATERIAL_PARAMETERS) 
    FfxFloat32 rawRoughness = texelFetch(r_input_material_parameters, FfxInt32x2(coordinate.xy), FfxInt32(coordinate.z))[RoughnessChannel()];
    if (IsRoughnessPerceptual())
    {
        rawRoughness *= rawRoughness;
    }

    return rawRoughness;
#else
    return 0.0f;
#endif
}

FfxBoolean IsRayIndexValid(FfxUInt32 ray_index)
{
#if defined (SSSR_BIND_UAV_RAY_COUNTER) 
    return ray_index < rw_ray_counter.data[1];
#else
    return FFX_FALSE;
#endif
}

FfxUInt32 GetRaylist(FfxUInt32 ray_index)
{
#if defined (SSSR_BIND_UAV_RAY_LIST) 
    return rw_ray_list.data[ray_index];
#else
    return 0;
#endif
}

void FFX_SSSR_StoreBlueNoiseSample(FfxUInt32x2 coordinate, FfxFloat32x2 blue_noise_sample)
{
#if defined (SSSR_BIND_UAV_BLUE_NOISE_TEXTURE) 
    imageStore(rw_blue_noise_texture, FfxInt32x2(coordinate), FfxFloat32x4(blue_noise_sample, 0.0f, 0.0f));
#endif
}

FfxFloat32 FFX_SSSR_LoadVarianceHistory(FfxInt32x3 coordinate)
{
#if defined ( SSSR_BIND_SRV_VARIANCE ) 
    return texelFetch(r_variance, coordinate.xy, coordinate.z).x;
#else
    return 0.0f;
#endif
}

void FFX_SSSR_StoreRadiance(FfxUInt32x2 coordinate, FfxFloat32x4 radiance)
{
#if defined (SSSR_BIND_UAV_RADIANCE) 
    imageStore(rw_radiance, FfxInt32x2(coordinate), radiance);
#endif
}

FfxUInt32 FFX_SSSR_GetSobolSample(FfxUInt32x3 coordinate)
{
#if defined (SSSR_BIND_SRV_SOBOL_BUFFER) 
    return FfxUInt32(texelFetch(r_sobol_buffer, FfxInt32x2(coordinate.xy), FfxInt32(coordinate.z)).r);
#else
    return 0;
#endif
}

FfxUInt32 FFX_SSSR_GetScramblingTile(FfxUInt32x3 coordinate)
{
#if defined (SSSR_BIND_SRV_SCRAMBLING_TILE_BUFFER) 
    return FfxUInt32(texelFetch(r_scrambling_tile_buffer, FfxInt32x2(coordinate.xy), FfxInt32(coordinate.z)).r);
#else
    return 0;
#endif
}

void FFX_SSSR_WriteIntersectIndirectArgs(FfxUInt32 index, FfxUInt32 data)
{
#if defined SSSR_BIND_UAV_INTERSECTION_PASS_INDIRECT_ARGS
    rw_intersection_pass_indirect_args.data[index] = data;
#endif
}

void FFX_SSSR_WriteRayCounter(FfxUInt32 index, FfxUInt32 data)
{
#if defined (SSSR_BIND_UAV_RAY_COUNTER) 
    rw_ray_counter.data[index] = data;
#endif
}

FfxUInt32 FFX_SSSR_GetRayCounter(FfxUInt32 index)
{
#if defined (SSSR_BIND_UAV_RAY_COUNTER) 
    return rw_ray_counter.data[index];
#else
    return 0;
#endif
}

void FFX_SSSR_GetInputDepthDimensions(FFX_PARAMETER_OUT FfxFloat32x2 image_size)
{
#if defined (SSSR_BIND_SRV_INPUT_DEPTH) 
    image_size = textureSize(sampler2D(r_input_depth, s_LinearSampler) , 0);
#else
    image_size = FfxFloat32x2(0.0f, 0.0f);
#endif
}

void FFX_SSSR_GetDepthHierarchyMipDimensions(FfxUInt32 mip, FFX_PARAMETER_OUT FfxFloat32x2 image_size)
{
#if defined (SSSR_BIND_UAV_DEPTH_HIERARCHY) 
    image_size = imageSize(rw_depth_hierarchy[mip]);
#else
    image_size = FfxFloat32x2(0.0f, 0.0f);
#endif
}

FfxFloat32 FFX_SSSR_GetInputDepth(FfxUInt32x2 coordinate)
{
#if defined (SSSR_BIND_SRV_INPUT_DEPTH) 
    return texelFetch(r_input_depth, FfxInt32x2(coordinate), 0).r;
#else
    return 0.0f;
#endif
}

FfxFloat32x4 SpdLoadSourceImage(FfxInt32x2 coordinate, FfxUInt32 slice)
{
    return FFX_SSSR_GetInputDepth(coordinate).xxxx;
}

void FFX_SSSR_WriteDepthHierarchy(FfxUInt32 index, FfxUInt32x2 coordinate, FfxFloat32 data)
{
#if defined (SSSR_BIND_UAV_DEPTH_HIERARCHY) 
    imageStore(rw_depth_hierarchy[index], FfxInt32x2(coordinate), FfxFloat32x4(data));
#endif
}

FfxFloat32x4 SpdLoad(FfxInt32x2 coordinate, FfxUInt32 slice)
{
#if defined (SSSR_BIND_UAV_DEPTH_HIERARCHY) 
    return FfxFloat32x4(imageLoad(rw_depth_hierarchy[6], coordinate).x);   // 5 -> 6 as we store a copy of the depth buffer at index 0
#else
    return FfxFloat32x4(0.0f);
#endif
}

void SpdStore(FfxInt32x2 pix, FfxFloat32x4 outValue, FfxUInt32 coordinate, FfxUInt32 slice)
{
#if defined (SSSR_BIND_UAV_DEPTH_HIERARCHY) 
    imageStore(rw_depth_hierarchy[coordinate + 1], pix, FfxFloat32x4(outValue.x));    // + 1 as we store a copy of the depth buffer at index 0
#endif
}

void SpdResetAtomicCounter(FfxUInt32 slice)
{
#if defined (SSSR_BIND_UAV_SPD_GLOBAL_ATOMIC) 
    rw_spd_global_atomic.data[0] = 0;
#endif
}

void FFX_SSSR_SPDIncreaseAtomicCounter(FFX_PARAMETER_INOUT FfxUInt32 spdCounter)
{
#if defined (SSSR_BIND_UAV_SPD_GLOBAL_ATOMIC) 
    spdCounter = atomicAdd(rw_spd_global_atomic.data[0], 1);
#endif
}

FfxFloat32x3 FFX_SSSR_LoadInputColor(FfxInt32x3 coordinate)
{
#if defined(SSSR_BIND_SRV_INPUT_COLOR) 
    return texelFetch(r_input_color, coordinate.xy, coordinate.z).xyz;
#else
    return FfxFloat32x3(0.0f);
#endif
}

FfxFloat32 FFX_SSSR_LoadExtractedRoughness(FfxInt32x3 coordinate)
{
#if defined(SSSR_BIND_SRV_EXTRACTED_ROUGHNESS) 
    return texelFetch(r_extracted_roughness, coordinate.xy, coordinate.z).x;
#else
    return 0.0f;
#endif
}

#endif // #if defined(FFX_GPU)
