#include "utest.h"
#include "../include/ffx_api.h"
#include "../include/ffx_api_types.h" 

UTEST(env, build_64b) {
  EXPECT_EQ(sizeof(void*) , 8);
}

UTEST(ffx_api_abi_size, ffxApiHeader) {
  EXPECT_EQ(sizeof(struct ffxApiHeader) , 16);
}

UTEST(ffx_api_abi_size, ffxCreateContextDescHeader) {
  EXPECT_EQ(sizeof(ffxCreateContextDescHeader) , 16);
}

UTEST(ffx_api_abi_size, ffxConfigureDescHeader) {
  EXPECT_EQ(sizeof(ffxConfigureDescHeader) , 16);
}

UTEST(ffx_api_abi_size, ffxQueryDescHeader) {
  EXPECT_EQ(sizeof(ffxQueryDescHeader) , 16);
}

UTEST(ffx_api_abi_size, ffxDispatchDescHeader) {
  EXPECT_EQ(sizeof(ffxDispatchDescHeader) , 16);
}

UTEST(ffx_api_abi_size, ffxReturnCode_t) {
  EXPECT_EQ(sizeof(ffxReturnCode_t) , 4);
}

UTEST(ffx_api_abi_size, ffxContext) {
  EXPECT_EQ(sizeof(ffxContext) , 8);
}

UTEST(ffx_api_abi_size, ffxApiMessage) {
  EXPECT_EQ(sizeof(ffxApiMessage) , 8);
}

UTEST(ffx_api_abi_size, ffxConfigureDescGlobalDebug1) {
  EXPECT_EQ(sizeof(struct ffxConfigureDescGlobalDebug1) , 32);
}

UTEST(ffx_api_abi_size, ffxQueryDescGetVersions) {
  EXPECT_EQ(sizeof(struct ffxQueryDescGetVersions) , 56);
}

UTEST(ffx_api_abi_size, ffxOverrideVersion) {
  EXPECT_EQ(sizeof(struct ffxOverrideVersion) , 24);
}

UTEST(ffx_api_abi_size, ffxAlloc) {
  EXPECT_EQ(sizeof(ffxAlloc) , 8);
}

UTEST(ffx_api_abi_size, ffxDealloc) {
  EXPECT_EQ(sizeof(ffxDealloc) , 8);
}

UTEST(ffx_api_abi_size, ffxAllocationCallbacks) {
  EXPECT_EQ(sizeof(ffxAllocationCallbacks) , 24);
}

UTEST(ffx_api_abi_size, PfnFfxCreateContext) {
  EXPECT_EQ(sizeof(PfnFfxCreateContext) , 8);
}

UTEST(ffx_api_abi_size, PfnFfxDestroyContext) {
  EXPECT_EQ(sizeof(PfnFfxDestroyContext) , 8);
}

UTEST(ffx_api_abi_size, PfnFfxConfigure) {
  EXPECT_EQ(sizeof(PfnFfxConfigure) , 8);
}

UTEST(ffx_api_abi_size, PfnFfxDispatch) {
  EXPECT_EQ(sizeof(PfnFfxDispatch) , 8);
}

UTEST(ffx_api_abi_offsets, ffxApiHeader) {
  EXPECT_EQ(offsetof(struct ffxApiHeader, type) , 0);
  EXPECT_EQ(offsetof(struct ffxApiHeader, pNext) , 8);
}

UTEST(ffx_api_abi_offsets, ffxCreateContextDescHeader) {
  EXPECT_EQ(offsetof(ffxCreateContextDescHeader, type) , 0);
  EXPECT_EQ(offsetof(ffxCreateContextDescHeader, pNext) , 8);
}

UTEST(ffx_api_abi_offsets, ffxConfigureDescHeader) {
  EXPECT_EQ(offsetof(ffxConfigureDescHeader, type) , 0);
  EXPECT_EQ(offsetof(ffxConfigureDescHeader, pNext) , 8);
}

UTEST(ffx_api_abi_offsets, ffxQueryDescHeader) {
  EXPECT_EQ(offsetof(ffxQueryDescHeader, type) , 0);
  EXPECT_EQ(offsetof(ffxQueryDescHeader, pNext) , 8);
}

UTEST(ffx_api_abi_offsets, ffxDispatchDescHeader) {
  EXPECT_EQ(offsetof(ffxDispatchDescHeader, type) , 0);
  EXPECT_EQ(offsetof(ffxDispatchDescHeader, pNext) , 8);
}
 
