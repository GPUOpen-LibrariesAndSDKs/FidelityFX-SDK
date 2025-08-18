#if !defined(DATA_TYPE)
#define DATA_TYPE float16_t
#endif

#if !defined(DATA_TYPE2)
#define DATA_TYPE2 float16_t2
#endif

#if !defined(DATA_TYPE3)
#define DATA_TYPE3 float16_t3
#endif

#if !defined(DATA_TYPE_VECTOR)
#define DATA_TYPE_VECTOR float16_t4
#endif

#if !defined(NETWORK_DATA_TYPE)
#define NETWORK_DATA_TYPE DATA_TYPE
#endif

#if !defined(RFM_CHANNELS)
#define RFM_CHANNELS 4
#endif

// Round up to multiple of 4
#define RFM_TEXTURES ((RFM_CHANNELS + 3) / 4)

#if !defined(FFM_VECTOR_DATA)
#define FFM_VECTOR_DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#endif

static const float ffm_vector_data[32] = {
    FFM_VECTOR_DATA
};

#ifndef RFM_ACTIVATION
#define RFM_ACTIVATION RfmActivation_None
#endif

#ifndef MODEL_INPUTS_NORMALIZE
#define MODEL_INPUTS_NORMALIZE 1
#endif

#if MODEL_INPUTS_NORMALIZE
#ifndef MODEL_COLOR_NORMALIZE
#define MODEL_COLOR_NORMALIZE(x) (2 * (x) - 1)
#endif

#ifndef MODEL_RFM_NORMALIZE
#define MODEL_RFM_NORMALIZE(x) (2 * (x) - 1)
#endif

#ifndef MODEL_COLOR_DENORMALIZE
#define MODEL_COLOR_DENORMALIZE(x) ((x + 1)*0.5)
#endif

#ifndef MODEL_RFM_DENORMALIZE
#define MODEL_RFM_DENORMALIZE(x) ((x + 1)*0.5)
#endif
#else
#ifndef MODEL_COLOR_NORMALIZE
#define MODEL_COLOR_NORMALIZE(x) (x)
#endif

#ifndef MODEL_RFM_NORMALIZE
#define MODEL_RFM_NORMALIZE(x) (x)
#endif

#ifndef MODEL_COLOR_DENORMALIZE
#define MODEL_COLOR_DENORMALIZE(x) (x)
#endif

#ifndef MODEL_RFM_DENORMALIZE
#define MODEL_RFM_DENORMALIZE(x) (x)
#endif
#endif

static const float MLSR_EPSILON = 1e-10f;
static const float MLSR_FLT_MAX = 3.402823466e+38f;
static const float MLSR_FLT_MIN = 1.175494351e-38f;

static const float reconstructedDepthBilinearWeightThreshold = 0.05f;
//static const uint2 RenderSize = int2(width_lr, height_lr);
//static const uint2 DisplaySize = int2(width, height);

#define DECLARE_SRV_REGISTER(regIndex)  t##regIndex
#define DECLARE_UAV_REGISTER(regIndex)  u##regIndex
#define DECLARE_CB_REGISTER(regIndex)   b##regIndex
#define FFX_FSR4UPSCALER_DECLARE_SRV(regIndex)  register(DECLARE_SRV_REGISTER(regIndex))
#define FFX_FSR4UPSCALER_DECLARE_UAV(regIndex)  register(DECLARE_UAV_REGISTER(regIndex))
#define FFX_FSR4UPSCALER_DECLARE_CB(regIndex)   register(DECLARE_CB_REGISTER(regIndex))

SamplerState g_history_sampler : register(s0);

Texture2D<DATA_TYPE3>                 r_history_color            : register(t0);
Texture2D<float2>                     r_velocity                 : register(t1);
Texture2D<float>                      r_depth                    : register(t2);
Texture2D<float3>                     r_input_color              : register(t3);
Texture2D<DATA_TYPE_VECTOR>           r_recurrent_0              : register(t4);
Texture2D<DATA_TYPE_VECTOR>           r_recurrent_1              : register(t5);
Texture2D<float>                      r_input_exposure           : register(t6);
StructuredBuffer<float>               r_auto_exposure            : register(t7);
Texture2D<DATA_TYPE3>                 r_upsampled_color          : register(t8);
Texture2D<DATA_TYPE3>                 r_reprojected_color        : register(t9);
StructuredBuffer<NETWORK_DATA_TYPE>   r_network_output_rfm       : register(t10); // Network output: RFM
StructuredBuffer<NETWORK_DATA_TYPE>   r_network_output_other     : register(t11); // Network output: Other outputs. (may be same resource as r_network_output_rfm)
Texture2D<float2>                     r_sr_mv_dilated            : register(t12);
Texture2D<float2>                     r_previous_velocity        : register(t13);
Texture2D<float>                      r_previous_depth           : register(t14);
StructuredBuffer<float>               r_previous_auto_exposure   : register(t15);
Texture2D<float>                      r_previous_input_exposure  : register(t16);
Texture2D<float>                      r_auto_exposure_texture    : register(t17);
Texture2D<float3>                     r_rcas_input               : register(t18);
Texture2D<float>                      r_debug_visualization      : register(t19); // Written to when debug visualization is enabled. [R: <Predicted blend factor>, G: <not used>, B: <not used>, A: <not used>]
Texture2D<float3>                     r_result_color             : register(t20); 


