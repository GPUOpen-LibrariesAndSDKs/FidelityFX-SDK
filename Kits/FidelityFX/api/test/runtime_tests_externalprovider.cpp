
#include "windows.h"
#include <iostream>
#include <fstream>  
#include <vector>  
#include <algorithm>  

#include "utest.h"
#include "../include/ffx_api.h" 
#include "../include/ffx_api_loader.h"
#include "../include/dx12/ffx_api_dx12.h"
#include "../../upscalers/include/ffx_upscale.h"

void replaceBytePattern(std::vector<char>&data, const std::vector<char>&oldPattern, const std::vector<char>&newPattern) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (data.size() - i < oldPattern.size()) {
            break;
        }
        if (std::equal(oldPattern.begin(), oldPattern.end(), data.begin() + i)) {
            std::copy(newPattern.begin(), newPattern.end(), data.begin() + i);
            i += oldPattern.size() - 1;
        }
    }
}

UTEST(env, build_64b) {
  EXPECT_EQ(sizeof(void*) , 8);
}

UTEST(env, loadDlls) {
  HMODULE driver = LoadLibraryExA("testffx.dll", NULL, 0);
  HMODULE dll1 = LoadLibraryExA("amd_fidelityfx_dx12_testffx.dll", NULL, 0);
  HMODULE dll2 = LoadLibraryExA("amd_fidelityfx_dx12_testnonamd.dll", NULL, 0);

  EXPECT_TRUE(driver != NULL);
  EXPECT_TRUE(dll1 != NULL);
  EXPECT_TRUE(dll2 != NULL);

  FreeLibrary(driver);
  FreeLibrary(dll1);
  FreeLibrary(dll2);
}

bool hasMockedProvider(std::vector<char*>& versionNames) {
  for (auto name : versionNames) {
    // we are past NDA so should have PostGPUOpen in name
    if (strcmp("PostGPUOpenExternalMockedDriverProviderFSR2", name) == 0)
    {
      return true;
    }
  }
  return false;
}

UTEST(ffx_api_driver_provider_ext, ffxExternalProvidersOverrideOFF) {
  
  // set the environment variable to mimic the adrenaline fidelityfx override switch OFF
  SetEnvironmentVariableA("FFX_API_DRIVER_ENABLE", NULL);

  HMODULE mockdriver = LoadLibraryExA("testffx.dll", NULL, 0);

  HMODULE dllundertest = LoadLibraryExA("amd_fidelityfx_dx12_testffx.dll", NULL, 0);

  ffxFunctions ffxApipFuncs = {};
  ffxLoadFunctions(&ffxApipFuncs, dllundertest);

  uint64_t versionCount = 0;
  ffxQueryDescGetVersions upscaleVersionRequest = {};
  upscaleVersionRequest.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  upscaleVersionRequest.device = (ID3D12Device*)UINT_PTR(1); // any non-null value to pass the device != null override check
  upscaleVersionRequest.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  upscaleVersionRequest.outputCount = &versionCount;
  ffxReturnCode_t ret = ffxApipFuncs.Query(nullptr, &upscaleVersionRequest.header);

  std::vector<uint64_t> versionIds;
  std::vector<char*> versionNames;

  versionIds.resize(versionCount);
  versionNames.resize(versionCount);


  upscaleVersionRequest.versionIds = &versionIds[0];
  upscaleVersionRequest.versionNames = (const char**) & versionNames[0];
  ret = ffxApipFuncs.Query(nullptr, &upscaleVersionRequest.header);

  // the main switch (FFX_API_DRIVER_ENABLE) if OFF above, so we should not find the mocked provider.
  EXPECT_FALSE(hasMockedProvider(versionNames));
  
  FreeLibrary(mockdriver);
  FreeLibrary(dllundertest);
}

