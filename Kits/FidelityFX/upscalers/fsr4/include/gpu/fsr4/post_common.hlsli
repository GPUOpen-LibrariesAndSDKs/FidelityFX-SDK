#include "mlsr_optimized_includes.hlsli"

#define RESOLUTION_1080 0
#define RESOLUTION_2160 1
#define RESOLUTION_4320 2

float3 apply_model_filter(uint x, uint y, const float4 filter, const float exposureValue)
{
    const uint kernelSize = 3;

    const float correlation_factor = ((exp(filter[0]) - exp(-filter[0])) / (exp(filter[0]) + exp(-filter[0])));
    const float scale_factor_x = 2.0 / (1.0 + exp(-filter[1]));
    const float scale_factor_y = 2.0 / (1.0 + exp(-filter[2]));

    const float scale_factor_x2 = scale_factor_x * scale_factor_x;
    const float scale_factor_y2 = scale_factor_y * scale_factor_y;
    const float scale_factor_xy = scale_factor_x * scale_factor_y;

    const float2 lr_pos_f = (-0.5 + inv_scale / 2 + float2(x, y) * inv_scale) + jitter;
    int2 lr_pos_i = round(lr_pos_f);
    if (kernelSize % 2 == 0)
        lr_pos_i = floor(lr_pos_f);

    const uint kernel_radius = kernelSize >> 1;
    float3 upscaled_color = 0.f;
    float total_weight = 0.f;
    [unroll]
    for (uint dy = 0; dy < kernelSize; ++dy)
    {
        const uint y_in = lr_pos_i.y + dy - kernel_radius;
        const float y_dist = ((float)y_in - lr_pos_f.y) * scale.y;
        const float y_dist2 = y_dist * y_dist;

        [unroll]
        for (uint dx = 0; dx < kernelSize; ++dx)
        {
            const uint x_in = lr_pos_i.x + dx - kernel_radius;
            const float x_dist = ((float)x_in - lr_pos_f.x) * scale.x;
            const float x_dist2 = x_dist * x_dist;

            const float gauss_weight = exp((-0.5 / (0.47 * 0.47)) * (x_dist2 * scale_factor_x2 + 2 * correlation_factor * (x_dist * y_dist) * scale_factor_xy + y_dist2 * scale_factor_y2));
            total_weight += gauss_weight;

            upscaled_color += ApplyMuLaw(LoadInputColor(uint2(x_in, y_in)) * exposureValue) * gauss_weight;
        }
    }
    upscaled_color /= total_weight;

    const float blending_factor = (1.0 / (1.0 + exp(-filter[3])));

    float3 res = upscaled_color * (1.0 - blending_factor) + r_reprojected_color[uint2(x, y)] * blending_factor;
    return res;
}

#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/scalar_functions.hlsli"

template<typename SW>
half4 ct2d_iteration_4(
    const uint z,
    const int2 pk,
    const uint4 packedVs,
    const QuantizedTensor4i8_HWCN<SW> weights,
    const float inputsScale)
{
    int4 accumulator = 0;
    uint weightsOffset = weights.OffsetOf(uint4(pk, z, 0));

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w0 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16;

        accumulator.x = dot4add_i8packed(w0.x, packedVs.x, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.y, packedVs.y, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.z, packedVs.z, accumulator.x);
        accumulator.x = dot4add_i8packed(w0.w, packedVs.w, accumulator.x);
    }

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w1 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16*weights.storageByteStrides.w;

        accumulator.y = dot4add_i8packed(w1.x, packedVs.x, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.y, packedVs.y, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.z, packedVs.z, accumulator.y);
        accumulator.y = dot4add_i8packed(w1.w, packedVs.w, accumulator.y);
    }

    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w2 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16*weights.storageByteStrides.w;

        accumulator.z = dot4add_i8packed(w2.x, packedVs.x, accumulator.z);
        accumulator.z = dot4add_i8packed(w2.y, packedVs.y, accumulator.z);
        accumulator.z = dot4add_i8packed(w2.z, packedVs.z, accumulator.z);
        accumulator.z = dot4add_i8packed(w2.w, packedVs.w, accumulator.z);
    }


    [unroll]
    for (uint c = 0 ; c != 16; c += 16)
    {
        const uint4 w3 = weights.storage.Load4(weightsOffset);
        weightsOffset += 16*weights.storageByteStrides.w;

        accumulator.w = dot4add_i8packed(w3.x, packedVs.x, accumulator.w);
        accumulator.w = dot4add_i8packed(w3.y, packedVs.y, accumulator.w);
        accumulator.w = dot4add_i8packed(w3.z, packedVs.z, accumulator.w);
        accumulator.w = dot4add_i8packed(w3.w, packedVs.w, accumulator.w);
    }

    return half4(accumulator * inputsScale.xxxx);
}