RWTexture2D<float3>                   rw_upsampled_color         : register(u0); // Written by Pre-shader used in post shader KPN.
RWTexture2D<float3>                   rw_history_color           : register(u1); // Post Shader Color Output for next frame's history.
RWTexture2D<float3>                   rw_mlsr_output_color       : register(u2); // Color Output texture provided by the game. Final output.
RWTexture2D<DATA_TYPE3>               rw_reprojected_color       : register(u3); // Written by Pre-shader used in post shader KPN.
RWStructuredBuffer<NETWORK_DATA_TYPE> rw_network_input_rfm       : register(u4); // Network inputs. RFM
RWStructuredBuffer<NETWORK_DATA_TYPE> rw_network_input_other     : register(u5); // Network inputs. Other Inputs (may be same resource as rw_network_input_rfm)
RWTexture2D<DATA_TYPE_VECTOR>         rw_recurrent_0             : register(u6); // Network output RFM. Repackaged to text2d to ease reprojection
RWTexture2D<DATA_TYPE_VECTOR>         rw_recurrent_1             : register(u7); // Network output RFM. Repackaged to text2d to ease reprojection
RWTexture2D<float2>                   rw_sr_mv_dilated           : register(u8); // Upscaled / dilated velocity. Written by pre-shader
RWTexture2D<float>                    rw_auto_exposure_texture   : register(u9); // Previous input exposure. Updated by pre-shader
RWTexture2D<DATA_TYPE3>               rw_output_color_for_rcas   : register(u10);
RWTexture2D<float3>                   rw_rcas_output             : register(u11);
RWTexture2D<float>                    rw_debug_visualization     : register(u12); // Written to when debug visualization is enabled. [R: <Predicted blend factor>, G: <not used>, B: <not used>, A: <not used>]


// ML2CODE buffer (spilling here because of pass fusion)
RWByteAddressBuffer ScratchBuffer   : register(u11);
ByteAddressBuffer InitializerBuffer : register(t18);


#if defined(FSR4UPSCALER_BIND_CB_RCAS)
cbuffer cbRCAS : FFX_FSR4UPSCALER_DECLARE_CB(FSR4UPSCALER_BIND_CB_RCAS)
{
    uint4 rcasConfig;

    float preExposure;
    uint3 pad;
};

float PreExposure()
{
    return preExposure;
}
#endif

#if defined(FSR4UPSCALER_BIND_CB_FSR4UPSCALER)
cbuffer MLSR_Optimized_Constants : FFX_FSR4UPSCALER_DECLARE_CB(FSR4UPSCALER_BIND_CB_FSR4UPSCALER)
{
    float2 inv_size;
    float2 scale;
    float2 inv_scale;
    float2 jitter;
    float2 mv_scale;
    float2 tex_size;
    float2 max_renderSize;
    float2 fMotionVectorJitterCancellation;

    uint width;
    uint height;
    uint reset;
    uint width_lr;

    uint height_lr;
    float preExposure;
    float previous_preExposure;
    uint rcas_enabled;

    float rcas_sharpness;
    float pad;
};

float PreExposure()
{
    return preExposure;
}
#endif

#if defined(FSR4UPSCALER_BIND_CB_RCAS) || defined(FSR4UPSCALER_BIND_CB_FSR4UPSCALER)
float LoadExposureValue()
{
#if FFX_MLSR_COLORSPACE == FFX_MLSR_COLORSPACE_LINEAR
    #if FFX_MLSR_AUTOEXPOSURE_ENABLED
            return r_auto_exposure_texture[int2(0, 0)].r / PreExposure();
    #else
            return (r_input_exposure[int2(0, 0)].r == 0.0f ? 1.0 : r_input_exposure[int2(0, 0)].r) / PreExposure(); //if exposure is 0 return 1
    #endif
#else // all other colorspaces
    return 1.0f;
#endif
}
#endif // #if defined(FSR4UPSCALER_BIND_CB_RCAS) || defined(FSR4UPSCALER_BIND_CB_FSR4UPSCALER)
