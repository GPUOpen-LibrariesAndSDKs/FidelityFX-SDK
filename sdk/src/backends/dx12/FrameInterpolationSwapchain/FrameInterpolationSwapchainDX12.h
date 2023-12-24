// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <comdef.h>
#include <synchapi.h>

#include "FrameInterpolationSwapchainDX12_Helpers.h"

#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include <FidelityFX/host/ffx_fsr3.h>

#define FFX_FRAME_INTERPOLATION_SWAP_CHAIN_VERSION          1
#define FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT 6

typedef struct PacingData
{
    FfxPresentCallbackFunc          presentCallback = nullptr;
    FfxResource                     uiSurface;

    bool                            vsync;
    bool                            tearingSupported;

    UINT64                          gameFenceValue;
    UINT64                          replacementBufferFenceSignal;
    UINT64                          numFramesSentForPresentationBase;
    UINT32                          numFramesToPresent;

    typedef enum FrameIdentifier
    {
        Interpolated_1,
        Real,
        Count
    } FrameIdentifier;

    struct FrameInfo
    {
        bool            doPresent;
        FfxResource     resource;
        UINT64          interpolationCompletedFenceValue;
        UINT64          presentIndex;
        UINT64          presentQpcTarget;
    };
    
    FrameInfo frames[FrameIdentifier::Count];

    void invalidate()
    {
        memset(this, 0, sizeof(PacingData));
    }
} PacingData;

typedef struct FrameinterpolationPresentInfo
{
    CRITICAL_SECTION    criticalSectionScheduledFrame;
    ID3D12Device8*      device    = nullptr;
    IDXGISwapChain4*    swapChain = nullptr;
    Dx12CommandPool<8>  commandPool;

    PacingData          scheduledInterpolations;
    PacingData          scheduledPresents;
    FfxResource         currentUiSurface;

    ID3D12CommandQueue* interpolationQueue      = nullptr;
    ID3D12CommandQueue* asyncComputeQueue       = nullptr;
    ID3D12CommandQueue* gameQueue               = nullptr;
    ID3D12CommandQueue* presentQueue            = nullptr;

    ID3D12Fence*        gameFence               = nullptr;
    ID3D12Fence*        interpolationFence      = nullptr;
    ID3D12Fence*        presentFence            = nullptr;
    ID3D12Fence*        replacementBufferFence  = nullptr;
    ID3D12Fence*        compositionFence        = nullptr;

    HANDLE              presentEvent            = 0;
    HANDLE              interpolationEvent      = 0;
    HANDLE              pacerEvent              = 0;

    volatile bool       shutdown                = false;
} FrameinterpolationPresentInfo;

typedef struct ReplacementResource
{
    ID3D12Resource* resource                = nullptr;
    UINT64          availabilityFenceValue  = 0;

    void destroy()
    {
        SafeRelease(resource);
        availabilityFenceValue = 0;
    }
} ReplacementResource;

// {BEED74B2-282E-4AA3-BBF7-534560507A45}
static const GUID IID_IFfxFrameInterpolationSwapChain = { 0xbeed74b2, 0x282e, 0x4aa3, {0xbb, 0xf7, 0x53, 0x45, 0x60, 0x50, 0x7a, 0x45} };
// {548CA0F7-DD5A-4CBC-BE21-7B0415E34907}
static const GUID IID_IFfxFrameInterpolationSwapChainResourceInfo = {0x548ca0f7, 0xdd5a, 0x4cbc, {0xbe, 0x21, 0x7b, 0x4, 0x15, 0xe3, 0x49, 0x7}};

typedef struct FfxFrameInterpolationSwapChainResourceInfo
{
    int  version;
    bool isInterpolated;
} FfxFrameInterpolationSwapChainResourceInfo;

class DECLSPEC_UUID("BEED74B2-282E-4AA3-BBF7-534560507A45") FrameInterpolationSwapChainDX12 : public IDXGISwapChain4
{
protected:
    HRESULT               shutdown();
    UINT                  getInterpolationEnabledSwapChainFlags(UINT nonAdjustedFlags);
    DXGI_SWAP_CHAIN_DESC1 getInterpolationEnabledSwapChainDescription(const DXGI_SWAP_CHAIN_DESC1* nonAdjustedDesc);

    FrameinterpolationPresentInfo presentInfo_ = {};

    CRITICAL_SECTION    criticalSection_{};

