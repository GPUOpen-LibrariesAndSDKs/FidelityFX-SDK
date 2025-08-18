#pragma once

#include <array>

struct FfxShaderBlob;

namespace fsr4_shaders
{

enum class Preset : uint32_t
{
    NativeAA,
    Quality,
    Balanced,
    Performance,
    DRS,
    UltraPerformance
};

const uint32_t qualityPresetBitShift = 3;

enum class MaxResolution : uint32_t
{
    Res_1920_1080,
    Res_3840_2160,
    Res_7680_4320,
};

const uint32_t maxResBitShift = 2;

static uint32_t PermutationOptionsToKey(
    Preset preset,
    MaxResolution maxRes,
    bool WMMA,
    bool DEPTH_INVERTED,
    bool LOW_RES_MV,
    bool AUTOEXPOSURE_ENABLED,
    bool JITTERED_MOTION_VECTORS,
    bool NONLINEAR_COLORSPACE,
    bool NONLINEAR_COLORSPACE_SRGB,
    bool NONLINEAR_COLORSPACE_PQ,
    bool DEBUG_VISUALIZE)
{
    uint32_t key = 0;
    if (DEBUG_VISUALIZE) key |= 1;
    key <<= 1;
    if (NONLINEAR_COLORSPACE_PQ) key |= 1;
    key <<= 1;
    if (NONLINEAR_COLORSPACE_SRGB) key |= 1;
    key <<= 1;
    if (NONLINEAR_COLORSPACE) key |= 1;
    key <<= 1;
    if (JITTERED_MOTION_VECTORS) key |= 1;
    key <<= 1;
    if (AUTOEXPOSURE_ENABLED) key |= 1;
    key <<= 1;
    if (LOW_RES_MV) key |= 1;
    key <<= 1;
    if (DEPTH_INVERTED) key |= 1;
    key <<= 1;
    if (WMMA) key |= 1;
    key <<= maxResBitShift;
    key |= (uint32_t)maxRes;
    key <<= qualityPresetBitShift;
    key |= (uint32_t)preset;
    return key;
}

struct PermutationOptions
{
    Preset preset;
    MaxResolution maxRes;
    bool WMMA;
    bool DEPTH_INVERTED;
    bool LOW_RES_MV;
    bool AUTOEXPOSURE_ENABLED;
    bool JITTERED_MOTION_VECTORS;
    bool NONLINEAR_COLORSPACE;
    bool NONLINEAR_COLORSPACE_SRGB;
    bool NONLINEAR_COLORSPACE_PQ;
    bool DEBUG_VISUALIZE;
};

static PermutationOptions PermutationOptionsFromKey(uint32_t key)
{
    PermutationOptions options = {};
    options.preset = (Preset)(key & ((1u << qualityPresetBitShift) - 1));
    key >>= qualityPresetBitShift;
    options.maxRes = (MaxResolution)(key & ((1u << maxResBitShift) - 1));
    key >>= maxResBitShift;
    options.WMMA = (key & 1) != 0;
    key >>= 1;
    options.DEPTH_INVERTED = (key & 1) != 0;
    key >>= 1;
    options.LOW_RES_MV = (key & 1) != 0;
    key >>= 1;
    options.AUTOEXPOSURE_ENABLED = (key & 1) != 0;
    key >>= 1;
    options.JITTERED_MOTION_VECTORS = (key & 1) != 0;
    key >>= 1;
    options.NONLINEAR_COLORSPACE = (key & 1) != 0;
    key >>= 1;
    options.NONLINEAR_COLORSPACE_SRGB = (key & 1) != 0;
    key >>= 1;
    options.NONLINEAR_COLORSPACE_PQ = (key & 1) != 0;
    key >>= 1;
    options.DEBUG_VISUALIZE = (key & 1) != 0;
    return options;
}

FfxShaderBlob GetPreShaderBlob(uint32_t permutationOption);
FfxShaderBlob GetPostShaderBlob(uint32_t permutationOption);
FfxShaderBlob GetSPDAutoExposureBlob();

FfxShaderBlob GetModelShaderBlob(unsigned int pass, uint32_t permutationOption);
FfxShaderBlob GetPaddingResetBlob(unsigned int pass, uint32_t permutationOption);

FfxShaderBlob GetDebugViewBlob(uint32_t permutationOption);

FfxShaderBlob GetRcasBlob(uint32_t permutationOption);

void GetInitializer(Preset qualit, bool is_wmma, void*& outBlobPointer, size_t& outBlobSize);
size_t GetInitializerSize();

struct DispatchSize { uint32_t x, y, z; };

size_t GetScratchSize(MaxResolution resolution);

} // namespace fsr4_shaders
