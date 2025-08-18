/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#include <unknwn.h>
#include <cinttypes>

/*      All AMD extensions contain the standard IUnknown interface:
 * virtual HRESULT STDMETHODCALLTYPE QueryInterface(
 *             REFIID riid,
 *             _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
 * virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;
 * virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
 */

/*
 * The app must use GetProcAddress, etc.to retrieve this exported function
 * The associated typedef provides a convenient way to define the function pointer
 *
 * HRESULT  __cdecl AmdExtD3DCreateInterface(
 *  IUnknown*   pOuter,     ///< [in] object on which to base this new interface; usually a D3D device
 *  REFIID      riid,       ///< ID of the requested interface
 *  void**      ppvObject); ///< [out] The result interface object
 */
typedef HRESULT (__cdecl *PFNAmdExtD3DCreateInterface)(IUnknown* pOuter, REFIID riid, void** ppvObject);

/**
***********************************************************************************************************************
* @brief Abstract factory for extension interfaces
*
* Each extension interface (e.g. tessellation) will derive from this class
***********************************************************************************************************************
*/
MIDL_INTERFACE("014937EC-9288-446F-A9AC-D75A8E3A984F")
IAmdExtD3DFactory : public IUnknown
{
public:
    virtual HRESULT CreateInterface(
        IUnknown* pOuter,           ///< [in] An object on which to base this new interface; the required object type
                                    ///< is usually a device object but not always
        REFIID    riid,             ///< The ID of the requested interface
        void**    ppvObject) = 0;   ///< [out] The result interface object
};
