#include <windows.h>
#include "utest.h"
#include "../include/ffx_api.h" 
#include "../include/ffx_api_loader.h" 

HMODULE g_dllUnderTest = {};

UTEST(env, build_64b) {
  EXPECT_EQ(sizeof(void*) , 8);
}

UTEST(ffx_api_load_dll, g_dllUnderTest) {
  ASSERT_TRUE(g_dllUnderTest != nullptr);
}

UTEST(ffx_api_load_dll, ffxCreateContext) {
  ASSERT_TRUE(GetProcAddress(g_dllUnderTest, "ffxCreateContext") != nullptr);
}

UTEST(ffx_api_load_dll, ffxDestroyContext) {
  ASSERT_TRUE(GetProcAddress(g_dllUnderTest, "ffxDestroyContext") != nullptr);
}

UTEST(ffx_api_load_dll, ffxConfigure) {
  ASSERT_TRUE(GetProcAddress(g_dllUnderTest, "ffxConfigure") != nullptr);
}

UTEST(ffx_api_load_dll, ffxQuery) {
  ASSERT_TRUE(GetProcAddress(g_dllUnderTest, "ffxQuery") != nullptr);
}

UTEST(ffx_api_load_dll, ffxDispatch) {
  ASSERT_TRUE(GetProcAddress(g_dllUnderTest, "ffxDispatch") != nullptr);
}

UTEST(ffx_api_load_dll, ffxLoadFunctions) {
  ffxFunctions funcs = {};
  ffxLoadFunctions(&funcs, g_dllUnderTest);
  ASSERT_TRUE(funcs.CreateContext != nullptr);
  ASSERT_TRUE(funcs.DestroyContext != nullptr);
  ASSERT_TRUE(funcs.Configure != nullptr);
  ASSERT_TRUE(funcs.Query != nullptr);
  ASSERT_TRUE(funcs.Dispatch != nullptr);
}

UTEST_STATE();

int main(int argc, const char *const argv[]) {
  
  g_dllUnderTest = LoadLibraryExA("amd_fidelityfx_loader_dx12.dll", NULL, 0);
  
  return utest_main(argc, argv);
}

