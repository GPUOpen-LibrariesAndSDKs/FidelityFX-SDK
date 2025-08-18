/* Copyright (c) 2017-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <unknwn.h>

/**
***********************************************************************************************************************
* @brief AntiLagApi Extension base structure.
***********************************************************************************************************************
*/

struct ApiCallbackData;

MIDL_INTERFACE("44085fbe-e839-40c5-bf38-0ebc5ab4d0a6")
IAmdExtAntiLagApi: public IUnknown
{
public:
    virtual HRESULT UpdateAntiLagState(ApiCallbackData* pApiCallbackData) = 0;
};
