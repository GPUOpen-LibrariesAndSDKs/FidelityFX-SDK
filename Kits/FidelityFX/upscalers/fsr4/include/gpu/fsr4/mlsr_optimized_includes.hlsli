#pragma once

inline float max3(const float a, const float b, const float c)
{
    return max(max(a, b), c);
}

inline float min3(const float a, const float b, const float c)
{
    return min(min(a, b), c);
}
#define FSR4UPSCALER_BIND_CB_FSR4UPSCALER 0
#include "ffx_fsr4_upscale_resources.h"
#include "ffx_fsr4_gpu_colorspace_common.h"

float3 RGBtoYCbCr(float3 rgb)
{
    const float Ka = 0.2126;
    const float Kb = 0.7152;
    const float Kc = 0.0722;
    const float Kd = 1.8556;
    const float Ke = 1.5748;

    const float R = rgb.r;
    const float G = rgb.g;
    const float B = rgb.b;

    const float Y = R * Ka + G * Kb + B * Kc;
    const float Cb = (B - Y) / Kd;
    const float Cr = (R - Y) / Ke;
    return float3(Y, Cb, Cr);
}

float2 LoadInputVelocity(int2 pos)
{
    float2 velocity = r_velocity[pos].xy * mv_scale;
    #if FFX_MLSR_JITTERED_MOTION_VECTORS == 1
        velocity -= fMotionVectorJitterCancellation;
    #endif
    return velocity;
}

float2 SampleInputVelocity(float2 uv)
{
    float2 velocity = r_velocity.SampleLevel(g_history_sampler, uv, 0).xy * mv_scale;
    #if FFX_MLSR_JITTERED_MOTION_VECTORS == 1
        velocity -= fMotionVectorJitterCancellation;
    #endif
    return velocity;
}

void WriteFinalUpscaledColor(uint2 iPxPos, DATA_TYPE3 color)
{
    rw_mlsr_output_color[iPxPos] = color;
}

float3 LoadInputColor(int2 iPxPos)
{
    // Clamp input position.
    iPxPos = clamp(iPxPos, int2(0,0), int2(width_lr-1, height_lr-1));
    float3 inputColor = r_input_color[iPxPos].rgb;

    return inputColor;
}

float FilterWeightGaussian(float dist, float upscale_factor)
{
    //f(x)=1.0 / (1.0 + 2.0*x^2 + 5.0*x^4)

    const float exp_input_factor = -0.5 / (0.47 * 0.47);
    const float x = upscale_factor * dist;
    return exp(x * x * exp_input_factor);
}

float4 optimized_input_upscale_user(uint2 tid, const float2 origin_offset, const float exposureValue)
{
    const float2 input_pos_f = (-0.5 + inv_scale / 2 + float2(tid)*inv_scale) + jitter;
    const int2 input_pos_i = int2(floor(input_pos_f));

    const int sample_pos_i_xL = input_pos_i.x;
    const int sample_pos_i_xR = input_pos_i.x + 1;
    const int sample_pos_i_yT = input_pos_i.y;
    const int sample_pos_i_yB = input_pos_i.y + 1;

    const float3 sample_TL = ApplyMuLaw(LoadInputColor(uint2(sample_pos_i_xL, sample_pos_i_yT)) * exposureValue);
    const float3 sample_TR = ApplyMuLaw(LoadInputColor(uint2(sample_pos_i_xR, sample_pos_i_yT)) * exposureValue);
    const float3 sample_BL = ApplyMuLaw(LoadInputColor(uint2(sample_pos_i_xL, sample_pos_i_yB)) * exposureValue);
    const float3 sample_BR = ApplyMuLaw(LoadInputColor(uint2(sample_pos_i_xR, sample_pos_i_yB)) * exposureValue);

    const float X1 = FilterWeightGaussian(abs(float(sample_pos_i_xL) - input_pos_f.x), scale.x);
    const float X2 = FilterWeightGaussian(abs(float(sample_pos_i_xR) - input_pos_f.x), scale.x);
    const float Y1 = FilterWeightGaussian(abs(float(sample_pos_i_yT) - input_pos_f.y), scale.y);
    const float Y2 = FilterWeightGaussian(abs(float(sample_pos_i_yB) - input_pos_f.y), scale.y);

    float weight_TL = X1 * Y1;
    float weight_TR = X2 * Y1;
    float weight_BL = X1 * Y2;
    float weight_BR = X2 * Y2;
    float weight_sum = weight_TL + weight_TR + weight_BL + weight_BR;

    float center_weight = max(max3(
        weight_TL,
        weight_TR,
        weight_BL),
        weight_BR);

    float3 total_pixel = weight_TL * sample_TL;
    total_pixel += weight_TR * sample_TR;
    total_pixel += weight_BL * sample_BL;
    total_pixel += weight_BR * sample_BR;

    return float4(total_pixel / weight_sum, center_weight);
}

int2 dilate_mv(uint2 tid, float2 origin_offset, Texture2D<float> depth_tex)
{
    ///-----------------------------------------
        /// MV upscale / Dilation
    ///-----------------------------------------

    // Note using 9 samples, but opt shader uses 6 (standard optimization for taau).
    //const float2 dist = lerp(float2(1.f, 1.f), scale, dilation_distance);

    static const uint kNumNeighboursDepth = 8;
    static const float2 kDepthOffsets[kNumNeighboursDepth] =
    {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0,  -1}, /*{0,  0},*/ {0,  1},
        {1,  -1}, {1,  0}, {1,  1}
    };

#if FFX_MLSR_LOW_RES_MV
    {
        int2 center_pos = round(float2(tid)*inv_scale + origin_offset); // depth (and MV) aren't jittered
        float center_depth = depth_tex[center_pos];

        float closest_depth = center_depth;
        int2 selected_pos = center_pos;

        // assume a central sample is the closest
        [unroll(kNumNeighboursDepth)]
        for (uint i = 0; i < kNumNeighboursDepth; i++)
        {
            //int2 pos_depth_neigbour = round(float2(tid + dist*kDepthOffsets[i])*inv_scale + origin_offset);
            int2 pos_depth_neigbour = round(float2(tid + kDepthOffsets[i])*inv_scale + origin_offset);
            float depth_neighbour = depth_tex[pos_depth_neigbour];

        #if FFX_MLSR_DEPTH_INVERTED
            if (depth_neighbour > closest_depth)
        #else
            if (depth_neighbour < closest_depth)
        #endif
            {
                closest_depth = depth_neighbour;
                selected_pos = pos_depth_neigbour;
            }
        }

        return selected_pos;
    }
#else
    {
        // Note that high resolution MV is assumed to be pre-dilated (by the game team).
        // depth (and MV) aren't jittered

        return tid;
    }
#endif
}

bool local_disocclusion_user(uint2 tid, inout float2 velocity)
{
    // Assume a velocity above 1 million is invalid.
    if (any(isnan(velocity)) || any(isinf(velocity)) || any(velocity > 1000000.f))
    {
        velocity = float2(0, 0);
        return true;
    }

    return false;
}

void DebugWritePredictedBlendFactor(int2 iPxPos, float blendFactor)
{
#if FFX_DEBUG_VISUALIZE
    rw_debug_visualization[iPxPos] = blendFactor;
#endif
}

float DebugSamplePredictedBlendFactor(float2 fUv)
{
#if FFX_DEBUG_VISUALIZE
    return r_debug_visualization.SampleLevel(g_history_sampler, fUv, 0);
#else
    return 0.0f;
#endif
}