template<typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void FusedConv2D_DW_Conv2D_PW_Relu_Conv2D_Add_ConvTranspose2D(
    const float rcprOutputScale0,
    const float inputScale1,
    const float rcprOutputScale1,
    const float inputScale2,
    const float cnbOutputScale,
    const QuantizedTensor3i8_NHWC<SI> input,
    const QuantizedTensor4i8_NHWC<SW0> weights0,
    const Tensor1h<SB0> bias0,
    const QuantizedTensor4i8_NHWC<SW1> weights1,
    const Tensor1h<SB1> bias1,
    const QuantizedTensor4i8_NHWC<SW2> weights2,
    const Tensor1h<SB2> bias2,
    const QuantizedTensor4i8_HWCN<SW3> ct2d_weights,
    const Tensor1h<SB3> ct2d_bias,
    const Tensor3h_NHWC<SO> output,
    const uint3 numThreads,
    const uint3 groupThreadID,
    const uint3 groupID,
    const uint3 dispatchThreadID)
{
    const uint numFeatures = weights0.logicalSize.w;
    const int3 workOffset = int3(dispatchThreadID.xy, 0);

    const int3 poBase = workOffset;
    const int3 piBase = int3(poBase.xy, 0) - int3(1, 1, 0);

    // CNB
    int accumulator[16];
    int relu_output[32];
    [unroll]
    for (uint i = 0; i < 16; ++i) { accumulator[i] = 0; }
    [unroll]
    for (uint i = 0; i < 32; ++i) { relu_output[i] = 0; }

    // First convolution
    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            if (!ValidPosition(input, int3(piBase.xy + int2(kx, ky), 0)))
                continue;

            int8_t4_packed vs[16/4];
            uint inputIndex = 0;
            [unroll]
            for (uint c = 0; c != weights0.logicalSize.z; c += 16)
            {
                const uint inputOffset = input.OffsetOf(piBase + int3(kx, ky, c));
                const uint4 inputDwords = input.storage.Load4(inputOffset);
                vs[inputIndex++] = inputDwords.x;
                vs[inputIndex++] = inputDwords.y;
                vs[inputIndex++] = inputDwords.z;
                vs[inputIndex++] = inputDwords.w;
            }

            [unroll]
            for (uint f = 0; f != numFeatures; ++f)
            {
                inputIndex = 0;
                [unroll]
                for (uint c = 0; c != weights0.logicalSize.z; c += 16)
                {
                    const uint weightsOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                    const uint4 weightsDwords = weights0.storage.Load4(weightsOffset);

                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.x, accumulator[f]);
                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.y, accumulator[f]);
                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.z, accumulator[f]);
                    accumulator[f] = dot4add_i8packed(vs[inputIndex++], weightsDwords.w, accumulator[f]);
                }
            }
        }

    int8_t4_packed conv_result[16/4];
    float inputscale = input.quantizationScale * weights0.quantizationScale;
    [unroll]
    for (uint f = 0; f != numFeatures; f+=4)
    {
        const float4 conv_accumulator = float4(accumulator[f], accumulator[f+1], accumulator[f+2], accumulator[f+3]);
        const int4 quantized_vs = round((conv_accumulator * inputscale + Load4hA(bias0, f)) * rcprOutputScale0);
        conv_result[f/4] = pack_clamp_s8(quantized_vs);
    }

    // Second Convolution + Relu
    [unroll]
    for (uint i = 0; i < 32; i++)
    {
        const uint weightsOffset = weights1.OffsetOf(uint4(0,0,0,i));
        const uint4 weightsDwords = weights1.storage.Load4(weightsOffset);

        relu_output[i] = dot4add_i8packed(conv_result[0], weightsDwords.x, relu_output[i]);
        relu_output[i] = dot4add_i8packed(conv_result[1], weightsDwords.y, relu_output[i]);
        relu_output[i] = dot4add_i8packed(conv_result[2], weightsDwords.z, relu_output[i]);
        relu_output[i] = dot4add_i8packed(conv_result[3], weightsDwords.w, relu_output[i]);
    }

    int8_t4_packed relu_result[32/4];
    inputscale = inputScale1 * weights1.quantizationScale;
    [unroll]
    for (uint i = 0; i < 32; i+=4)
    {
        const float4 vs = float4(relu_output[i], relu_output[i+1], relu_output[i+2], relu_output[i+3]);
        const int4 quantized_vs = round((vs * inputscale + Load4hA(bias1, i)) * rcprOutputScale1);
        relu_result[i/4] = pack_s8(ReluClampS8(quantized_vs));
    }

    int conv3_output[16];
    [unroll]
    for (int i = 0; i < 16; ++i) { conv3_output[i] = 0; }

    // Third Convolution
    [unroll]
    for (uint i = 0; i < 16; i++)
    {
        uint weightsOffset = weights2.OffsetOf(uint4(0,0,0,i));
        uint4 weightsDwords = weights2.storage.Load4(weightsOffset);

        conv3_output[i] = dot4add_i8packed(relu_result[0], weightsDwords.x, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[1], weightsDwords.y, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[2], weightsDwords.z, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[3], weightsDwords.w, conv3_output[i]);

        weightsOffset = weights2.OffsetOf(uint4(0,0,16,i));
        weightsDwords = weights2.storage.Load4(weightsOffset);

        conv3_output[i] = dot4add_i8packed(relu_result[4], weightsDwords.x, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[5], weightsDwords.y, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[6], weightsDwords.z, conv3_output[i]);
        conv3_output[i] = dot4add_i8packed(relu_result[7], weightsDwords.w, conv3_output[i]);
    }

    // Add
    const uint4 inputDwords = input.storage.Load4(input.OffsetOf(int3(poBase.xy, 0)));

    uint4 packedVss;
    inputscale = weights2.quantizationScale * inputScale2;
    const float outputRcprScale = 1.0f / cnbOutputScale;
    [unroll]
    for (uint i = 0; i < 16; i+=4)
    {
        const float4 vs = float4(conv3_output[i], conv3_output[i+1], conv3_output[i+2], conv3_output[i+3]);
        float4 result = vs * inputscale + Load4hA(bias2, i);
        result += unpack_s8s32(inputDwords[i/4]) * input.quantizationScale;

        packedVss[i/4] = pack_clamp_s8(int4(round(result * outputRcprScale)));
    }

    // CT2D
    const float ct2d_input_quantizationScale = cnbOutputScale * ct2d_weights.quantizationScale;
    const int2 localOffset[4] = { int2(0,0), int2(1,0), int2(0,1), int2(1,1) };

    const uint4 biasDwords = ct2d_bias.storage.Load4(0);

    const float exposureValue = LoadExposureValue();
    const bool shouldConvertColorspace = !rcas_enabled;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        const int2 pk = localOffset[i];
        const uint2 tid = dispatchThreadID.xy * 2 + pk;

        // Recurrent Channels
        half4 recurrent = ct2d_iteration_4(4, pk, packedVss, ct2d_weights, ct2d_input_quantizationScale) + Unpack4h(biasDwords.zw);
        recurrent = 1.0f / (1.0f + exp(-recurrent));
        rw_recurrent_0[tid] = recurrent; // 4k tid

        const float4 model_parameters = ct2d_iteration_4(0, pk, packedVss, ct2d_weights, ct2d_input_quantizationScale) + Unpack4h(biasDwords.xy);
        float3 model_color = apply_model_filter(tid.x, tid.y, model_parameters, exposureValue); // 4k tid

#if FFX_DEBUG_VISUALIZE
        {
            float blending_factor = (1.0f / (1.0f + exp(-model_parameters.w)));
            blending_factor = 1.0f - blending_factor;
            DebugWritePredictedBlendFactor(tid, blending_factor);
        }
#endif

        // Model color (in model color space) is retained for history.
        rw_history_color[tid] = model_color; // 4k tid

        // "Final" color is converted to input color space unless RCAS is enabled
        // in which case RCAS will convert it.
        float3 final_color = model_color;

        if (shouldConvertColorspace)
            final_color.rgb = RemoveMuLaw(final_color.rgb) / exposureValue;

        final_color = clamp(final_color, 0, 64000); // Don't overflow storage (half) type.

        rw_mlsr_output_color[tid] = final_color; // 4k tid

    }

}

RWByteAddressBuffer buffer_fused_quantized_NHWC_output;
