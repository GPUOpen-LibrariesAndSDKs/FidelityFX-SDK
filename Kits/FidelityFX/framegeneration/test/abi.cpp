#include "utest.h"
#include "../../api/include/ffx_api.h"
#include "../include/ffx_framegeneration.h"

UTEST(ffx_api_dtypes, dtypes_stable) {
  // tests that the dtype constants can be referenced and do not change.
  EXPECT_EQ(FFX_API_EFFECT_ID_FRAMEGENERATION,                                      UINT64_C(0x20000));
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION,                       UINT64_C(0x20001));
  EXPECT_EQ(FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION,                            UINT64_C(0x20002));
  EXPECT_EQ(FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION,                             UINT64_C(0x20003));
  EXPECT_EQ(FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE,                     UINT64_C(0x20004));
  EXPECT_EQ(FFX_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT,                     UINT64_C(0x20005));
  EXPECT_EQ(FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_KEYVALUE,                   UINT64_C(0x20006));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE,               UINT64_C(0x20007));
  EXPECT_EQ(FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE, UINT64_C(0x20008));
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_HUDLESS,               UINT64_C(0x20009));
  EXPECT_EQ(FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_CAMERAINFO,          UINT64_C(0x2000a));
}

UTEST(ffx_api_framegen_types_abi_size, FfxApiSwapchainFramePacingTuning) {
  EXPECT_EQ(sizeof(struct FfxApiSwapchainFramePacingTuning) , 20);
}

UTEST(ffx_api_framegen_types_abi_offsets, FfxApiSwapchainFramePacingTuning) {
  EXPECT_EQ(offsetof(struct FfxApiSwapchainFramePacingTuning, safetyMarginInMs) , 0);
  EXPECT_EQ(offsetof(struct FfxApiSwapchainFramePacingTuning, varianceFactor) , 4);
  EXPECT_EQ(offsetof(struct FfxApiSwapchainFramePacingTuning, allowHybridSpin) , 8);
  EXPECT_EQ(offsetof(struct FfxApiSwapchainFramePacingTuning, hybridSpinTime) , 12);
  EXPECT_EQ(offsetof(struct FfxApiSwapchainFramePacingTuning, allowWaitForSingleObjectOnFence) , 16);
}

UTEST(ffx_api_framegen_abi_size, ffxCreateContextDescFrameGeneration) {
  EXPECT_EQ(sizeof(struct ffxCreateContextDescFrameGeneration) , 40);
}

UTEST(ffx_api_framegen_abi_offsets, ffxCreateContextDescFrameGeneration) {
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGeneration, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGeneration, flags) , 16);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGeneration, displaySize) , 20);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGeneration, maxRenderSize) , 28);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGeneration, backBufferFormat) , 36);
}

UTEST(ffx_api_framegen_abi_size, ffxCreateContextDescFrameGenerationHudless) {
  EXPECT_EQ(sizeof(struct ffxCreateContextDescFrameGenerationHudless) , 24);
}

UTEST(ffx_api_framegen_abi_offsets, ffxCreateContextDescFrameGenerationHudless) {
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationHudless, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationHudless, hudlessBackBufferFormat) , 16);
}

UTEST(ffx_api_framegen_abi_size, ffxCallbackDescFrameGenerationPresent) {
  EXPECT_EQ(sizeof(struct ffxCallbackDescFrameGenerationPresent) , 192);
}

UTEST(ffx_api_framegen_abi_offsets, ffxCallbackDescFrameGenerationPresent) {
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, device) , 16);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, commandList) , 24);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, currentBackBuffer) , 32);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, currentUI) , 80);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, outputSwapChainBuffer) , 128);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, isGeneratedFrame) , 176);
  EXPECT_EQ(offsetof(struct ffxCallbackDescFrameGenerationPresent, frameID) , 184);
}

UTEST(ffx_api_framegen_abi_size, FfxApiPresentCallbackFunc) {
  EXPECT_EQ(sizeof(FfxApiPresentCallbackFunc) , 8);
}

UTEST(ffx_api_framegen_abi_size, FfxApiFrameGenerationDispatchFunc) {
  EXPECT_EQ(sizeof(FfxApiFrameGenerationDispatchFunc) , 8);
}


UTEST(ffx_api_framegen_abi_size, ffxConfigureDescFrameGeneration) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescFrameGeneration) , 144);
}

UTEST(ffx_api_framegen_abi_offsets, ffxConfigureDescFrameGeneration) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, swapChain) , 16);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, presentCallback) , 24);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, presentCallbackUserContext) , 32);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, frameGenerationCallback) , 40);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, frameGenerationCallbackUserContext) , 48);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, frameGenerationEnabled) , 56);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, allowAsyncWorkloads) , 57);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, HUDLessColor) , 64);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, flags) , 112);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, onlyPresentGenerated) , 116);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, generationRect) , 120);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGeneration, frameID) , 136);
}

