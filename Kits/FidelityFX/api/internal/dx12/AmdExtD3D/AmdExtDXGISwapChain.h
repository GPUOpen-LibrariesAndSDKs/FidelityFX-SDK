/* Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "AmdExtD3D.h"
#include "dxgi.h"

/**
***********************************************************************************************************************
* @brief bitfield of flags for IAmdExtDXGISwapChain::ResizeBuffers() extension.
***********************************************************************************************************************
*/
struct AmdExtDXGISwapChainResizeInfo
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
* @brief Extension DXGISwapChain API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("04864F6E-D6D5-4DBF-8B00-CD0DEDA5E1E6")
IAmdExtDXGISwapChain : public IUnknown
{
public:
    virtual HRESULT ResizeBuffers(
        const AmdExtDXGISwapChainResizeInfo* pInfo,
        UINT                                 bufferCount,
        UINT                                 width,
        UINT                                 height,
        DXGI_FORMAT                          newFormat,
        UINT                                 swapChainFlags) = 0;
};
