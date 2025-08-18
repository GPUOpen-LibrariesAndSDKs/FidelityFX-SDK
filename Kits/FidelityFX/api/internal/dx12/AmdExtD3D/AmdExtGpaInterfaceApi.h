/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

/**
***********************************************************************************************************************
*    // Example code for getting access to the GpaSession
*    // note: This requires to link against the driver lib and to have a stub dll in the system32/syswow64 directory.
*
*    #ifdef _WIN64
*    #pragma comment(lib, "amdxc64.lib")
*    #else
*    #pragma comment(lib, "amdxc32.lib")
*    #endif
*
*    extern "C" HMODULE __stdcall AmdGetDxcModuleHandle();
*
*    HRESULT hr = S_OK;
*    IAmdExtD3DFactory*                  pAmdExtObject;
*    IAmdExtGpaInterface*                pAmdShaderIntrinsicExt;
*
*    HMODULE m_hAmdD3d12 = AmdGetDxcModuleHandle();
*
*    // alternatively it's also possible to use LoadLibrary but it's not allowed on rs2 for a windows store app:
*    //#ifdef _WIN64
*    //    HMODULE m_hAmdD3d12 = LoadLibrary(L"amdxc64.dll");
*    //#else
*    //    HMODULE m_hAmdD3d12 = LoadLibrary("amdxc32.dll");
*    //#endif
*
*    if (m_hAmdD3d12 != NULL)
*    {
*        PFNAmdExtD3DCreateInterface pAmdExtD3dCreateFunc = (PFNAmdExtD3DCreateInterface)GetProcAddress(m_hAmdD3d12,
*            "AmdExtD3DCreateInterface");
*
*        if (pAmdExtD3dCreateFunc != NULL)
*        {
*            hr = pAmdExtD3dCreateFunc(device.Get(), __uuidof(IAmdExtD3DFactory), reinterpret_cast<void **>(&pAmdExtObject));
*
*            if (hr == S_OK)
*            {
*                hr = pAmdExtObject->CreateInterface(device.Get(),
*                    __uuidof(IAmdExtGpaInterface),
*                    reinterpret_cast<void **>(&pAmdShaderIntrinsicExt));
*            }
*        }
*    }
*
*    IAmdExtGpaSession* gpaSession = pAmdShaderIntrinsicExt->CreateGpaSession();
*    gpaSession->Begin(commandList.Get());
***********************************************************************************************************************
*/

#pragma once

#include <unknwn.h>
#include <d3d12.h>
#include "AmdExtGpaInterface.h"

/**
***********************************************************************************************************************
* @brief Gpu Perf API extension API object
***********************************************************************************************************************
*/
MIDL_INTERFACE("10D00E78-DCBF-4210-BE46-94BD222A332B")
IAmdExtGpaSession : public IUnknown
{
public:
    virtual HRESULT Begin(ID3D12GraphicsCommandList* pGfxCmdList) = 0;
    virtual HRESULT End(ID3D12GraphicsCommandList* pGfxCmdList) = 0;
    virtual UINT32 BeginSample(ID3D12GraphicsCommandList* pGfxCmdList, const AmdExtGpaSampleConfig& config) = 0;
    virtual void EndSample(ID3D12GraphicsCommandList* pGfxCmdList, UINT32 sampleId) = 0;
    virtual bool IsReady() const = 0;
    virtual HRESULT GetResults(UINT32 sampleId, size_t* pSizeInBytes, void *pData) const = 0;
    virtual HRESULT Reset() = 0;
    virtual void CopyResults(ID3D12GraphicsCommandList* pGfxCmdList) = 0;
};

MIDL_INTERFACE("16AE5721-7ED4-4292-9B50-B976DF1347FE")
IAmdExtGpaInterface : public IUnknown
{
public:
    /// Queries for GPU blocks and available counters. Does not return data for counters after AmdExtGpuBlock::Rmi
    virtual HRESULT GetPerfExperimentProperties(AmdExtPerfExperimentProperties* pProperties) const = 0;
    /// Ability to control clocks for profiling
    virtual void SetClockMode(
        const AmdExtDeviceClockMode clockModeInput,
        AmdExtSetClockModeOutput*   pClockModeOutput) = 0;
    /// Creates a gpa session
    virtual IAmdExtGpaSession* CreateGpaSession() = 0;
    /// Creates and returns a copy of a GpaSession object for use with second level command lists
    virtual IAmdExtGpaSession* CopyGpaSession(const IAmdExtGpaSession* pSrc) = 0;
    /// Destroys and frees memory allocated for the gpa session
    virtual void DestroyGpaSession(IAmdExtGpaSession* pExtGpaSession) = 0;
};

MIDL_INTERFACE("802984F4-E529-4A4E-AC4D-A32B1197B55E")
IAmdExtGpaInterface2 : public IAmdExtGpaInterface
{
public:
    /// Queries for GPU blocks and available counters. Pass AmdExtGpuBlock::Count for blockCount
    virtual HRESULT GetPerfExperimentProperties2(AmdExtPerfExperimentProperties* pProperties, AmdExtGpuBlock blockCount) const = 0;
};
