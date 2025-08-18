#include "../../../api/internal/ffx_util.h"

#include "shader_selector.h"
#include "fsr4_permutations.h"

#ifndef FSR4_ENABLE_DOT4
    #define FSR4_ENABLE_DOT4 0
#endif
#if FSR4_ENABLE_DOT4
#include <fsr4_model_v07_i8_native.h>
#include <fsr4_model_v07_i8_native.cpp>
#include <fsr4_model_v07_i8_quality.h>
#include <fsr4_model_v07_i8_quality.cpp>
#include <fsr4_model_v07_i8_balanced.h>
#include <fsr4_model_v07_i8_balanced.cpp>
#include <fsr4_model_v07_i8_performance.h>
#include <fsr4_model_v07_i8_performance.cpp>
#include <fsr4_model_v07_i8_drs.h>
#include <fsr4_model_v07_i8_drs.cpp>
#include <fsr4_model_v07_i8_ultraperf.h>
#include <fsr4_model_v07_i8_ultraperf.cpp>
#endif

#include <fsr4_model_v07_fp8_no_scale_native.h>
#include <fsr4_model_v07_fp8_no_scale_native.cpp>
#include <fsr4_model_v07_fp8_no_scale_quality.h>
#include <fsr4_model_v07_fp8_no_scale_quality.cpp>
#include <fsr4_model_v07_fp8_no_scale_balanced.h>
#include <fsr4_model_v07_fp8_no_scale_balanced.cpp>
#include <fsr4_model_v07_fp8_no_scale_performance.h>
#include <fsr4_model_v07_fp8_no_scale_performance.cpp>
#include <fsr4_model_v07_fp8_no_scale_drs.h>
#include <fsr4_model_v07_fp8_no_scale_drs.cpp>
#include <fsr4_model_v07_fp8_no_scale_ultraperf.h>
#include <fsr4_model_v07_fp8_no_scale_ultraperf.cpp>

#include <algorithm> // for std::max

#define FOREACH_RESOLUTION(callback, ...) \
    callback(1080, MaxResolution::Res_1920_1080, __VA_ARGS__) \
    callback(2160, MaxResolution::Res_3840_2160, __VA_ARGS__) \
    callback(4320, MaxResolution::Res_7680_4320, __VA_ARGS__)

#define FOREACH_QUALITY(callback, ...) \
    callback(native, Preset::NativeAA, __VA_ARGS__) \
    callback(quality, Preset::Quality, __VA_ARGS__) \
    callback(balanced, Preset::Balanced, __VA_ARGS__) \
    callback(performance, Preset::Performance, __VA_ARGS__) \
    callback(drs, Preset::DRS, __VA_ARGS__) \
    callback(ultraperf, Preset::UltraPerformance, __VA_ARGS__)

#define FOR_1_TO_12(callback, ...) \
    callback(1, __VA_ARGS__) \
    callback(2, __VA_ARGS__) \
    callback(3, __VA_ARGS__) \
    callback(4, __VA_ARGS__) \
    callback(5, __VA_ARGS__) \
    callback(6, __VA_ARGS__) \
    callback(7, __VA_ARGS__) \
    callback(8, __VA_ARGS__) \
    callback(9, __VA_ARGS__) \
    callback(10, __VA_ARGS__) \
    callback(11, __VA_ARGS__) \
    callback(12, __VA_ARGS__)

#define FOR_0_TO_12(callback, ...) \
    callback(0, __VA_ARGS__) \
    callback(1, __VA_ARGS__) \
    callback(2, __VA_ARGS__) \
    callback(3, __VA_ARGS__) \
    callback(4, __VA_ARGS__) \
    callback(5, __VA_ARGS__) \
    callback(6, __VA_ARGS__) \
    callback(7, __VA_ARGS__) \
    callback(8, __VA_ARGS__) \
    callback(9, __VA_ARGS__) \
    callback(10, __VA_ARGS__) \
    callback(11, __VA_ARGS__) \
    callback(12, __VA_ARGS__)

#define ASSIGN_BLOB_HELPER(_info, _tableIdx) POPULATE_SHADER_BLOB_FFX(_info, _tableIdx);

