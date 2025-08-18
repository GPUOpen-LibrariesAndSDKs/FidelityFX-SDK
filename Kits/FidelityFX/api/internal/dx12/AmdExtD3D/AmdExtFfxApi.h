/* Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <unknwn.h>

/**
***********************************************************************************************************************
* @brief FFX (FidelityFX) extension API object for MLSR (Machine Learning Super Resolution)
***********************************************************************************************************************
*/
MIDL_INTERFACE("b58d6601-7401-4234-8180-6febfc0e484c")
IAmdExtFfxApi : public IUnknown
{
public:
    // Update FFX API provider
    virtual HRESULT UpdateFfxApiProvider(void* pData, uint32_t dataSizeInBytes) = 0;
};

/**
***********************************************************************************************************************
* @brief Most recent FFX API extension object
***********************************************************************************************************************
*/
using IAmdExtFfxApiLatest = IAmdExtFfxApi;
