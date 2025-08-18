// These test functionality via the API that can be invoked with a null context.
// Internally, a context provider is queried, which is slightly slower than when a context is provided straight out.


#include <windows.h>
#include "utest.h"
#include "../../api/include/ffx_api.h" 
#include "../../api/include/ffx_api_loader.h" 
#include "../include/ffx_upscale.h" 

HMODULE g_dllUnderTest = {};
ffxFunctions funcs = {};

#define NUM_VERSIONS 3

uint64_t versions[NUM_VERSIONS] = {0};
const char *versionNames[NUM_VERSIONS] = {0};

UTEST(env, build_64b) {
  EXPECT_EQ(sizeof(void*) , 8);
}

UTEST(setup, enough_versions) {
  // make sure we don't have more versions than we can test. If this fails, increase NUM_VERSIONS.
  ffxQueryDescGetVersions versionQuery{};
  versionQuery.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  uint64_t numVersions = 0;
  versionQuery.outputCount = &numVersions;
  funcs.Query(nullptr, &versionQuery.header);
  ASSERT_LE(numVersions, NUM_VERSIONS);
}

UTEST(setup, g_dllUnderTest) {
  ASSERT_TRUE(g_dllUnderTest != nullptr);
}

struct ffx_api_upscale_no_ctx {
  ffxOverrideVersion versionOverride;
};

UTEST_I_SETUP(ffx_api_upscale_no_ctx) {
  utest_fixture->versionOverride.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
  utest_fixture->versionOverride.versionId = versions[utest_index];

  if (versions[utest_index] == 0) {
    UTEST_SKIP("Not this many versions.");
  }
}

UTEST_I_TEARDOWN(ffx_api_upscale_no_ctx) {
  (void)utest_fixture;
  if (utest_result && *utest_result == UTEST_TEST_FAILURE) {
    UTEST_PRINTF("Version number: %llx, version name: %s\n", versions[utest_index], versionNames[utest_index]);
  }
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode_nativeAA, NUM_VERSIONS) {
  float output = 0.0f;

  if ((utest_fixture->versionOverride.versionId & 0x7FFFFFFFu) < (3u << 22u)) {
    UTEST_SKIP("NativeAA not supported on 2.x");
  }

  ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_NATIVEAA;
  desc.pOutUpscaleRatio = &output;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(output != 0.0f);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode_quality, NUM_VERSIONS) {
  float output = 0.0f;
  ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_QUALITY;
  desc.pOutUpscaleRatio = &output;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(output != 0.0f);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode_balanced, NUM_VERSIONS) {
  float output = 0.0f;
  ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_BALANCED;
  desc.pOutUpscaleRatio = &output;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(output != 0.0f);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode_performance, NUM_VERSIONS) {
  float output = 0.0f;
  ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_PERFORMANCE;
  desc.pOutUpscaleRatio = &output;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(output != 0.0f);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode_ultraperformance, NUM_VERSIONS) {
  float output = 0.0f;
  ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE;
  desc.pOutUpscaleRatio = &output;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(output != 0.0f);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode_ultraperformance, NUM_VERSIONS) {
  uint32_t input_x = 1920;
  uint32_t input_y = 1080;
  uint32_t output_x = input_x;
  uint32_t output_y = input_y;
  ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE;
  desc.displayWidth = input_x;
  desc.displayHeight = input_y;
  desc.pOutRenderWidth = &output_x;
  desc.pOutRenderHeight = &output_y;

  funcs.Query(nullptr, &desc.header);

  ASSERT_LT(output_x, input_x);
  ASSERT_LT(output_y, input_y);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode_performance, NUM_VERSIONS) {
  uint32_t input_x = 1920;
  uint32_t input_y = 1080;
  uint32_t output_x = input_x;
  uint32_t output_y = input_y;
  ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_PERFORMANCE;
  desc.displayWidth = input_x;
  desc.displayHeight = input_y;
  desc.pOutRenderWidth = &output_x;
  desc.pOutRenderHeight = &output_y;

  funcs.Query(nullptr, &desc.header);

  ASSERT_LT(output_x, input_x);
  ASSERT_LT(output_y, input_y);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode_balanced, NUM_VERSIONS) {
  uint32_t input_x = 1920;
  uint32_t input_y = 1080;
  uint32_t output_x = input_x;
  uint32_t output_y = input_y;
  ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_BALANCED;
  desc.displayWidth = input_x;
  desc.displayHeight = input_y;
  desc.pOutRenderWidth = &output_x;
  desc.pOutRenderHeight = &output_y;

  funcs.Query(nullptr, &desc.header);

  ASSERT_LT(output_x, input_x);
  ASSERT_LT(output_y, input_y);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode_quality, NUM_VERSIONS) {
  uint32_t input_x = 1920;
  uint32_t input_y = 1080;
  uint32_t output_x = input_x;
  uint32_t output_y = input_y;
  ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_QUALITY;
  desc.displayWidth = input_x;
  desc.displayHeight = input_y;
  desc.pOutRenderWidth = &output_x;
  desc.pOutRenderHeight = &output_y;

  funcs.Query(nullptr, &desc.header);

  ASSERT_LT(output_x, input_x);
  ASSERT_LT(output_y, input_y);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetRenderResolutionFromQualityMode_nativeaa, NUM_VERSIONS) {
  if ((utest_fixture->versionOverride.versionId & 0x7FFFFFFFu) < (3u << 22u)) {
    UTEST_SKIP("NativeAA not supported on 2.x");
  }

  uint32_t input_x = 1920;
  uint32_t input_y = 1080;
  uint32_t output_x = input_x;
  uint32_t output_y = input_y;
  ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.qualityMode = FFX_UPSCALE_QUALITY_MODE_NATIVEAA;
  desc.displayWidth = input_x;
  desc.displayHeight = input_y;
  desc.pOutRenderWidth = &output_x;
  desc.pOutRenderHeight = &output_y;

  funcs.Query(nullptr, &desc.header);

  ASSERT_EQ(output_x, input_x);
  ASSERT_EQ(output_y, input_y);
}

UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetJitterPhaseCount, NUM_VERSIONS) {

  int32_t output = 0u;
  
  ffxQueryDescUpscaleGetJitterPhaseCount desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.renderWidth = 1080;
  desc.displayWidth = 1440;
  desc.pOutPhaseCount = &output; 

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(output != 0u);
}
 
UTEST_I(ffx_api_upscale_no_ctx, ffxQueryDescUpscaleGetJitterOffset, NUM_VERSIONS) {
  float output_x = -5.0f;
  float output_y = -5.0f;
  float output_x2 = -5.0f;
  float output_y3 = -5.0f;
  float output_x3 = -5.0f;
  float output_y2 = -5.0f;
  uint32_t index = 1;
  uint32_t phaseCount = 10;
  
  ffxQueryDescUpscaleGetJitterOffset desc = {};
  desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET;
  desc.header.pNext = &utest_fixture->versionOverride.header;
  desc.index = index;
  desc.phaseCount = phaseCount;
  desc.pOutX = &output_x;
  desc.pOutY = &output_y;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(-5.0f != output_x);
  ASSERT_TRUE(-5.0f != output_y);
  
  desc.index = index+1;
  desc.phaseCount = phaseCount;
  desc.pOutX = &output_x2;
  desc.pOutY = &output_y2;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(-5.0f != output_x2 && output_x != output_x2);
  ASSERT_TRUE(-5.0f != output_y2 && output_y != output_y2);
  
  desc.index = index+2;
  desc.phaseCount = phaseCount;
  desc.pOutX = &output_x3;
  desc.pOutY = &output_y3;

  funcs.Query(nullptr, &desc.header);

  ASSERT_TRUE(-5.0f != output_x3 && output_x2 != output_x3);
  ASSERT_TRUE(-5.0f != output_y3 && output_y2 != output_y3);
}

UTEST_STATE();

int main(int argc, const char *const argv[]) {
  
  g_dllUnderTest = LoadLibraryExA("amd_fidelityfx_upscaler_dx12.dll", NULL, 0);

  ffxLoadFunctions(&funcs, g_dllUnderTest);

  ffxQueryDescGetVersions versionQuery{};
  versionQuery.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  uint64_t numVersions = NUM_VERSIONS;
  versionQuery.outputCount = &numVersions;
  versionQuery.versionIds = versions;
  versionQuery.versionNames = versionNames;
  funcs.Query(nullptr, &versionQuery.header);
  
  return utest_main(argc, argv);
}
