#include "utest.h"
#include "../../api/include/ffx_api.h"
#include "../include/ffx_upscale.h"

UTEST(ffx_api_dtypes, dtypes_stable) {
  // tests that the dtype constants can be referenced and do not change.
  EXPECT_EQ(FFX_API_EFFECT_ID_UPSCALE,                                          UINT64_C(0x10000));
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,                           UINT64_C(0x10000));
  EXPECT_EQ(FFX_API_DISPATCH_DESC_TYPE_UPSCALE,                                 UINT64_C(0x10001));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE,     UINT64_C(0x10002));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE, UINT64_C(0x10003));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT,                UINT64_C(0x10004));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET,                    UINT64_C(0x10005));
  EXPECT_EQ(FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK,            UINT64_C(0x10006));
  EXPECT_EQ(FFX_API_CONFIGURE_DESC_TYPE_UPSCALE_KEYVALUE,                       UINT64_C(0x10007));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE,                   UINT64_C(0x10008));
}


UTEST(ffx_api_upscale_abi_size, ffxCreateContextDescUpscale) {
  EXPECT_EQ(sizeof(struct ffxCreateContextDescUpscale) , 48);
}

UTEST(ffx_api_upscale_abi_offsets, ffxCreateContextDescUpscale) {
  EXPECT_EQ(offsetof(struct ffxCreateContextDescUpscale, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescUpscale, flags) , 16);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescUpscale, maxRenderSize) , 20);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescUpscale, maxUpscaleSize) , 28);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescUpscale, fpMessage) , 40);
}

UTEST(ffx_api_upscale_abi_size, ffxDispatchDescUpscale) {
  EXPECT_EQ(sizeof(struct ffxDispatchDescUpscale) , 432);
}

UTEST(ffx_api_upscale_abi_offsets, ffxDispatchDescUpscale) {
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, header) , 0);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, commandList) , 16);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, color) , 24);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, depth) , 72);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, motionVectors) , 120);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, exposure) , 168);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, reactive) , 216);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, transparencyAndComposition) , 264);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, output) , 312);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, jitterOffset) , 360);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, motionVectorScale) , 368);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, renderSize) , 376);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, upscaleSize) , 384);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, enableSharpening) , 392);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, sharpness) , 396);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, frameTimeDelta) , 400);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, preExposure) , 404);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, reset) , 408);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, cameraNear) , 412);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, cameraFar) , 416);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, cameraFovAngleVertical) , 420);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, viewSpaceToMetersFactor) , 424);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscale, flags) , 428);
}

UTEST(ffx_api_upscale_abi_size, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode) {
  EXPECT_EQ(sizeof(struct ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode) , 32);
}

UTEST(ffx_api_upscale_abi_offsets, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode) {
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode, qualityMode) , 16);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode, pOutUpscaleRatio) , 24);
}

UTEST(ffx_api_upscale_abi_size, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode) {
  EXPECT_EQ(sizeof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode) , 48);
}

UTEST(ffx_api_upscale_abi_offsets, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode) {
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode, displayWidth) , 16);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode, displayHeight) , 20);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode, qualityMode) , 24);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode, pOutRenderWidth) , 32);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode, pOutRenderHeight) , 40);
}

UTEST(ffx_api_upscale_abi_size, ffxQueryDescUpscaleGetJitterPhaseCount) {
  EXPECT_EQ(sizeof(struct ffxQueryDescUpscaleGetJitterPhaseCount) , 32);
}

UTEST(ffx_api_upscale_abi_offsets, ffxQueryDescUpscaleGetJitterPhaseCount) {
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterPhaseCount, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterPhaseCount, renderWidth) , 16);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterPhaseCount, displayWidth) , 20);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterPhaseCount, pOutPhaseCount) , 24);
}


UTEST(ffx_api_upscale_abi_size, ffxQueryDescUpscaleGetJitterOffset) {
  EXPECT_EQ(sizeof(struct ffxQueryDescUpscaleGetJitterOffset) , 40);
}

UTEST(ffx_api_upscale_abi_offsets, ffxQueryDescUpscaleGetJitterOffset) {
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterOffset, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterOffset, index) , 16);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterOffset, phaseCount) , 20);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterOffset, pOutX) , 24);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetJitterOffset, pOutY) , 32);
}


UTEST(ffx_api_upscale_abi_size, ffxDispatchDescUpscaleGenerateReactiveMask) {
  EXPECT_EQ(sizeof(struct ffxDispatchDescUpscaleGenerateReactiveMask) , 192);
}

