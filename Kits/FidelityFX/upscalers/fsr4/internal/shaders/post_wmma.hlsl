#include "mlsr_optimized_includes.hlsli"

#define RESOLUTION_1080 0
#define RESOLUTION_2160 1
#define RESOLUTION_4320 2

groupshared uint outputLDS[16*16];

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

#include "AmdExtD3D/AmdExtD3DShaderIntrinsicsMatrixOps.hlsl"
#include "ml2code_runtime/tensor_float16.hlsli"
#include "ml2code_runtime/tensor_float8.hlsli"
#include "ml2code_runtime/tensor_float32.hlsli"

template<int numOutputChannels, typename SI, typename SW0, typename SB0, typename SW1, typename SB1, typename SW2, typename SW3, typename SB2, typename SB3, typename SO>
void CNB_CT2D(
    const float rcprOutputScale0,
    const float inputScale1,
    const float rcprOutputScale1,
    const float inputScale2,
    const float cnbOutputScale,
    const QuantizedTensor3f8_NHWC<SI> input,
    const QuantizedTensor4f8_HWNC<SW0> weights0,
    const Tensor1f<SB0> bias0,
    const QuantizedTensor4f8_HWNC< SW1> weights1,
    const Tensor1f<SB1> bias1,
    const QuantizedTensor4f8_HWNC< SW2> weights2,
    const Tensor1f<SB2> bias2,
    const QuantizedTensor4f8_HWCN< SW3> ct2d_weights,
    const Tensor1f<SB3> ct2d_bias,
    const Tensor3h_NHWC<SO> output,
    const uint3 numThreads,
    const uint3 groupThreadID,
    const uint3 groupID,
    const uint3 dispatchThreadID)
{
    const int3 workOffset = groupID * int3(32, 1, 1);

    int stride = 2;
    int3 poBase = stride * workOffset;
    int3 piBase = workOffset - int3(1, 1, 0);

    AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > conv0OutputsWmma[2];

    // BIAS
    AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix[2];
    uint byteAddress = bias0.OffsetOf(0);
    accumulatorMatrix[0].Load(bias0.storage.buffer, byteAddress, 0, true);
    accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

    [unroll]
    for (uint ky = 0; ky < weights0.logicalSize.y; ++ky)
    {
        [unroll]
        for (uint kx = 0; kx < weights0.logicalSize.x; ++kx)
        {
            [unroll]
            for (uint f = 0; f < weights0.logicalSize.w; f += 16)
            {
                [unroll]
                for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
                {
                    [unroll]
                    for (uint c = 0; c < weights0.logicalSize.z; c += 16)
                    {
                        AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix;
                        inputMatrix.Fill(0);

                        int inputOffset = input.OffsetOf(piBase + int3(kx + inputWaveOffset * 16, ky, c));
                        inputMatrix.Load(input.storage.buffer, inputOffset, input.storageByteStrides.x, true);

                        uint weightOffset = weights0.OffsetOf(uint4(kx, ky, c, f));
                        AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                        weightMatrix.Load(weights0.storage.buffer, weightOffset, weights0.storageByteStrides.w, false);

                        accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
                    }
                }
            }
        }
    }

    [unroll]
    for (uint matId = 0; matId < 2; ++matId)
    {
        conv0OutputsWmma[matId].CopySat(accumulatorMatrix[matId]);
    }

    // Second Convolution + Relu
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> conv1OutputsWmma[32 / 16][2];

    [unroll]
    for (uint f = 0; f < weights1.logicalSize.w; f += 16)
    {
        // BIAS
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
        uint byteAddress = bias1.OffsetOf(f);
        accumulatorMatrix[0].Load(bias1.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights1.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv0OutputsWmma[inputWaveOffset];

                int weightOffset = weights1.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
                weightMatrix.Load(weights1.storage.buffer, weightOffset, weights1.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }

            // Relu()
            [unroll]
            for (uint i = 0; i < accumulatorMatrix[inputWaveOffset].Length(); i++)
            {
                float elem = accumulatorMatrix[inputWaveOffset].Element(i);
                accumulatorMatrix[inputWaveOffset].SetElement(i, max(elem, 0.0f));
            }
        }


        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            conv1OutputsWmma[matId][f/16].CopySat(accumulatorMatrix[matId]);
        }
    }

    // Third Convolution
    AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> outputMats[2];
    AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> residualMats[2];
    {
        int loadOffset = input.OffsetOf(int3(piBase.xy + int2(1, 1), 0));
        residualMats[0].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
        loadOffset = input.OffsetOf(int3(piBase.xy + int2(16 + 1, 1), 0));
        residualMats[1].Load(input.storage.buffer, loadOffset, input.storageByteStrides.x, true);
    }

    [unroll]
    for (uint f = 0; f < weights2.logicalSize.w; f += 16)
    {
        // BIAS
        AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> accumulatorMatrix[2];
        uint byteAddress = bias2.OffsetOf(f);
        accumulatorMatrix[0].Load(bias2.storage.buffer, byteAddress, 0, true);
        accumulatorMatrix[1].Copy(accumulatorMatrix[0]);

        [unroll]
        for (uint inputWaveOffset = 0; inputWaveOffset < (32/16); ++inputWaveOffset)
        {
            [unroll]
            for (uint c = 0; c < weights2.logicalSize.z; c += 16)
            {
                AmdWaveMatrixB<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> inputMatrix = conv1OutputsWmma[inputWaveOffset][c / 16];

                int weightOffset = weights2.OffsetOf(int4(0, 0, c, f));
                AmdWaveMatrixA<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16> weightMatrix;
                weightMatrix.Load(weights2.storage.buffer, weightOffset, weights2.storageByteStrides.w, false);

                accumulatorMatrix[inputWaveOffset] = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix[inputWaveOffset]);
            }
        }

        [unroll]
        for (uint matId = 0; matId < 2; ++matId)
        {
            AmdWaveMatrixAccumulator<AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16> residualF32;
            residualF32.Copy(residualMats[matId]);

            [unroll]
            for (uint i = 0; i < accumulatorMatrix[matId].Length(); i++)
            {
                float accumulatorValue = float(accumulatorMatrix[matId].Element(i));
                float residual = float(residualF32.Element(i));
                accumulatorValue += residual;

                accumulatorMatrix[matId].SetElement(i, accumulatorValue);
            }

            outputMats[matId].CopySat(accumulatorMatrix[matId]);
        }
    }

    // Fourth convolution (x = 2, y = 2, half = 2, W = 1)
    float4 ctBias[2];
    ctBias[0] = Load4f(ct2d_bias, 0);
    ctBias[1] = Load4f(ct2d_bias, 4);

    const float exposureValue = LoadExposureValue();
    const bool shouldConvertColorspace = !rcas_enabled;

    for (uint inputWaveOffset = 0; inputWaveOffset < (32 / 16); ++inputWaveOffset)
    {
        for (uint ky = 0; ky < ct2d_weights.logicalSize.y; ++ky)
        {
            uint kx = 0;
            {
                // Note: Biases init to zero, add later
                AmdWaveMatrixAccumulator < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_F32, 16, 16 > accumulatorMatrix;
                accumulatorMatrix.Fill(0);

                AmdWaveMatrixB < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > inputMatrix = outputMats[inputWaveOffset];

                uint weightOffset = ct2d_weights.OffsetOf(uint4(kx, ky, 0, 0));
                AmdWaveMatrixA < AmdExtD3DShaderIntrinsicsWaveMatrixDataFormat_FP8, 16, 16 > weightMatrix;
                weightMatrix.Load(ct2d_weights.storage.buffer, weightOffset, ct2d_weights.storageByteStrides.z, false);

                accumulatorMatrix = AmdWaveMatrixMultiply(weightMatrix, inputMatrix, accumulatorMatrix);

                [unroll]
                for (uint i = 0; i < accumulatorMatrix.Length(); i++)
                {
                    float matElement = (float)accumulatorMatrix.Element(i);
                    matElement += ctBias[i/4][i%4];

                    accumulatorMatrix.SetElement(i, matElement);
                }

                AMD_GROUPSHARED_STORE(accumulatorMatrix, outputLDS, 0, 16, true);
                float result[8];
                [unroll]
                for (uint i = 0; i < 8; i++)
                {
                    uint linearIdx = i + 8*WaveGetLaneIndex();
                    result[i] = asfloat(outputLDS[linearIdx]);
                }

                // Fused Post-Pass
                const uint2 tid = (int3(poBase) + int3(WaveGetLaneIndex(),ky,0) + int3(stride * inputWaveOffset * 16, 0, 0)).xy;

                // Recurrent Channels
                float4 recurrent = float4(result[4], result[5], result[6], result[7]);
                recurrent = 1.0f / (1.0f + exp(-recurrent));
                rw_recurrent_0[tid] = recurrent; // 4k tid

                const float4 model_parameters = float4(result[0], result[1], result[2], result[3]);
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
    }
}


RWByteAddressBuffer buffer_fused_quantized_NHWC_output;

[numthreads(32, 1, 1)]
void fsr4_model_v07_fp8_no_scale_postpass(
    uint3 ml2c_dispatchThreadId : SV_DispatchThreadID,
    uint3 temp_groupId : SV_GroupID,
    uint3 ml2c_groupThreadId : SV_GroupThreadID
)
{
    uint3 ml2c_groupId = temp_groupId;
    const uint3 ml2c_numThreads = uint3(32, 1, 1);

    const RWBufferStorage storage_fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 = { ScratchBuffer };
#if FFX_MLSR_RESOLUTION == RESOLUTION_1080
    const QuantizedTensor3f8_NHWC< RWBufferStorage > fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 = {
        uint3(960, 540, 16), // logicalSize
        uint3(0, 0, 0), // threadGroupSliceStart
        uint3(960, 540, 16), // threadGroupSliceSize
        uint3(962, 542, 16), // storageSize
        uint3(16, 15392, 1), // storageByteStrides
        uint3(1, 1, 0), // paddingBegin
        uint3(1, 1, 0), // paddingEnd
        0, // threadGroupStorageByteOffset
        1.0, storage_fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 };
#elif FFX_MLSR_RESOLUTION == RESOLUTION_2160
    const QuantizedTensor3f8_NHWC< RWBufferStorage > fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 = {
        uint3(1920, 1080, 16), // logicalSize
        uint3(0, 0, 0), // threadGroupSliceStart
        uint3(1920, 1080, 16), // threadGroupSliceSize
        uint3(1922, 1082, 16), // storageSize
        uint3(16, 30752, 1), // storageByteStrides
        uint3(1, 1, 0), // paddingBegin
        uint3(1, 1, 0), // paddingEnd
        0, // threadGroupStorageByteOffset
        1.0, storage_fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 };
#elif FFX_MLSR_RESOLUTION == RESOLUTION_4320
    const QuantizedTensor3f8_NHWC< RWBufferStorage > fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 = {
        uint3(3840, 2160, 16), // logicalSize
        uint3(0, 0, 0), // threadGroupSliceStart
        uint3(3840, 2160, 16), // threadGroupSliceSize
        uint3(3842, 2162, 16), // storageSize
        uint3(16, 61472, 1), // storageByteStrides
        uint3(1, 1, 0), // paddingBegin
        uint3(1, 1, 0), // paddingEnd
        0, // threadGroupStorageByteOffset
        1.0, storage_fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0 };
#endif

    const BufferStorage storage_hwnc__decoder2_ResidualBlock_2_body_conv_dw_weight_quant_export_handler_QuantizeLinear_output_0 = { InitializerBuffer };
    const QuantizedTensor4f8_HWNC< BufferStorage > hwnc__decoder2_ResidualBlock_2_body_conv_dw_weight_quant_export_handler_QuantizeLinear_output_0 = {
        uint4(3, 3, 16, 16), // logicalSize
        uint4(0, 0, 0, 0), // threadGroupSliceStart
        uint4(3, 3, 16, 16), // threadGroupSliceSize
        uint4(3, 3, 16, 16), // storageSize
        uint4(256, 768, 1, 16), // storageByteStrides
        uint4(0, 0, 0, 0), // paddingBegin
        uint4(0, 0, 0, 0), // paddingEnd
        116008, // threadGroupStorageByteOffset
        1.0, storage_hwnc__decoder2_ResidualBlock_2_body_conv_dw_weight_quant_export_handler_QuantizeLinear_output_0 };
    const BufferStorage storage_decoder2_ResidualBlock_2_body_conv_dw_bias = { InitializerBuffer };
    const Tensor1f< BufferStorage > decoder2_ResidualBlock_2_body_conv_dw_bias = {
        16, // logicalSize
        0, // threadGroupSliceStart
        16, // threadGroupSliceSize
        16, // storageSize
        4, // storageByteStrides
        0, // paddingBegin
        0, // paddingEnd
        6912, // threadGroupStorageByteOffset
        storage_decoder2_ResidualBlock_2_body_conv_dw_bias };
    const BufferStorage storage_hwnc__decoder2_ResidualBlock_2_body_conv_pw_expand_weight_quant_export_handler_QuantizeLinear_output_0 = { InitializerBuffer };
    const QuantizedTensor4f8_HWNC< BufferStorage > hwnc__decoder2_ResidualBlock_2_body_conv_pw_expand_weight_quant_export_handler_QuantizeLinear_output_0 = {
        uint4(1, 1, 16, 32), // logicalSize
        uint4(0, 0, 0, 0), // threadGroupSliceStart
        uint4(1, 1, 16, 32), // threadGroupSliceSize
        uint4(1, 1, 16, 32), // storageSize
        uint4(512, 512, 1, 16), // storageByteStrides
        uint4(0, 0, 0, 0), // paddingBegin
        uint4(0, 0, 0, 0), // paddingEnd
        118312, // threadGroupStorageByteOffset
        1.0, storage_hwnc__decoder2_ResidualBlock_2_body_conv_pw_expand_weight_quant_export_handler_QuantizeLinear_output_0 };
    const BufferStorage storage_decoder2_ResidualBlock_2_body_conv_pw_expand_bias = { InitializerBuffer };
    const Tensor1f< BufferStorage > decoder2_ResidualBlock_2_body_conv_pw_expand_bias = {
        32, // logicalSize
        0, // threadGroupSliceStart
        32, // threadGroupSliceSize
        32, // storageSize
        4, // storageByteStrides
        0, // paddingBegin
        0, // paddingEnd
        6976, // threadGroupStorageByteOffset
        storage_decoder2_ResidualBlock_2_body_conv_pw_expand_bias };
    const BufferStorage storage_hwnc__decoder2_ResidualBlock_2_body_conv_pw_contract_weight_quant_export_handler_QuantizeLinear_output_0 = { InitializerBuffer };
    const QuantizedTensor4f8_HWNC< BufferStorage > hwnc__decoder2_ResidualBlock_2_body_conv_pw_contract_weight_quant_export_handler_QuantizeLinear_output_0 = {
        uint4(1, 1, 32, 16), // logicalSize
        uint4(0, 0, 0, 0), // threadGroupSliceStart
        uint4(1, 1, 32, 16), // threadGroupSliceSize
        uint4(1, 1, 32, 16), // storageSize
        uint4(512, 512, 1, 32), // storageByteStrides
        uint4(0, 0, 0, 0), // paddingBegin
        uint4(0, 0, 0, 0), // paddingEnd
        118824, // threadGroupStorageByteOffset
        1.0, storage_hwnc__decoder2_ResidualBlock_2_body_conv_pw_contract_weight_quant_export_handler_QuantizeLinear_output_0 };
    const BufferStorage storage_decoder2_ResidualBlock_2_body_conv_pw_contract_bias = { InitializerBuffer };
    const Tensor1f< BufferStorage > decoder2_ResidualBlock_2_body_conv_pw_contract_bias = {
        16, // logicalSize
        0, // threadGroupSliceStart
        16, // threadGroupSliceSize
        16, // storageSize
        4, // storageByteStrides
        0, // paddingBegin
        0, // paddingEnd
        7104, // threadGroupStorageByteOffset
        storage_decoder2_ResidualBlock_2_body_conv_pw_contract_bias };
    const BufferStorage storage_hwcn__decoder2_UpscaleConvTranspose2x2_upscale_conv_weight_quant_export_handler_QuantizeLinear_output_0 = { InitializerBuffer };
    const QuantizedTensor4f8_HWCN< BufferStorage > hwcn__decoder2_UpscaleConvTranspose2x2_upscale_conv_weight_quant_export_handler_QuantizeLinear_output_0 = {
        uint4(2, 2, 8, 16), // logicalSize
        uint4(0, 0, 0, 0), // threadGroupSliceStart
        uint4(2, 2, 8, 16), // threadGroupSliceSize
        uint4(2, 2, 8, 16), // storageSize
        uint4(128, 256, 16, 1), // storageByteStrides
        uint4(0, 0, 0, 0), // paddingBegin
        uint4(0, 0, 0, 0), // paddingEnd
        129576, // threadGroupStorageByteOffset
        1.0, storage_hwcn__decoder2_UpscaleConvTranspose2x2_upscale_conv_weight_quant_export_handler_QuantizeLinear_output_0 };
    const BufferStorage storage_decoder2_UpscaleConvTranspose2x2_upscale_conv_bias = { InitializerBuffer };
    const Tensor1f< BufferStorage > decoder2_UpscaleConvTranspose2x2_upscale_conv_bias = {
        8, // logicalSize
        0, // threadGroupSliceStart
        8, // threadGroupSliceSize
        8, // storageSize
        4, // storageByteStrides
        0, // paddingBegin
        0, // paddingEnd
        7168, // threadGroupStorageByteOffset
        storage_decoder2_UpscaleConvTranspose2x2_upscale_conv_bias };
    // fused_quantized_NHWC_output
#if FFX_MLSR_RESOLUTION == RESOLUTION_1080
    const uint3 logicalSize_fused_quantized_NHWC_output = uint3(1920, 1080, 8);
    const int3 groupStart_fused_quantized_NHWC_output = int3(0, 0, 0) + ml2c_groupId.xyz * int3(64, 2, 8);
    const uint3 groupSize_fused_quantized_NHWC_output = uint3(64, 2, 8);
    const uint3 storageSize_fused_quantized_NHWC_output = uint3(1920, 1080, 8);
    const uint3 tensorByteStrides_fused_quantized_NHWC_output = uint3(16, 30720, 2);
#elif FFX_MLSR_RESOLUTION == RESOLUTION_2160
    const uint3 logicalSize_fused_quantized_NHWC_output = uint3(3840, 2160, 8);
    const int3 groupStart_fused_quantized_NHWC_output = int3(0, 0, 0) + ml2c_groupId.xyz * int3(64, 2, 8);
    const uint3 groupSize_fused_quantized_NHWC_output = uint3(64, 2, 8);
    const uint3 storageSize_fused_quantized_NHWC_output = uint3(3840, 2160, 8);
    const uint3 tensorByteStrides_fused_quantized_NHWC_output = uint3(16, 61440, 2);
#elif FFX_MLSR_RESOLUTION == RESOLUTION_4320
    const uint3 logicalSize_fused_quantized_NHWC_output = uint3(7680, 4320, 8);
    const int3 groupStart_fused_quantized_NHWC_output = int3(0, 0, 0) + ml2c_groupId.xyz * int3(64, 2, 8);
    const uint3 groupSize_fused_quantized_NHWC_output = uint3(64, 2, 8);
    const uint3 storageSize_fused_quantized_NHWC_output = uint3(7680, 4320, 8);
    const uint3 tensorByteStrides_fused_quantized_NHWC_output = uint3(16, 122880, 2);
#endif
    const uint3 paddingBegin_fused_quantized_NHWC_output = uint3(0, 0, 0);
    const uint3 paddingEnd_fused_quantized_NHWC_output = uint3(0, 0, 0);
    const int threadGroupByteOffsetInTensor_fused_quantized_NHWC_output = dot(groupStart_fused_quantized_NHWC_output, tensorByteStrides_fused_quantized_NHWC_output);
    const RWBufferStorage storage_fused_quantized_NHWC_output = { buffer_fused_quantized_NHWC_output };
    const Tensor3h_NHWC<RWBufferStorage> fused_quantized_NHWC_output = { logicalSize_fused_quantized_NHWC_output, groupStart_fused_quantized_NHWC_output, groupSize_fused_quantized_NHWC_output, storageSize_fused_quantized_NHWC_output, tensorByteStrides_fused_quantized_NHWC_output, paddingBegin_fused_quantized_NHWC_output, paddingEnd_fused_quantized_NHWC_output, threadGroupByteOffsetInTensor_fused_quantized_NHWC_output + 0, storage_fused_quantized_NHWC_output };
    // FusedFused/decoder2/ResidualBlock_2/body/conv_dw/Conv_/decoder2/ResidualBlock_2/body/conv_pw_expand/Conv_/decoder2/ResidualBlock_2/body/conv_pw_expand_act/Relu_/decoder2/ResidualBlock_2/body/conv_pw_contract/Conv_/decoder2/UpscaleConvTranspose2x2/upscale_conv/ConvTranspose (16, 1080, 1920), (16, 16, 3, 3), (16,), (32, 16, 1, 1), (32,), (16, 32, 1, 1), (16,), (16, 8, 2, 2), (8,) -> (8, 2160, 3840)
    CNB_CT2D<8>(1.0, 1.0, 1.0, 1.0, 1.0, fused_quantized_NHWC__decoder2_ResidualBlock_2_body_input_quantization_act_quant_export_handler_QuantizeLinear_output_0, hwnc__decoder2_ResidualBlock_2_body_conv_dw_weight_quant_export_handler_QuantizeLinear_output_0, decoder2_ResidualBlock_2_body_conv_dw_bias, hwnc__decoder2_ResidualBlock_2_body_conv_pw_expand_weight_quant_export_handler_QuantizeLinear_output_0, decoder2_ResidualBlock_2_body_conv_pw_expand_bias, hwnc__decoder2_ResidualBlock_2_body_conv_pw_contract_weight_quant_export_handler_QuantizeLinear_output_0, decoder2_ResidualBlock_2_body_conv_pw_contract_bias, hwcn__decoder2_UpscaleConvTranspose2x2_upscale_conv_weight_quant_export_handler_QuantizeLinear_output_0, decoder2_UpscaleConvTranspose2x2_upscale_conv_bias, fused_quantized_NHWC_output, ml2c_numThreads, ml2c_groupThreadId, ml2c_groupId, ml2c_dispatchThreadId);
}
