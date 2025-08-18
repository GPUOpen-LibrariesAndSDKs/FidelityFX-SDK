#include "mlsr_optimized_includes.hlsli"

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"
#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

#define RESOLUTION_1080 0
#define RESOLUTION_2160 1
#define RESOLUTION_4320 2

// The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
void SampleTextureCatmullRom(in float2 reprojectedPos, in float2 texSize, out float3 history_color)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = reprojectedPos * texSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    float3 result = 0.0f;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += r_history_color.SampleLevel(g_history_sampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    history_color = result;
}

uint32_t packf16(half2 values)
{
    return (uint32_t)asuint16(values.x) | (((uint32_t)asuint16(values.y) << 16));
}

groupshared uint inputLDS[16*16*2];
static const uint widthOffset = 16*16;
static const uint heightOffset = (16*16)/2;

[numthreads(32, 1, 1)]
void fsr4_model_v07_fp8_no_scale_prepass(uint3 temp_gid : SV_GroupID, uint2 gtid : SV_GroupThreadID, uint2 real_dtid : SV_DispatchThreadID)
{
    uint3 gid = temp_gid;
    uint3 ml2c_groupId = gid;
    const float exposureValue = LoadExposureValue();

    const BufferStorage storage_encoder1_DownscaleStridedConv2x2_downscale_conv_weight = { InitializerBuffer };
    const Tensor4h_HNWC< BufferStorage > weights = {
        uint4(2, 2, 7, 16), // logicalSize
        uint4(0, 0, 0, 0), // threadGroupSliceStart
        uint4(2, 2, 7, 16), // threadGroupSliceSize
        uint4(2, 2, 8, 16), // storageSize
        uint4(16, 512, 2, 32), // storageByteStrides
        uint4(0, 0, 0, 0), // paddingBegin
        uint4(0, 0, 0, 0), // paddingEnd
        0, // threadGroupStorageByteOffset
        storage_encoder1_DownscaleStridedConv2x2_downscale_conv_weight };
    const BufferStorage storage_encoder1_DownscaleStridedConv2x2_downscale_conv_bias = { InitializerBuffer };
    const Tensor1f< BufferStorage > bias = {
        16, // logicalSize
        0, // threadGroupSliceStart
        16, // threadGroupSliceSize
        16, // storageSize
        4, // storageByteStrides
        0, // paddingBegin
        0, // paddingEnd
        1024, // threadGroupStorageByteOffset
        storage_encoder1_DownscaleStridedConv2x2_downscale_conv_bias };

    const int3 groupStart_slice_0 = int3(0, 0, 0) + ml2c_groupId.xyz * int3(32, 1, 1);
#if FFX_MLSR_RESOLUTION == RESOLUTION_1080
    const uint3 logicalSize_slice_0 = uint3(960, 540, 16);
    const uint3 groupSize_slice_0 = uint3(16, 1, 16);
    const uint3 storageSize_slice_0 = uint3(962, 542, 16);
    const uint3 tensorByteStrides_slice_0 = uint3(16, 15392, 1);
#elif FFX_MLSR_RESOLUTION == RESOLUTION_2160
    const uint3 logicalSize_slice_0 = uint3(1920, 1080, 16);
    const uint3 groupSize_slice_0 = uint3(16, 1, 16);
    const uint3 storageSize_slice_0 = uint3(1922, 1082, 16);
    const uint3 tensorByteStrides_slice_0 = uint3(16, 30752, 1);
#elif FFX_MLSR_RESOLUTION == RESOLUTION_4320
    const uint3 logicalSize_slice_0 = uint3(3840, 2160, 16);
    const uint3 groupSize_slice_0 = uint3(16, 1, 16);
    const uint3 storageSize_slice_0 = uint3(3842, 2162, 16);
    const uint3 tensorByteStrides_slice_0 = uint3(16, 61472, 1);
#endif
    const uint3 paddingBegin_slice_0 = uint3(1, 1, 0);
    const uint3 paddingEnd_slice_0 = uint3(1, 1, 0);
    const int threadGroupByteOffsetInTensor_slice_0 = dot(groupStart_slice_0, tensorByteStrides_slice_0);
    const float quantizationScale_slice_0 = 1.0;
    const RWBufferStorage storage_slice_0 = { ScratchBuffer };
    const QuantizedTensor3f8_NHWC<RWBufferStorage> slice_0 = { logicalSize_slice_0, groupStart_slice_0, groupSize_slice_0, storageSize_slice_0, tensorByteStrides_slice_0, paddingBegin_slice_0, paddingEnd_slice_0, threadGroupByteOffsetInTensor_slice_0 + 0, quantizationScale_slice_0, storage_slice_0 };


    for (int xOffset = 0; xOffset < 2; ++xOffset)
    {
        [unroll]
        for (int ky = 0; ky < 2; ++ky)
        {
            uint2 dtid = gid.xy * uint2(64,2) + gtid.xy;
            dtid.x += 32 * xOffset;
            dtid.y += ky;

            const uint dst_idx = dtid.y * width + dtid.x;
            float2 origin_offset = 0.5 * inv_scale - 0.5f;

            float4 upscale_input = optimized_input_upscale_user(dtid, origin_offset, exposureValue);

            const float history_rect_alpha = 0.1f;
            float temporal_sample_weight = upscale_input.a * history_rect_alpha;

            int2 mv_dialated_pos = dilate_mv(dtid, origin_offset, r_depth);
            float2 velocity = LoadInputVelocity(mv_dialated_pos);

            bool disocclusion = local_disocclusion_user(dtid, velocity);

            // on reset frames, model is trained to the current frame as both input and history
            // Otherwise (offscreen reprojection) the default is 0.0f
            float3 history = reset ? upscale_input.rgb : 0.f;
            float4 recurrent = 0;
            float2 dstSize = { width, height };

            if (disocclusion == 0 && reset == 0)
            {
                ///-----------------------------------------
                /// Reprojection
                ///-----------------------------------------
                const float2 history_xy = float2(dtid)+velocity * dstSize;
                const float2 history_uv = (history_xy + 0.5f) / tex_size;

                bool onscreen = (0 <= history_xy.x && history_xy.x <= (width-1) && 0 <= history_xy.y && history_xy.y <= (height-1));

                if (onscreen)
                {
                    SampleTextureCatmullRom(history_uv, tex_size, history);
                    recurrent = r_recurrent_0.SampleLevel(g_history_sampler, history_uv, 0);
                }

                // History is unclamped PQ right now. don't saturate at 1.
                history = max(history, 0);
                history = temporal_sample_weight * upscale_input.rgb + (1.0 - temporal_sample_weight) * history;
            }

            rw_reprojected_color[dtid.xy] = DATA_TYPE3(history);

            half4 downscaleInputs[2];

            // Y + Y + Color Distance (cbcr/euclidean)
            {
                float3 input_ycbcr = RGBtoYCbCr(upscale_input.xyz);
                float3 history_ycbcr = RGBtoYCbCr(history);
                const float color_distance = sqrt(pow(input_ycbcr.y - history_ycbcr.y, 2) + pow(input_ycbcr.z - history_ycbcr.z, 2) + 1e-6);

                downscaleInputs[0].x = MODEL_COLOR_NORMALIZE(NETWORK_DATA_TYPE(input_ycbcr.x));
                downscaleInputs[0].y = MODEL_COLOR_NORMALIZE(NETWORK_DATA_TYPE(history_ycbcr.x));
                downscaleInputs[0].z = NETWORK_DATA_TYPE(color_distance);

            }

            // RFM / Recurrent data
            {
                downscaleInputs[0].w = MODEL_RFM_NORMALIZE(NETWORK_DATA_TYPE(recurrent[0]));
                downscaleInputs[1].x = MODEL_RFM_NORMALIZE(NETWORK_DATA_TYPE(recurrent[1]));
                downscaleInputs[1].y = MODEL_RFM_NORMALIZE(NETWORK_DATA_TYPE(recurrent[2]));
                downscaleInputs[1].z = MODEL_RFM_NORMALIZE(NETWORK_DATA_TYPE(recurrent[3]));
                downscaleInputs[1].w = 0;
            }

            inputLDS[xOffset*widthOffset + heightOffset*ky + WaveGetLaneIndex()*4+0] = packf16(downscaleInputs[0].xy);
            inputLDS[xOffset*widthOffset + heightOffset*ky + WaveGetLaneIndex()*4+1] = packf16(downscaleInputs[0].zw);
            inputLDS[xOffset*widthOffset + heightOffset*ky + WaveGetLaneIndex()*4+2] = packf16(downscaleInputs[1].xy);
            inputLDS[xOffset*widthOffset + heightOffset*ky + WaveGetLaneIndex()*4+3] = packf16(downscaleInputs[1].zw);
        }
    }

    // Downscale
    AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > weightMatrix[2];
    [unroll]
    for (int ky = 0; ky < 2; ++ky)
    {
        uint weightOffset = weights.threadGroupStorageByteOffset + 512 * ky;
        weightMatrix[ky].Load(weights.storage.buffer, weightOffset, 32, false);
    }

    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
    const uint biasOffset = bias.OffsetOf(0);
    accumulatorMatrix[0].Load(bias.storage.buffer, biasOffset, 0, true);
    accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

    [unroll]
    for (int xOffset = 0; xOffset < 2; ++xOffset)
    {
        [unroll]
        for (int ky = 0; ky < 2; ++ky)
        {
            // WMMA
            AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F16, 16, 16 > inputMatrix;
            AMD_GROUPSHARED_LOAD(inputMatrix, inputLDS, xOffset*widthOffset + heightOffset*ky, 8, true);

            accumulatorMatrix[xOffset] = AmdWaveMatrixMultiply(weightMatrix[ky], inputMatrix, accumulatorMatrix[xOffset]);
        }

        AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > convOutputsWmma;
        convOutputsWmma.CopySat(accumulatorMatrix[xOffset]);

        uint storeOffset = slice_0.OffsetOf(int3(gid.xy * uint2(32, 1) + int2(16 * xOffset, 0), 0));
        convOutputsWmma.Store(ScratchBuffer, storeOffset, 16, true);
    }
}