UTEST(ffx_api_framegen_abi_size, ffxDispatchDescFrameGeneration) {
  EXPECT_EQ(sizeof(struct ffxDispatchDescFrameGeneration), 312);
}

UTEST(ffx_api_framegen_abi_offsets, ffxDispatchDescFrameGeneration) {
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, header), 0);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, commandList), 16);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, presentColor), 24);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, outputs), 72);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, numGeneratedFrames), 264);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, reset), 268);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, backbufferTransferFunction), 272);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, minMaxLuminance), 276);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, generationRect), 284);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGeneration, frameID), 304);
}

UTEST(ffx_api_framegen_abi_size, ffxDispatchDescFrameGenerationPrepare) {
  EXPECT_EQ(sizeof(struct ffxDispatchDescFrameGenerationPrepare), 184);
}

UTEST(ffx_api_framegen_abi_offsets, ffxDispatchDescFrameGenerationPrepare) {
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, header) , 0);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, frameID) , 16);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, flags) , 24);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, commandList) , 32);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, renderSize) , 40);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, jitterOffset) , 48);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, motionVectorScale) , 56);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, frameTimeDelta) , 64);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, unused_reset) , 68);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, cameraNear) , 72);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, cameraFar) , 76);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, cameraFovAngleVertical) , 80);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, viewSpaceToMetersFactor) , 84);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, depth) , 88);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepare, motionVectors) , 136);
}

UTEST(ffx_api_framegen_abi_size, ffxConfigureDescFrameGenerationKeyValue) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescFrameGenerationKeyValue) , 40);
}

UTEST(ffx_api_framegen_abi_offsets, ffxConfigureDescFrameGenerationKeyValue) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationKeyValue, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationKeyValue, key) , 16);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationKeyValue, u64) , 24);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationKeyValue, ptr) , 32);
}

UTEST(ffx_api_framegen_abi_size, ffxQueryDescFrameGenerationGetGPUMemoryUsage) {
  EXPECT_EQ(sizeof(struct ffxQueryDescFrameGenerationGetGPUMemoryUsage) , 24);
}

UTEST(ffx_api_framegen_abi_offsets, ffxQueryDescFrameGenerationGetGPUMemoryUsage) {
  EXPECT_EQ(offsetof(struct ffxQueryDescFrameGenerationGetGPUMemoryUsage, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescFrameGenerationGetGPUMemoryUsage, gpuMemoryUsageFrameGeneration) , 16); 
}

UTEST(ffx_api_framegen_abi_size, ffxConfigureDescFrameGenerationRegisterDistortionFieldResource) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescFrameGenerationRegisterDistortionFieldResource) , 64);
}

UTEST(ffx_api_framegen_abi_offsets, ffxConfigureDescFrameGenerationRegisterDistortionFieldResource) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationRegisterDistortionFieldResource, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationRegisterDistortionFieldResource, distortionField) , 16); 
}

UTEST(ffx_api_framegen_abi_size, ffxDispatchDescFrameGenerationPrepareCameraInfo) {
  EXPECT_EQ(sizeof(struct ffxDispatchDescFrameGenerationPrepareCameraInfo), 64);
}

UTEST(ffx_api_framegen_abi_offsets, ffxDispatchDescFrameGenerationPrepareCameraInfo) {
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepareCameraInfo, header), 0);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepareCameraInfo, cameraPosition), 16);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepareCameraInfo, cameraUp), 28);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepareCameraInfo, cameraRight), 40);
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationPrepareCameraInfo, cameraForward), 52);
}

UTEST(ffx_api_framegen_enums, FfxApiCreateContextFramegenerationFlags) {
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT,             1 << 0);
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS,  1 << 1);
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION, 1 << 2);
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED,                     1 << 3);
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE,                     1 << 4);
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE,                 1 << 5);
  EXPECT_EQ(FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING,                     1 << 6);
}

UTEST(ffx_api_framegen_enums, FfxApiDispatchFramegenerationFlags) {
  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES,       1 << 0);
  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS, 1 << 1);
  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW,             1 << 2);
  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY, 1 << 3);
  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES,     1 << 4);

  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_RESERVED_1,                  1 << 5);
  EXPECT_EQ(FFX_FRAMEGENERATION_FLAG_RESERVED_2,                  1 << 6);
}

UTEST(ffx_api_framegen_enums, FfxApiUiCompositionFlags) {
  EXPECT_EQ(FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_USE_PREMUL_ALPHA,                    1 << 0);
  EXPECT_EQ(FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING, 1 << 1);
}

UTEST_MAIN()
