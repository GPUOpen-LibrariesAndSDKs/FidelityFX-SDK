/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "d3d12.h"
#include <unknwn.h>

/**
***********************************************************************************************************************
* @brief Depth-bounds test command list extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("C9FE749F-C5FA-4F40-942B-49A6A72E9D66")
IAmdExtD3DDepthBounds : public IUnknown
{
public:
    virtual void SetDepthBounds(FLOAT minDepth, FLOAT maxDepth) = 0;
};

/**
***********************************************************************************************************************
* @brief Command list extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("CE01333E-BFE5-4462-864B-91A71B00CD32")
IAmdExtD3DGraphicsCommandList : public IUnknown
{
public:
    /**
    *******************************************************************************************************************
    * @brief Perform a ClearRenderTargetView in the runtime, but don't actually do anything in the driver.
    *******************************************************************************************************************
    */
    virtual void DummyClearRenderTargetView(
        ID3D12GraphicsCommandList*  pCmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView,
        const FLOAT                 colorRGBA[4],
        UINT                        numRects,
        const D3D12_RECT*           pRects
    ) = 0;
};