UTEST(ffx_api_abi_offsets, ffxConfigureDescGlobalDebug1) {
  EXPECT_EQ(offsetof(struct ffxConfigureDescGlobalDebug1, header) , 0);
  EXPECT_EQ(offsetof(struct ffxConfigureDescGlobalDebug1, fpMessage) , 16);
  EXPECT_EQ(offsetof(struct ffxConfigureDescGlobalDebug1, debugLevel) , 24);
}
 
UTEST(ffx_api_abi_offsets, ffxQueryDescGetVersions) {
  EXPECT_EQ(offsetof(struct ffxQueryDescGetVersions, header) , 0);
  EXPECT_EQ(offsetof(struct ffxQueryDescGetVersions, createDescType) , 16);
  EXPECT_EQ(offsetof(struct ffxQueryDescGetVersions, device) , 24);
  EXPECT_EQ(offsetof(struct ffxQueryDescGetVersions, outputCount) , 32);
  EXPECT_EQ(offsetof(struct ffxQueryDescGetVersions, versionIds) , 40);
  EXPECT_EQ(offsetof(struct ffxQueryDescGetVersions, versionNames) , 48);
}

UTEST(ffx_api_abi_offsets, ffxOverrideVersion) {
  EXPECT_EQ(offsetof(struct ffxOverrideVersion, header) , 0);
  EXPECT_EQ(offsetof(struct ffxOverrideVersion, versionId) , 16);
}
 
UTEST(ffx_api_abi_offsets, ffxAllocationCallbacks) {
  EXPECT_EQ(offsetof(struct ffxAllocationCallbacks, pUserData) , 0);
  EXPECT_EQ(offsetof(struct ffxAllocationCallbacks, alloc) , 8);
  EXPECT_EQ(offsetof(struct ffxAllocationCallbacks, dealloc) , 16);
}

///////////////TYPES///////////////////


UTEST(ffx_api_types_abi_size, FfxApiDimensions2D) {
  EXPECT_EQ(sizeof(struct FfxApiDimensions2D) , 8);
}


UTEST(ffx_api_types_abi_size, FfxApiFloatCoords2D) {
  EXPECT_EQ(sizeof(struct FfxApiFloatCoords2D) , 8);
}

UTEST(ffx_api_types_abi_size, FfxApiRect2D) {
  EXPECT_EQ(sizeof(struct FfxApiRect2D) , 16);
}

UTEST(ffx_api_types_abi_size, FfxApiResourceDescription) {
  EXPECT_EQ(sizeof(struct FfxApiResourceDescription) , 32);
}

UTEST(ffx_api_types_abi_size, FfxApiResource) {
  EXPECT_EQ(sizeof(struct FfxApiResource) , 48);
}

UTEST(ffx_api_types_abi_size, FfxApiEffectMemoryUsage) {
  EXPECT_EQ(sizeof(struct FfxApiEffectMemoryUsage) , 16);
}

UTEST(ffx_api_types_abi_offsets, FfxApiDimensions2D) {
  EXPECT_EQ(offsetof(struct FfxApiDimensions2D, width) , 0);
  EXPECT_EQ(offsetof(struct FfxApiDimensions2D, height) , 4);
}

UTEST(ffx_api_types_abi_offsets, FfxApiFloatCoords2D) {
  EXPECT_EQ(offsetof(struct FfxApiFloatCoords2D, x) , 0);
  EXPECT_EQ(offsetof(struct FfxApiFloatCoords2D, y) , 4);
}

UTEST(ffx_api_types_abi_offsets, FfxApiRect2D) {
  EXPECT_EQ(offsetof(struct FfxApiRect2D, left) , 0);
  EXPECT_EQ(offsetof(struct FfxApiRect2D, top) , 4);
  EXPECT_EQ(offsetof(struct FfxApiRect2D, width) , 8);
  EXPECT_EQ(offsetof(struct FfxApiRect2D, height) , 12);
}

UTEST(ffx_api_types_abi_offsets, FfxApiResourceDescription) {
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, type) , 0);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, format) , 4);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, width) , 8);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, size) , 8);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, height) , 12);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, stride) , 12);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, depth) , 16);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, alignment) , 16);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, mipCount) , 20);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, flags) , 24);
  EXPECT_EQ(offsetof(struct FfxApiResourceDescription, usage) , 28);
}

UTEST(ffx_api_types_abi_offsets, FfxApiResource) {
  EXPECT_EQ(offsetof(struct FfxApiResource, resource) , 0);
  EXPECT_EQ(offsetof(struct FfxApiResource, description) , 8);
  EXPECT_EQ(offsetof(struct FfxApiResource, state) , 40);
}

