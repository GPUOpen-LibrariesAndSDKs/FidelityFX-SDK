/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <unknwn.h>
#include "AmdExtD3DShaderIntrinsics.h"

/**
***********************************************************************************************************************
* @brief Shader Intrinsics extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("BA019D53-CCAB-4CBD-B56A-7230ED4330AD")
IAmdExtD3DShaderIntrinsics: public IUnknown
{
public:
    virtual HRESULT GetInfo(AmdExtD3DShaderIntrinsicsInfo* pInfo) = 0;
    virtual HRESULT CheckSupport(AmdExtD3DShaderIntrinsicsSupport opcodeSupport) = 0;
    virtual HRESULT Enable() = 0;
};

/**
***********************************************************************************************************************
* @brief Shader Intrinsics extension API object version 1
***********************************************************************************************************************
*/
MIDL_INTERFACE("E6144584-03DE-439C-9C0B-43AE6D009BC6")
IAmdExtD3DShaderIntrinsics1: public IAmdExtD3DShaderIntrinsics
{
public:
    virtual HRESULT SetExtensionUavBinding(UINT registerIndex, UINT registerSpace) = 0;
};
