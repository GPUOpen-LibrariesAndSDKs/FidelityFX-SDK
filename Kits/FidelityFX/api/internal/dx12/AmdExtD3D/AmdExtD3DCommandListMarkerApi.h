/* Copyright (c) 2017-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <unknwn.h>

/**
***********************************************************************************************************************
* @brief D3D Command List Marker extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("735F1F3A-555D-4F70-AB92-7DB4A3AB1D28")
IAmdExtD3DCommandListMarker : public IUnknown
{
public:
    /// Set a command list marker to indicate the beginning of a rendering pass
    virtual void PushMarker(const char* pMarker) = 0;
    /// Set a command list marker to indicate the end of the current rendering pass
    virtual void PopMarker() = 0;
    /// Set a command list marker to indicate a rendering activity
    virtual void SetMarker(const char* pMarker) = 0;
};