UTEST(ffx_api_types_abi_offsets, FfxApiEffectMemoryUsage) {
  EXPECT_EQ(offsetof(struct FfxApiEffectMemoryUsage, totalUsageInBytes) , 0);
  EXPECT_EQ(offsetof(struct FfxApiEffectMemoryUsage, aliasableUsageInBytes) , 8);
}

UTEST(ffx_api_type_enums, FfxApiSurfaceFormat) {
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_UNKNOWN,                0);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS,  1);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT,      2);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT,     3);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT,     4);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32G32B32_FLOAT,        5);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32G32_FLOAT,           6);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8_UINT,                7);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32_UINT,               8);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS,      9);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM,        10);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM,        11);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB,         12);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS,     13);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM,        14);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB,         15);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT,       16);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM,     17);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16G16_FLOAT,          18);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16G16_UINT,           19);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16G16_SINT,           20);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16_FLOAT,             21);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16_UINT,              22);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16_UNORM,             23);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16_SNORM,             24);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8_UNORM,              25);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8_UNORM,            26);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8_UINT,             27);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32_FLOAT,             28);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP,    29);

  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16G16B16A16_TYPELESS, 30);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32G32_TYPELESS,       31);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R10G10B10A2_TYPELESS,  32);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16G16_TYPELESS,       33);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R16_TYPELESS,          34);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8_TYPELESS,           35);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R8G8_TYPELESS,         36);
  EXPECT_EQ(  FFX_API_SURFACE_FORMAT_R32_TYPELESS,          37);
}

UTEST(ffx_api_type_enums,  FfxApiResourceUsage) {
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_READ_ONLY , 0);
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_RENDERTARGET , (1<<0));
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_UAV , (1<<1));
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_DEPTHTARGET , (1<<2));
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_INDIRECT , (1<<3));
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_ARRAYVIEW , (1<<4));
  EXPECT_EQ(   FFX_API_RESOURCE_USAGE_STENCILTARGET , (1<<5));
}

UTEST(ffx_api_type_enums,  FfxApiResourceState) {
  EXPECT_EQ(  FFX_API_RESOURCE_STATE_COMMON, (1 << 0));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, (1 << 1));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_COMPUTE_READ, (1 << 2));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_PIXEL_READ, (1 << 3));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ, (FFX_API_RESOURCE_STATE_PIXEL_READ | FFX_API_RESOURCE_STATE_COMPUTE_READ));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_COPY_SRC, (1 << 4));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_COPY_DEST, (1 << 5));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_GENERIC_READ, (FFX_API_RESOURCE_STATE_COPY_SRC | FFX_API_RESOURCE_STATE_COMPUTE_READ));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_INDIRECT_ARGUMENT, (1 << 6));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_PRESENT, (1 << 7));
  EXPECT_EQ(   FFX_API_RESOURCE_STATE_RENDER_TARGET, (1 << 8));
}

UTEST(ffx_api_type_enums,  FfxApiResourceDimension) {
  EXPECT_EQ(  FFX_API_RESOURCE_DIMENSION_TEXTURE_1D,   0);
  EXPECT_EQ(   FFX_API_RESOURCE_DIMENSION_TEXTURE_2D,  1);
}

UTEST(ffx_api_type_enums,  FfxApiResourceFlags) {
  EXPECT_EQ(  FFX_API_RESOURCE_FLAGS_NONE, 0);
  EXPECT_EQ(   FFX_API_RESOURCE_FLAGS_ALIASABLE, (1 << 0));
  EXPECT_EQ(   FFX_API_RESOURCE_FLAGS_UNDEFINED, (1 << 1));
}

UTEST(ffx_api_type_enums,  FfxApiResourceType) {
  EXPECT_EQ(   FFX_API_RESOURCE_TYPE_BUFFER,0);
  EXPECT_EQ(   FFX_API_RESOURCE_TYPE_TEXTURE1D,1);
  EXPECT_EQ(   FFX_API_RESOURCE_TYPE_TEXTURE2D,2);
  EXPECT_EQ(   FFX_API_RESOURCE_TYPE_TEXTURE_CUBE,3);
  EXPECT_EQ(   FFX_API_RESOURCE_TYPE_TEXTURE3D,4);
}

UTEST(ffx_api_type_enums,  FfxApiBackbufferTransferFunction) {
  EXPECT_EQ(   FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB,0);
  EXPECT_EQ(   FFX_API_BACKBUFFER_TRANSFER_FUNCTION_PQ,1);
  EXPECT_EQ(   FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SCRGB,2);
}



UTEST_MAIN()