    UINT                gameBufferCount_           = 0;
    UINT                gameFlags_                 = 0;
    DXGI_SWAP_EFFECT    gameSwapEffect_            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    UINT                gameMaximumFrameLatency_   = 1;  // default is 1
    ID3D12Resource*     realBackBuffer0_           = nullptr;  //hold reference to avoid DXGI deadlock on Windows 10

    HANDLE              replacementFrameLatencyWaitableObjectHandle_ = 0;

    UINT64              interpolationFenceValue_             = 0;
    UINT64              gameFenceValue_                      = 0;
    bool                frameInterpolationResetCondition_    = false;

    Dx12Commands*       registeredInterpolationCommandLists_[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT] = {};
    ReplacementResource replacementSwapBuffers_[FFX_FRAME_INTERPOLATION_SWAP_CHAIN_MAX_BUFFER_COUNT] = {};
    ReplacementResource interpolationOutputs_[2]                                                     = {};
    int                 replacementSwapBufferIndex_                                                  = 0;
    int                 interpolationBufferIndex_                                                    = 0;
    UINT64              presentCount_                                                                = 0;

    bool                tearingSupported_          = false;
    bool                interpolationEnabled_      = false;
    bool                presentInterpolatedOnly_   = false;

    UINT64              framesSentForPresentation_  = 0;
    UINT64              nextPresentWaitValue_       = 0;
    HANDLE              interpolationThreadHandle_  = 0;
    ULONG               refCount_                   = 1;

    UINT                backBufferTransferFunction_ = 0;
    float               minLuminance_               = 0.0f;
    float               maxLuminance_               = 0.0f;

    FfxPresentCallbackFunc         presentCallback_         = nullptr;
    FfxFrameGenerationDispatchFunc frameGenerationCallback_ = nullptr;

    void presentPassthrough(UINT SyncInterval, UINT Flags);
    void presentWithUiComposition(UINT SyncInterval, UINT Flags);

    void dispatchInterpolationCommands(FfxResource* pInterpolatedFrame, FfxResource* pRealFrame);
    void presentInterpolated(UINT SyncInterval, UINT Flags);

    bool verifyBackbufferDuplicateResources();
    bool destroyReplacementResources();
    bool killPresenterThread();
    bool spawnPresenterThread();
    void discardOutstandingInterpolationCommandLists();

    IDXGISwapChain4* real();

public:
    void setFrameGenerationConfig(FfxFrameGenerationConfig const* config);
    bool waitForPresents();

    FfxResource interpolationOutput(int index = 0);
    ID3D12GraphicsCommandList* getInterpolationCommandList();

    void registerUiResource(FfxResource uiResource);

    FrameInterpolationSwapChainDX12();
    virtual ~FrameInterpolationSwapChainDX12();

    HRESULT init(
        HWND hWnd,
        const DXGI_SWAP_CHAIN_DESC1* swapChainDesc1,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
        ID3D12CommandQueue* queue,
        IDXGIFactory2* dxgiFactory);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IDXGIObject
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData);
    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown * pUnknown);
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT * pDataSize, void* pData);
    virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent);

    // IDXGIDeviceSubObject
    virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice);

    // IDXGISwapChain1
    virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags);
    virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface);
    virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput * pTarget);
    virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL * pFullscreen, IDXGIOutput * *ppTarget);
    virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC * pDesc);
    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC * pNewTargetParameters);
    virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput * *ppOutput);
    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS * pStats);
    virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT * pLastPresentCount);

    // IDXGISwapChain1
    virtual HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 * pDesc);
    virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pDesc);
    virtual HRESULT STDMETHODCALLTYPE GetHwnd(HWND * pHwnd);
    virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void** ppUnk);
    virtual HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS * pPresentParameters);
    virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported(void);
    virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput * *ppRestrictToOutput);
    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA * pColor);
    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA * pColor);
    virtual HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation);
    virtual HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION * pRotation);

    // IDXGISwapChain2
    virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height);
    virtual HRESULT STDMETHODCALLTYPE GetSourceSize(UINT * pWidth, UINT * pHeight);
    virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency);
    virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT * pMaxLatency);
    virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject(void);
    virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F * pMatrix);
    virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F * pMatrix);

    // IDXGISwapChain3
    virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void);
    virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT * pColorSpaceSupport);
    virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace);
    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT * pCreationNodeMask, IUnknown* const* ppPresentQueue);

    // IDXGISwapChain4
    virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void* pMetaData);
};
