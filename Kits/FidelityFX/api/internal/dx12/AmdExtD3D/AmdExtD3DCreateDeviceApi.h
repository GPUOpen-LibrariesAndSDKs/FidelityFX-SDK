/* Copyright (c) 2017-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dxgi.h>
#include <d3dcommon.h>

/**
***********************************************************************************************************************
* @brief CreateDevice Extension linked list base structure.
* All CreateDevice extensions have structures derived from this.
***********************************************************************************************************************
*/

// List of structure types for use with CreateDevice extensions
enum AmdExtD3DCreateDeviceStructTypeId
{
    AmdExtD3DStructTypeAppRegId         = 0x00000001,
    AmdExtD3DStructTypeDisableDevDriver = 0x00000002,
    AmdExtD3DStructTypeWorkTiling       = 0x00000003,
    AmdExtD3DStructTypePeerUmdOnDxgi    = 0x00000004,
};

struct AmdExtD3DCreateDeviceInfo
{
    AmdExtD3DCreateDeviceStructTypeId   type;   ///< Extension struct type ID
    AmdExtD3DCreateDeviceInfo*          pNext;  ///< pointer to next extension in chain.
};

MIDL_INTERFACE("C4200955-FD57-481A-BD22-947174C6C698")
IAmdExtD3DCreateDevice: public IUnknown
{
public:
virtual HRESULT AmdD3D12CreateDevice(
    IDXGIAdapter*              pAdapter,
    D3D_FEATURE_LEVEL          pFeatureLevels,
    REFIID                     riid,
    void**                     ppDevice,
    AmdExtD3DCreateDeviceInfo* pAmdExtParam) = 0;
};
