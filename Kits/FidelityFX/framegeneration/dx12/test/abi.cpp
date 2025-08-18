#include "../../../api/include/ffx_api.h"
#include "../../include/dx12/ffx_api_framegeneration_dx12.h"
#include "utest.h"

UTEST(ffx_api_dtypes, dtypes_stable) {
  // tests that the dtype constants can be referenced and do not change.
  EXPECT_EQ(FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_DX12,                                    UINT64_C(0x30000));
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12,                UINT64_C(0x30001));
  EXPECT_EQ(FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12,       UINT64_C(0x30002));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12,     UINT64_C(0x30003));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12,         UINT64_C(0x30004));
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12,                 UINT64_C(0x30005));
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12,            UINT64_C(0x30006));
  EXPECT_EQ(FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12,         UINT64_C(0x30007));
  EXPECT_EQ(FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX12, 				UINT64_C(0x30008));
  EXPECT_EQ(FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12,             UINT64_C(0x30009));
}

UTEST(ffx_api_framegen_abi_size, ffxCreateContextDescFrameGenerationSwapChainWrapDX12) {
  EXPECT_EQ(sizeof(struct ffxCreateContextDescFrameGenerationSwapChainWrapDX12) , 32);
}

UTEST(ffx_api_framegen_abi_offsets, ffxCreateContextDescFrameGenerationSwapChainWrapDX12) {
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainWrapDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainWrapDX12, swapchain) , 16);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainWrapDX12, gameQueue) , 24);
}

UTEST(ffx_api_framegen_abi_size, ffxCreateContextDescFrameGenerationSwapChainNewDX12) {
  EXPECT_EQ(sizeof(struct ffxCreateContextDescFrameGenerationSwapChainNewDX12) , 48);
}

UTEST(ffx_api_framegen_abi_offsets, ffxCreateContextDescFrameGenerationSwapChainNewDX12) {
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainNewDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainNewDX12, swapchain) , 16);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainNewDX12, desc) , 24);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainNewDX12, dxgiFactory) , 32);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainNewDX12, gameQueue) , 40);
}

UTEST(ffx_api_framegen_abi_size, ffxCreateContextDescFrameGenerationSwapChainForHwndDX12) {
  EXPECT_EQ(sizeof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12) , 64);
}

UTEST(ffx_api_framegen_abi_offsets, ffxCreateContextDescFrameGenerationSwapChainForHwndDX12) {
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, swapchain) , 16);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, hwnd) , 24);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, desc) , 32);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, fullscreenDesc) , 40);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, dxgiFactory) , 48);
  EXPECT_EQ(offsetof(struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12, gameQueue) , 56);
}

UTEST(ffx_api_framegen_abi_size, ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12) , 72);
}

UTEST(ffx_api_framegen_abi_offsets, ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12, uiResource) , 16);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12, flags) , 64);
}

UTEST(ffx_api_framegen_abi_size, ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12) {
  EXPECT_EQ(sizeof(struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12) , 24);
}

UTEST(ffx_api_framegen_abi_offsets, ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12) {
  EXPECT_EQ(offsetof(struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12, pOutCommandList) , 16);
}

UTEST(ffx_api_framegen_abi_size, ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12) {
  EXPECT_EQ(sizeof(struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12) , 24);
}

UTEST(ffx_api_framegen_abi_offsets, ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12) {
  EXPECT_EQ(offsetof(struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12, pOutTexture) , 16);
}

UTEST(ffx_api_framegen_abi_size, ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12) {
  EXPECT_EQ(sizeof(struct ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12) , 16);
}

UTEST(ffx_api_framegen_abi_offsets, ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12) {
  EXPECT_EQ(offsetof(struct ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12, header) , 0);
}

UTEST(ffx_api_framegen_abi_size, ffxConfigureDescFrameGenerationSwapChainKeyValueDX12) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12) , 40);
}

UTEST(ffx_api_framegen_abi_offsets, ffxConfigureDescFrameGenerationSwapChainKeyValueDX12) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12, key) , 16);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12, u64) , 24);
  EXPECT_EQ(offsetof(struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12, ptr) , 32);
}

UTEST(ffx_api_framegen_enums, FfxApiConfigureFrameGenerationSwapChainKeyDX12) {
  EXPECT_EQ(FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK,		0);
  EXPECT_EQ(FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING,	2);
}

UTEST(ffx_api_framegen_abi_size, ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12) {
  EXPECT_EQ(sizeof(struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12) , 24);
}

UTEST(ffx_api_framegen_abi_offsets, ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12) {
  EXPECT_EQ(offsetof(struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12, gpuMemoryUsageFrameGenerationSwapchain) , 16);
}

UTEST_MAIN()