UTEST(ffx_api_driver_provider_ext, ffxExternalProvidersOverrideON) {
  
  // set the environment variable to mimic the adrenaline fidelityfx override switch ON
  SetEnvironmentVariableA("FFX_API_DRIVER_ENABLE", "1");

  HMODULE mockdriver = LoadLibraryExA("testffx.dll", NULL, 0);

  HMODULE dllundertest = LoadLibraryExA("amd_fidelityfx_dx12_testffx.dll", NULL, 0);

  ffxFunctions ffxApipFuncs = {};
  ffxLoadFunctions(&ffxApipFuncs, dllundertest);

  uint64_t versionCount = 0;
  ffxQueryDescGetVersions upscaleVersionRequest = {};
  upscaleVersionRequest.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  upscaleVersionRequest.device = (ID3D12Device*)UINT_PTR(1); // any non-null value to pass the device != null override check
  upscaleVersionRequest.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  upscaleVersionRequest.outputCount = &versionCount;
  ffxReturnCode_t ret = ffxApipFuncs.Query(nullptr, &upscaleVersionRequest.header);

  std::vector<uint64_t> versionIds;
  std::vector<char*> versionNames;

  versionIds.resize(versionCount);
  versionNames.resize(versionCount);


  upscaleVersionRequest.versionIds = &versionIds[0];
  upscaleVersionRequest.versionNames = (const char**) & versionNames[0];
  ret = ffxApipFuncs.Query(nullptr, &upscaleVersionRequest.header);

  EXPECT_TRUE(hasMockedProvider(versionNames));
  
  FreeLibrary(mockdriver);
  FreeLibrary(dllundertest);
}

UTEST(ffx_api_driver_provider_ext, ffxExternalProvidersOverrideNonAMDGpu) {
  
  // set the environment variable to mimic the adrenaline fidelityfx override switch ON
  // We want this to "succeed" if the test is to fail, so act as though override should work.
  SetEnvironmentVariableA("FFX_API_DRIVER_ENABLE", "1");

  HMODULE dllundertest = LoadLibraryExA("amd_fidelityfx_dx12_testnonamd.dll", NULL, 0);

  ffxFunctions ffxApipFuncs = {};
  ffxLoadFunctions(&ffxApipFuncs, dllundertest);

  uint64_t versionCount = 0;
  ffxQueryDescGetVersions upscaleVersionRequest = {};
  upscaleVersionRequest.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  upscaleVersionRequest.device = (ID3D12Device*)UINT_PTR(1); // any non-null value to pass the device != null override check
  upscaleVersionRequest.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  upscaleVersionRequest.outputCount = &versionCount;
  ffxReturnCode_t ret = ffxApipFuncs.Query(nullptr, &upscaleVersionRequest.header);

  std::vector<uint64_t> versionIds;
  std::vector<char*> versionNames;

  versionIds.resize(versionCount);
  versionNames.resize(versionCount);


  upscaleVersionRequest.versionIds = &versionIds[0];
  upscaleVersionRequest.versionNames = (const char**) & versionNames[0];
  ret = ffxApipFuncs.Query(nullptr, &upscaleVersionRequest.header);

  // Despite override env enabled, we are "not on amd", so we should not find the mocked provider.
  EXPECT_FALSE(hasMockedProvider(versionNames));
   
  FreeLibrary(dllundertest);
}

UTEST_STATE();

int main(int argc, const char *const argv[]) {
  
  //take the artefact and write out a version that uses a mock driver dll rather than amdxc64
  std::ifstream inputFile("amd_fidelityfx_upscaler_dx12.dll", std::ios::binary);
  std::ofstream outputFile("amd_fidelityfx_dx12_testffx.dll", std::ios::binary);
  // outputput2 to emulate running on non-amd target, without amd driver resident
  std::ofstream outputFile2("amd_fidelityfx_dx12_testnonamd.dll", std::ios::binary);
  if (!inputFile){ 
      std::cerr << "Error opening amd_fidelityfx_upscaler_dx12!" << std::endl;
      return 1;
  }
  if (!outputFile) { 
      std::cerr << "Error opening amd_fidelityfx_dx12_testffx!" << std::endl;
      return 1;
  }
  if (!outputFile2) { 
      std::cerr << "Error opening amd_fidelityfx_dx12_testnonamd!" << std::endl;
      return 1;
  }
  // Read the entire file into a vector  
  std::vector<char> data((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
  // Define the old and new patterns  
  std::vector<char> oldPattern = { 'a','m', 'd', 'x', 'c' , '6' , '4', '.' };
  std::vector<char> newPattern = { 't','e', 's', 't', 'f' , 'f' , 'x', '.' };
  std::vector<char> newPattern2 = { 'n','o', 'n', '_', 'a' , 'm' , 'd', '.' };
  // Replace byte pattern  
  replaceBytePattern(data, oldPattern, newPattern);
  // Write the modified data to the output file  
  outputFile.write(data.data(), data.size());
  
  replaceBytePattern(data, newPattern, newPattern2);
  outputFile2.write(data.data(), data.size());
     
  inputFile.close();
  outputFile.close();
  outputFile2.close();
  
  return utest_main(argc, argv);
}
