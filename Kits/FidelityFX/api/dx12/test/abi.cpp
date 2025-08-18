#include "../../include/ffx_api.h" 
#include "../../include/ffx_api_types.h" 
#include "../../include/dx12/ffx_api_dx12.h" 

#include "utest.h"

UTEST(env, build_64b) {
  EXPECT_EQ(sizeof(void*) , 8);
}

UTEST(ffx_api_dtypes, dtypes_stable) {
  // tests that the dtype constants can be referenced and do not change.
  EXPECT_EQ(FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12, UINT64_C(0x0000002));
}

UTEST(ffx_api_dx12_abi_size, ffxCreateBackendDX12Desc) {
  EXPECT_EQ(sizeof(struct ffxCreateBackendDX12Desc) , 24);
}

UTEST(ffx_api_dx12_abi_offsets, ffxCreateBackendDX12Desc) {
  EXPECT_EQ(offsetof(struct ffxCreateBackendDX12Desc, header) , 0);
  EXPECT_EQ(offsetof(struct ffxCreateBackendDX12Desc, device) , 16); 
}

UTEST_MAIN()