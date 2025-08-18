/* Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once
#include <d3d12.h>
#include <dxgi1_2.h>

#include "AmdExtD3D.h"

/**
***********************************************************************************************************************
* @brief AMD extension structure enumeration supported by the driver
***********************************************************************************************************************
*/
enum class AmdExtDXGIStructType : uint32_t
{
    Unknown,           ///< Unsupported
    CreateSwapChain,   ///< DXGISwapChain creation extension
};

/**
***********************************************************************************************************************
* @brief Extension create info base structure
***********************************************************************************************************************
*/
struct AmdExtDXGICreateInfo
{
    AmdExtDXGIStructType type;      ///< AMD create info structure. Must be one of the supported types.
    void*                pNext;     ///< Pointer to a valid AMD structure. Must be nullptr if using base version.
                                    ///  Structures defined by multiple extensions (or versions) may be chained
                                    ///  together using this field. When chaining, the driver will process the chain
                                    ///  starting from the base parameter onwards.
};

/**
***********************************************************************************************************************
* @brief bitfield of swapchain create flags
***********************************************************************************************************************
*/
struct AmdExtDXGISwapChainCreateFlags
{
    union
    {
        struct
        {
            uint32_t vulkanSwapChain : 1;  ///< We're creating a swap chain for AMD's Vulkan driver to use.
            uint32_t oglSwapChain    : 1;  ///< We're creating a swap chain for AMD's OpenGL driver to use.
            uint32_t reserved        : 30; ///< Must be 0
        };
        uint32_t all;
    };
};

/**
***********************************************************************************************************************
* @brief extended swap chain create info structure
***********************************************************************************************************************
*/
struct AmdExtDXGISwapChainCreateInfo : AmdExtDXGICreateInfo
{
    AmdExtDXGISwapChainCreateFlags flags;
};

/**
***********************************************************************************************************************
* @brief Extension DXGIFactory API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("D94E87E7-5EEF-46E4-9669-4A783E056577")
    IAmdExtDXGIFactory : public IUnknown
{
public:
    virtual HRESULT CreateSwapChainForHwnd(
        const AmdExtDXGISwapChainCreateInfo*     pCreateInfo,
        ID3D12CommandQueue*                      pQueue,
        HWND                                     hWnd,
        const DXGI_SWAP_CHAIN_DESC1*             pDesc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*   pFullscreenDesc,
        IDXGIOutput*                             pRestrictToOutput,
        IDXGISwapChain1**                        ppSwapChain) = 0;
};

using IAmdExtDXGIFactoryLatest = IAmdExtDXGIFactory;