UTEST(ffx_api_upscale_abi_offsets, ffxDispatchDescUpscaleGenerateReactiveMask) {
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, header) , 0);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, commandList) , 16);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, colorOpaqueOnly) , 24);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, colorPreUpscale) , 72);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, outReactive) , 120);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, renderSize) , 168);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, scale) , 176);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, cutoffThreshold) , 180);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, binaryValue) , 184);
  EXPECT_EQ(offsetof(struct ffxDispatchDescUpscaleGenerateReactiveMask, flags) , 188);
}


UTEST(ffx_api_upscale_abi_size, ffxConfigureDescUpscaleKeyValue) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescUpscaleKeyValue) , 40);
}

UTEST(ffx_api_upscale_abi_offsets, ffxConfigureDescUpscaleKeyValue) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescUpscaleKeyValue, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescUpscaleKeyValue, key) , 16);
  EXPECT_EQ(offsetof(struct ffxConfigureDescUpscaleKeyValue, u64) , 24);
  EXPECT_EQ(offsetof(struct ffxConfigureDescUpscaleKeyValue, ptr) , 32);
}

UTEST(ffx_api_upscale_abi_size, ffxQueryDescUpscaleGetGPUMemoryUsage) {
  EXPECT_EQ(sizeof(struct ffxQueryDescUpscaleGetGPUMemoryUsage) , 24);
}

UTEST(ffx_api_upscale_abi_offsets, ffxQueryDescUpscaleGetGPUMemoryUsage) {
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetGPUMemoryUsage, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescUpscaleGetGPUMemoryUsage, gpuMemoryUsageUpscaler) , 16); 
}

UTEST(ffx_api_upscale_enums, FfxApiUpscaleQualityMode) {
  EXPECT_EQ(FFX_UPSCALE_QUALITY_MODE_NATIVEAA,          0);
  EXPECT_EQ(FFX_UPSCALE_QUALITY_MODE_QUALITY,           1);
  EXPECT_EQ(FFX_UPSCALE_QUALITY_MODE_BALANCED,          2);
  EXPECT_EQ(FFX_UPSCALE_QUALITY_MODE_PERFORMANCE,       3);
  EXPECT_EQ(FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE, 4);
}

UTEST(ffx_api_upscale_enums, FfxApiCreateContextUpscaleFlags) {
  EXPECT_EQ(FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE,                 1 << 0);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS,  1 << 1);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION, 1 << 2);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_DEPTH_INVERTED,                     1 << 3);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_DEPTH_INFINITE,                     1 << 4);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_AUTO_EXPOSURE,                      1 << 5);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_DYNAMIC_RESOLUTION,                 1 << 6);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_DEBUG_CHECKING,                     1 << 7);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE,              1 << 8);
  EXPECT_EQ(FFX_UPSCALE_ENABLE_DEBUG_VISUALIZATION,                1 << 9);
}

UTEST(ffx_api_upscale_enums, FfxApiDispatchFsrUpscaleFlags) {
  EXPECT_EQ(FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW,       1 << 0);
  EXPECT_EQ(FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB, 1 << 1);
  EXPECT_EQ(FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_PQ,   1 << 2);
}

UTEST(ffx_api_upscale_enums, FfxApiDispatchUpscaleAutoreactiveFlags) {
  EXPECT_EQ(FFX_UPSCALE_AUTOREACTIVEFLAGS_APPLY_TONEMAP,        1 << 0);
  EXPECT_EQ(FFX_UPSCALE_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP, 1 << 1);
  EXPECT_EQ(FFX_UPSCALE_AUTOREACTIVEFLAGS_APPLY_THRESHOLD,      1 << 2);
  EXPECT_EQ(FFX_UPSCALE_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX,   1 << 3);
}

UTEST(ffx_api_upscale_enums, FfxApiConfigureUpscaleKey) {
  EXPECT_EQ(FFX_API_CONFIGURE_UPSCALE_KEY_FVELOCITYFACTOR,              0);
  EXPECT_EQ(FFX_API_CONFIGURE_UPSCALE_KEY_FREACTIVENESSSCALE,           1);
  EXPECT_EQ(FFX_API_CONFIGURE_UPSCALE_KEY_FSHADINGCHANGESCALE,          2);
  EXPECT_EQ(FFX_API_CONFIGURE_UPSCALE_KEY_FACCUMULATIONADDEDPERFRAME,   3);
  EXPECT_EQ(FFX_API_CONFIGURE_UPSCALE_KEY_FMINDISOCCLUSIONACCUMULATION, 4);
}

UTEST_MAIN()