#define ASSIGN_BLOB(_key_index, _table) \
    ASSIGN_BLOB_HELPER(g_ ## _table ## _PermutationInfo, g_ ## _table ## _IndirectionTable[_key_index])

static uint32_t GetColorspace(fsr4_shaders::PermutationOptions options)
{
    if (options.NONLINEAR_COLORSPACE_PQ)
        return 3;

    if (options.NONLINEAR_COLORSPACE_SRGB)
        return 2;

    // generic nonlinear colorspace
    if (options.NONLINEAR_COLORSPACE)
        return 1;

    // no flags = linear.
    return 0;
}

FfxShaderBlob fsr4_shaders::GetPreShaderBlob(uint32_t permutationOption)
{
    PermutationOptions options = fsr4_shaders::PermutationOptionsFromKey(permutationOption);

    if (options.WMMA || !FSR4_ENABLE_DOT4)
    {
        fsr4_model_v07_fp8_no_scale_0_PermutationKey key = {};
        key.FFX_MLSR_DEPTH_INVERTED          = options.DEPTH_INVERTED;
        key.FFX_MLSR_LOW_RES_MV              = options.LOW_RES_MV;
        key.FFX_MLSR_AUTOEXPOSURE_ENABLED    = options.AUTOEXPOSURE_ENABLED;
        key.FFX_MLSR_COLORSPACE              = GetColorspace(options);
        key.FFX_MLSR_JITTERED_MOTION_VECTORS = options.JITTERED_MOTION_VECTORS;
        key.FFX_MLSR_RESOLUTION              = (uint32_t)options.maxRes;
        return ASSIGN_BLOB(key.index, fsr4_model_v07_fp8_no_scale_0);
    }
#if FSR4_ENABLE_DOT4
    else
    {
        #define GENERATOR(_qname, _qenum, ...) \
            case _qenum: \
            { \
                fsr4_model_v07_i8_ ## _qname ## _0_PermutationKey key = {};             \
                key.FFX_MLSR_DEPTH_INVERTED          = options.DEPTH_INVERTED;          \
                key.FFX_MLSR_LOW_RES_MV              = options.LOW_RES_MV;              \
                key.FFX_MLSR_AUTOEXPOSURE_ENABLED    = options.AUTOEXPOSURE_ENABLED;    \
                key.FFX_MLSR_COLORSPACE              = GetColorspace(options);          \
                key.FFX_MLSR_JITTERED_MOTION_VECTORS = options.JITTERED_MOTION_VECTORS; \
                key.FFX_MLSR_RESOLUTION              = (uint32_t)options.maxRes;        \
                return ASSIGN_BLOB(key.index, fsr4_model_v07_i8_ ## _qname ## _0);      \
            }
        
        switch (options.preset)
        {
            FOREACH_QUALITY(GENERATOR)
            default: return {};
        }
        #undef GENERATOR
    }
#endif
}

FfxShaderBlob fsr4_shaders::GetPostShaderBlob(uint32_t permutationOption)
{
    PermutationOptions options = fsr4_shaders::PermutationOptionsFromKey(permutationOption);

    if (options.WMMA || !FSR4_ENABLE_DOT4)
    {
        fsr4_model_v07_fp8_no_scale_13_PermutationKey key = {};
        key.FFX_MLSR_COLORSPACE           = GetColorspace(options);
        key.FFX_MLSR_AUTOEXPOSURE_ENABLED = options.AUTOEXPOSURE_ENABLED;
        key.FFX_DEBUG_VISUALIZE           = options.DEBUG_VISUALIZE;
        key.FFX_MLSR_RESOLUTION           = (uint32_t)options.maxRes;
        return ASSIGN_BLOB(key.index, fsr4_model_v07_fp8_no_scale_13);
    }
#if FSR4_ENABLE_DOT4
    else
    {
        #define GENERATOR(_qname, _qenum, ...) \
            case _qenum: \
            { \
                fsr4_model_v07_i8_ ## _qname ## _13_PermutationKey key = {};        \
                key.FFX_MLSR_COLORSPACE           = GetColorspace(options);         \
                key.FFX_MLSR_AUTOEXPOSURE_ENABLED = options.AUTOEXPOSURE_ENABLED;   \
                key.FFX_DEBUG_VISUALIZE           = options.DEBUG_VISUALIZE;        \
                key.FFX_MLSR_RESOLUTION           = (uint32_t)options.maxRes;       \
                return ASSIGN_BLOB(key.index, fsr4_model_v07_i8_ ## _qname ## _13); \
            }
        
        switch (options.preset)
        {
            FOREACH_QUALITY(GENERATOR)
            default: return {};
        }
        #undef GENERATOR
    }
#endif
}

FfxShaderBlob fsr4_shaders::GetModelShaderBlob(unsigned int pass, uint32_t permutationOption)
{
    PermutationOptions options = fsr4_shaders::PermutationOptionsFromKey(permutationOption);

    if (options.WMMA || !FSR4_ENABLE_DOT4)
    {
        #define GENERATOR_2(_passid, _res) \
            case _passid: \
            { \
                return ASSIGN_BLOB(0, fsr4_model_v07_fp8_no_scale_ ## _res ## _ ## _passid); \
            }

        #define GENERATOR_1(_res, _resenum, ...) case _resenum: { switch (pass) { FOR_1_TO_12(GENERATOR_2, _res) } }

        switch (options.maxRes)
        {
            FOREACH_RESOLUTION(GENERATOR_1)
            default: return {};
        }

        #undef GENERATOR_1
        #undef GENERATOR_2
    }
#if FSR4_ENABLE_DOT4
    else
    {
        #define GENERATOR_3(_passid, _qname_res) \
            case _passid: \
            { \
                return ASSIGN_BLOB(0, fsr4_model_v07_i8_ ## _qname_res ## _ ## _passid); \
            }

        #define GENERATOR_2(_qname, _qenum, _res) case _qenum: { switch (pass) { FOR_1_TO_12(GENERATOR_3, _qname ## _ ## _res) } }

        #define GENERATOR_1(_res, _resenum, ...) case _resenum: { switch (options.preset) { FOREACH_QUALITY(GENERATOR_2, _res) } }

        switch (options.maxRes)
        {
            FOREACH_RESOLUTION(GENERATOR_1)
            default: return {};
        }

        #undef GENERATOR_1
        #undef GENERATOR_2
        #undef GENERATOR_3
    }
#endif
}

FfxShaderBlob fsr4_shaders::GetPaddingResetBlob(unsigned int pass, uint32_t permutationOption)
{
    PermutationOptions options = fsr4_shaders::PermutationOptionsFromKey(permutationOption);

    if (options.WMMA || !FSR4_ENABLE_DOT4)
    {
    #define GENERATOR_2(_passid, _res)                                             \
        case _passid:                                                              \
        {                                                                          \
            return ASSIGN_BLOB(0, fsr4_model_v07_fp8_no_scale_##_res##_##_passid##_post); \
        }

    #define GENERATOR_1(_res, _resenum, ...)   \
        case _resenum:                         \
        {                                      \
            switch (pass)                      \
            {                                  \
                FOR_0_TO_12(GENERATOR_2, _res) \
            }                                  \
        }

            switch (options.maxRes)
            {
                FOREACH_RESOLUTION(GENERATOR_1)
            default:
                return {};
            }

    #undef GENERATOR_1
    #undef GENERATOR_2
    }

    return {};
}

FfxShaderBlob fsr4_shaders::GetSPDAutoExposureBlob()
{
    return ASSIGN_BLOB(0, spd_auto_exposure);
}

FfxShaderBlob fsr4_shaders::GetDebugViewBlob(uint32_t permutationOption)
{
    PermutationOptions options              = fsr4_shaders::PermutationOptionsFromKey(permutationOption);
    debug_view_PermutationKey key           = {};
    key.FFX_MLSR_JITTERED_MOTION_VECTORS    = options.JITTERED_MOTION_VECTORS;
    return ASSIGN_BLOB(key.index, debug_view);
}

FfxShaderBlob fsr4_shaders::GetRcasBlob(uint32_t permutationOption)
{
    PermutationOptions options = fsr4_shaders::PermutationOptionsFromKey(permutationOption);
    rcas_PermutationKey key = {};
    key.FFX_MLSR_AUTOEXPOSURE_ENABLED = options.AUTOEXPOSURE_ENABLED;
    key.FFX_MLSR_COLORSPACE           = GetColorspace(options);
    return ASSIGN_BLOB(key.index, rcas);
}

void fsr4_shaders::GetInitializer(Preset quality, bool is_wmma, void*& outBlobPointer, size_t& outBlobSize)
{
    if (is_wmma || !FSR4_ENABLE_DOT4)
    {
        #define GENERATOR(_qname, _qenum, ...) case _qenum: { outBlobSize = g_fsr4_model_v07_fp8_no_scale_ ## _qname ## _initializers_size; outBlobPointer = (void*)g_fsr4_model_v07_fp8_no_scale_ ## _qname ## _initializers_data; break; }
        switch (quality)
        {
            FOREACH_QUALITY(GENERATOR)
        }
        #undef GENERATOR
    }
#if FSR4_ENABLE_DOT4
    else
    {
        #define GENERATOR(_qname, _qenum, ...) case _qenum: { outBlobSize = g_fsr4_model_v07_i8_ ## _qname ## _initializers_size; outBlobPointer = (void*)g_fsr4_model_v07_i8_ ## _qname ## _initializers_data; break; }
        switch (quality)
        {
            FOREACH_QUALITY(GENERATOR)
        }
        #undef GENERATOR
    }
#endif
}

size_t fsr4_shaders::GetInitializerSize()
{
    // get maximum size required for any init blob.
    size_t blobSize = 0;

    #define GENERATOR(_qname, _qenum, ...) blobSize = std::max(g_fsr4_model_v07_fp8_no_scale_ ## _qname ## _initializers_size, blobSize);
    FOREACH_QUALITY(GENERATOR)
    #undef GENERATOR
#if FSR4_ENABLE_DOT4
    #define GENERATOR(_qname, _qenum, ...) blobSize = std::max(g_fsr4_model_v07_i8_ ## _qname ## _initializers_size, blobSize);
    FOREACH_QUALITY(GENERATOR)
    #undef GENERATOR
#endif

    return blobSize;
}

size_t fsr4_shaders::GetScratchSize(MaxResolution resolution)
{
    // must match the ML2Code scratch for the resolution
    // TODO: automate this in both ML2Code and here
    switch (resolution)
    {
    case (MaxResolution::Res_1920_1080):
        return 20880256;
    case (MaxResolution::Res_3840_2160):
        return 83232256;
    case (MaxResolution::Res_7680_4320):
    default:
        return 332352256;
    }
}
