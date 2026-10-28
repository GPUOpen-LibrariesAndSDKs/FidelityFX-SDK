// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "../../../api/include/dx12/ffx_api_dx12.h"

#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MAJOR 4
#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MINOR 0
#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_PATCH 0

#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION                           FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MAJOR, FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MINOR, FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_PATCH)

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x01)
struct ffxCreateContextDescFrameGenerationSwapChainWrapDX12
{
    ffxCreateContextDescHeader header;     ///< Description header for frame generation swapchain wrapping context creation.
    IDXGISwapChain4**          swapchain;  ///< Input swap chain to wrap with frame interpolation. On return, contains the wrapped frame interpolation swapchain.
    ID3D12CommandQueue*        gameQueue;  ///< The command queue to be used for presentation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x05)
struct ffxCreateContextDescFrameGenerationSwapChainNewDX12
{
    ffxCreateContextDescHeader header;       ///< Description header for frame generation swapchain creation.
    IDXGISwapChain4**          swapchain;    ///< Output pointer to receive the created frame interpolation swapchain.
    DXGI_SWAP_CHAIN_DESC*      desc;         ///< Swap chain creation parameters.
    IDXGIFactory*              dxgiFactory;  ///< The DXGI factory to use for DX12 swapchain creation.
    ID3D12CommandQueue*        gameQueue;    ///< The command queue to be used for presentation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x06)
struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX12
{
    ffxCreateContextDescHeader       header;          ///< Description header for frame generation swapchain creation for HWND.
    IDXGISwapChain4**                swapchain;       ///< Output pointer to receive the created frame interpolation swapchain.
    HWND                             hwnd;            ///< Window handle for the calling application.
    DXGI_SWAP_CHAIN_DESC1*           desc;            ///< Swap chain creation parameters.
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc;  ///< Fullscreen swap chain creation parameters (can be NULL for windowed mode).
    IDXGIFactory*                    dxgiFactory;     ///< The DXGI factory to use for DX12 swapchain creation.
    ID3D12CommandQueue*              gameQueue;       ///< The command queue to be used for presentation.
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x02)
struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12
{
    ffxConfigureDescHeader header;      ///< Description header for UI resource registration configuration.
    struct FfxApiResource  uiResource;  ///< Resource containing user interface to composite onto generated frames. May be empty.
    uint32_t               flags;       ///< UI composition flags. Zero or combination of values from FfxApiUiCompositionFlags.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x03)
struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12
{
    ffxQueryDescHeader header;           ///< Description header for interpolation command list query.
    void**             pOutCommandList;  ///< Output pointer to receive the command list (ID3D12GraphicsCommandList) to be used for frame generation dispatch.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x04)
struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12
{
    ffxQueryDescHeader     header;       ///< Description header for interpolation texture query.
    struct FfxApiResource* pOutTexture;  ///< Output pointer to receive the resource in which the frame interpolation result should be placed.
};

#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x07)
struct ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12
{
    ffxDispatchDescHeader header;  ///< Description header for wait for presents dispatch.
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x08)
struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX12
{
    ffxConfigureDescHeader header;  ///< Description header for key-value configuration.
    uint64_t               key;     ///< Configuration key from the FfxApiConfigureFrameGenerationSwapChainKeyDX12 enumeration.
    uint64_t               u64;     ///< Integer or enum value to set.
    void*                  ptr;     ///< Pointer value to set or pointer to structure containing value to set.
};

//enum value matches enum FfxFrameInterpolationSwapchainConfigureKey
enum FfxApiConfigureFrameGenerationSwapChainKeyDX12
{
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK      = 0,  ///< Sets FfxWaitCallbackFunc.
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING = 2,  ///< Sets FfxApiSwapchainFramePacingTuning.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x09)
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12
{
    ffxQueryDescHeader              header;                                  ///< Description header for GPU memory usage query.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;  ///< Output pointer to receive GPU memory usage information for frame generation swapchain.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x0a)
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2
{
    ffxQueryDescHeader              header;                                  ///< Description header for GPU memory usage query.
    void*                           device;                                  ///< The GPU device (pointer to ID3D12Device). App needs to fill out before Query() call.
    struct FfxApiDimensions2D       displaySize;                             ///< The resolution at which both rendered and generated frames will be displayed. App needs to fill out before Query() call.
    uint32_t                        backBufferFormat;                        ///< The surface format for the backbuffer. One of the values from FfxApiSurfaceFormat. App needs to fill out before Query() call.
    uint32_t                        backBufferCount;                         ///< The number of backbuffers in the swapchain. App needs to fill out before Query() call.
    struct FfxApiDimensions2D       uiResourceSize;                          ///< The resolution of the resource that will be used for UI composition. Set to (0,0) if providing null uiResource in  ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12. App needs to fill out before Query() call.
    uint32_t                        uiResourceFormat;                        ///< The surface format for the uiResource. One of the values from FfxApiSurfaceFormat. Set to FFX_API_SURFACE_FORMAT_UNKNOWN(0) if providing null uiResource in  ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12. App needs to fill out before Query() call.
    uint32_t                        flags;                                   ///< UI composition flags. Zero or combination of values from FfxApiUiCompositionFlags. App needs to fill out before Query() call.
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;  ///< Output pointer to receive GPU memory usage information for frame generation swapchain. Populated by Query() call.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12 FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(FFX_API_BACKEND_ID_DX12, FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x0b)
struct ffxCreateContextDescFrameGenerationSwapChainVersionDX12
{
    ffxCreateContextDescHeader header;   ///< Description header for frame generation swapchain version context creation.
    uint32_t                   version;  ///< The API version the application was built against. This must be set to FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION.
};
