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


#include <initguid.h>
#include "FrameInterpolationSwapChainDX12.h"

#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include "FrameInterpolationSwapchainDX12_UiComposition.h"

FfxErrorCode ffxRegisterFrameinterpolationUiResourceDX12(FfxSwapchain gameSwapChain, FfxResource uiResource)
{
    IDXGISwapChain4* swapChain = ffxGetDX12SwapchainPtr(gameSwapChain);

    FrameInterpolationSwapChainDX12* framinterpolationSwapchain = nullptr;
    if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&framinterpolationSwapchain))))
    {
        framinterpolationSwapchain->registerUiResource(uiResource);

        SafeRelease(framinterpolationSwapchain);

        return FFX_OK;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}

FFX_API FfxErrorCode ffxSetFrameGenerationConfigToSwapchainDX12(FfxFrameGenerationConfig const* config)
{
    FfxErrorCode result = FFX_ERROR_INVALID_ARGUMENT;

    if (config->swapChain)
    {
        IDXGISwapChain4*                 swapChain = ffxGetDX12SwapchainPtr(config->swapChain);
        FrameInterpolationSwapChainDX12* framinterpolationSwapchain = nullptr;
        if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&framinterpolationSwapchain))))
        {
            framinterpolationSwapchain->setFrameGenerationConfig(config);

            SafeRelease(framinterpolationSwapchain);
            
            result = FFX_OK;
        }
    }

    return result;
}

FfxResource ffxGetFrameinterpolationTextureDX12(FfxSwapchain gameSwapChain)
{
    FfxResource                      res = { nullptr };
    IDXGISwapChain4* swapChain = ffxGetDX12SwapchainPtr(gameSwapChain);
    FrameInterpolationSwapChainDX12* framinterpolationSwapchain = nullptr;
    if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&framinterpolationSwapchain))))
    {
        res = framinterpolationSwapchain->interpolationOutput(0);

        SafeRelease(framinterpolationSwapchain);
    }
    return res;
}

FfxErrorCode ffxGetFrameinterpolationCommandlistDX12(FfxSwapchain gameSwapChain, FfxCommandList& gameCommandlist)
{
    // 1) query FrameInterpolationSwapChainDX12 from gameSwapChain
    // 2) call  FrameInterpolationSwapChainDX12::getInterpolationCommandList()
    IDXGISwapChain4* swapChain = ffxGetDX12SwapchainPtr(gameSwapChain);

    FrameInterpolationSwapChainDX12* framinterpolationSwapchain = nullptr;
    if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&framinterpolationSwapchain))))
    {
        gameCommandlist = framinterpolationSwapchain->getInterpolationCommandList();

        SafeRelease(framinterpolationSwapchain);

        return FFX_OK;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}

FfxErrorCode ffxReplaceSwapchainForFrameinterpolationDX12(FfxCommandQueue gameQueue, FfxSwapchain& gameSwapChain)
{
    FfxErrorCode     status            = FFX_ERROR_INVALID_ARGUMENT;
    IDXGISwapChain4* dxgiGameSwapChain = reinterpret_cast<IDXGISwapChain4*>(gameSwapChain);
    FFX_ASSERT(dxgiGameSwapChain);

    ID3D12CommandQueue* queue = reinterpret_cast<ID3D12CommandQueue*>(gameQueue);
    FFX_ASSERT(queue);

    // we just need the desc, release the real swapchain as we'll replace that with one doing frameinterpolation
    HWND                            hWnd;
    DXGI_SWAP_CHAIN_DESC1           desc1;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
    if (SUCCEEDED(dxgiGameSwapChain->GetDesc1(&desc1)) &&
        SUCCEEDED(dxgiGameSwapChain->GetFullscreenDesc(&fullscreenDesc)) &&
        SUCCEEDED(dxgiGameSwapChain->GetHwnd(&hWnd))
        )
    {
        FFX_ASSERT_MESSAGE(fullscreenDesc.Windowed == TRUE, "Illegal to release a fullscreen swap chain.");

        IDXGIFactory* dxgiFactory = getDXGIFactoryFromSwapChain(dxgiGameSwapChain);
        SafeRelease(dxgiGameSwapChain);

        FfxSwapchain proxySwapChain;
        status = ffxCreateFrameinterpolationSwapchainForHwndDX12(hWnd, &desc1, &fullscreenDesc, queue, dxgiFactory, proxySwapChain);
        if (status == FFX_OK)
        {
            gameSwapChain = proxySwapChain;
        }

        SafeRelease(dxgiFactory);
    }

    return status;
}

FfxErrorCode ffxCreateFrameinterpolationSwapchainDX12(const DXGI_SWAP_CHAIN_DESC*   desc,
                                                      ID3D12CommandQueue*           queue,
                                                      IDXGIFactory*                 dxgiFactory,
                                                      FfxSwapchain&                 outGameSwapChain)
{
    FFX_ASSERT(desc);
    FFX_ASSERT(queue);
    FFX_ASSERT(dxgiFactory);

    DXGI_SWAP_CHAIN_DESC1 desc1{};
    desc1.Width         = desc->BufferDesc.Width;
    desc1.Height        = desc->BufferDesc.Height;
    desc1.Format        = desc->BufferDesc.Format;
    desc1.SampleDesc    = desc->SampleDesc;
    desc1.BufferUsage   = desc->BufferUsage;
    desc1.BufferCount   = desc->BufferCount;
    desc1.SwapEffect    = desc->SwapEffect;
    desc1.Flags         = desc->Flags;

    // for clarity, params not part of DXGI_SWAP_CHAIN_DESC
    // implicit behavior of DXGI when you call the IDXGIFactory::CreateSwapChain
    desc1.Scaling       = DXGI_SCALING_STRETCH;
    desc1.AlphaMode     = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc1.Stereo        = FALSE;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
    fullscreenDesc.Scaling          = desc->BufferDesc.Scaling;
    fullscreenDesc.RefreshRate      = desc->BufferDesc.RefreshRate;
    fullscreenDesc.ScanlineOrdering = desc->BufferDesc.ScanlineOrdering;
    fullscreenDesc.Windowed         = desc->Windowed;

    return ffxCreateFrameinterpolationSwapchainForHwndDX12(desc->OutputWindow, &desc1, &fullscreenDesc, queue, dxgiFactory, outGameSwapChain);
}

FfxErrorCode ffxCreateFrameinterpolationSwapchainForHwndDX12(HWND                                   hWnd,
                                                             const DXGI_SWAP_CHAIN_DESC1*           desc1,
                                                             const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
                                                             ID3D12CommandQueue*                    queue,
                                                             IDXGIFactory*                          dxgiFactory,
                                                             FfxSwapchain&                          outGameSwapChain)
{
    // don't assert fullscreenDesc, nullptr valid
    FFX_ASSERT(hWnd != 0);
    FFX_ASSERT(desc1);
    FFX_ASSERT(queue);
    FFX_ASSERT(dxgiFactory);

    FfxErrorCode err = FFX_ERROR_INVALID_ARGUMENT;

    IDXGIFactory2* dxgiFactory2 = nullptr;
    if (SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory2))))
    {
        // Create proxy swapchain
        FrameInterpolationSwapChainDX12* fiSwapchain = new FrameInterpolationSwapChainDX12();
        if (fiSwapchain)
        {
            if (SUCCEEDED(fiSwapchain->init(hWnd, desc1, fullscreenDesc, queue, dxgiFactory2)))
            {
                outGameSwapChain = ffxGetSwapchainDX12(fiSwapchain);

                err = FFX_OK;
            }
            else
            {
                delete fiSwapchain;
                err = FFX_ERROR_INVALID_ARGUMENT;
            }
        }
        else
        {
            err = FFX_ERROR_OUT_OF_MEMORY;
        }

        SafeRelease(dxgiFactory2);
    }

    return err;
}

FfxErrorCode ffxWaitForPresents(FfxSwapchain gameSwapChain)
{
    IDXGISwapChain4* swapChain = ffxGetDX12SwapchainPtr(gameSwapChain);

    FrameInterpolationSwapChainDX12* framinterpolationSwapchain;
    if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&framinterpolationSwapchain))))
    {
        framinterpolationSwapchain->waitForPresents();
        SafeRelease(framinterpolationSwapchain);

        return FFX_OK;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}

void setSwapChainBufferResourceInfo(IDXGISwapChain4* swapChain, bool isInterpolated)
{
    uint32_t        currBackbufferIndex = swapChain->GetCurrentBackBufferIndex();
    ID3D12Resource* swapchainBackbuffer = nullptr;

    if (SUCCEEDED(swapChain->GetBuffer(currBackbufferIndex, IID_PPV_ARGS(&swapchainBackbuffer))))
    {
        FfxFrameInterpolationSwapChainResourceInfo info{};
        info.version        = FFX_FRAME_INTERPOLATION_SWAP_CHAIN_VERSION;
        info.isInterpolated = isInterpolated;
        HRESULT hr = swapchainBackbuffer->SetPrivateData(IID_IFfxFrameInterpolationSwapChainResourceInfo, sizeof(info), &info);
        FFX_ASSERT(SUCCEEDED(hr));

        /*
        usage example:
        
        FfxFrameInterpolationSwapChainResourceInfo info{};
        UINT size = sizeof(info);
        if (SUCCEEDED(swapchainBackbuffer->GetPrivateData(IID_IFfxFrameInterpolationSwapChainResourceInfo, &size, &info))) {
            
        } else {
            // buffer was not presented using proxy swapchain
        }
        */

        SafeRelease(swapchainBackbuffer);
    }
}

HRESULT compositeSwapChainFrame(FrameinterpolationPresentInfo* presenter, PacingData* pacingEntry, uint32_t frameID)
{
    const PacingData::FrameInfo& frameInfo = pacingEntry->frames[frameID];

    presenter->presentQueue->Wait(presenter->interpolationFence, frameInfo.interpolationCompletedFenceValue);

    if (pacingEntry->presentCallback)
    {
        auto gpuCommands = presenter->commandPool.get(presenter->presentQueue, L"compositeSwapChainFrame");

        uint32_t        currBackbufferIndex = presenter->swapChain->GetCurrentBackBufferIndex();
        ID3D12Resource* swapchainBackbuffer = nullptr;
        presenter->swapChain->GetBuffer(currBackbufferIndex, IID_PPV_ARGS(&swapchainBackbuffer));

        FfxPresentCallbackDescription desc{};
        desc.commandList            = ffxGetCommandListDX12(gpuCommands->reset());
        desc.device                 = presenter->device;
        desc.isInterpolatedFrame    = frameID != PacingData::FrameIdentifier::Real;
        desc.outputSwapChainBuffer  = ffxGetResourceDX12(swapchainBackbuffer, GetFfxResourceDescriptionDX12(swapchainBackbuffer), nullptr, FFX_RESOURCE_STATE_PRESENT);
        desc.currentBackBuffer      = frameInfo.resource;
        desc.currentUI              = pacingEntry->uiSurface;

        pacingEntry->presentCallback(&desc);

        gpuCommands->execute(true);

        SafeRelease(swapchainBackbuffer);
    }

    presenter->presentQueue->Signal(presenter->compositionFence, frameInfo.presentIndex);

    return S_OK;
}

void presentToSwapChain(FrameinterpolationPresentInfo* presenter, PacingData* pacingEntry, uint32_t frameID)
{
    const PacingData::FrameInfo& frameInfo = pacingEntry->frames[frameID];

    const UINT uSyncInterval            = pacingEntry->vsync ? 1 : 0;
    const bool bExclusiveFullscreen     = isExclusiveFullscreen(presenter->swapChain);
    const bool bSetAllowTearingFlag     = pacingEntry->tearingSupported && !bExclusiveFullscreen && (0 == uSyncInterval);
    const UINT uFlags                   = bSetAllowTearingFlag * DXGI_PRESENT_ALLOW_TEARING;

    presenter->swapChain->Present(uSyncInterval, uFlags);

    // tick frames sent for presentation
    presenter->presentQueue->Signal(presenter->presentFence, frameInfo.presentIndex);
}

DWORD WINAPI presenterThread(LPVOID param)
{
    FrameinterpolationPresentInfo* presenter = static_cast<FrameinterpolationPresentInfo*>(param);

    if (presenter)
    {
        UINT64 numFramesSentForPresentation = 0;

        while (!presenter->shutdown)
        {

            WaitForSingleObject(presenter->pacerEvent, INFINITE);

            if (!presenter->shutdown)
            {
                EnterCriticalSection(&presenter->criticalSectionScheduledFrame);

                PacingData entry = presenter->scheduledPresents;
                presenter->scheduledPresents.invalidate();

                LeaveCriticalSection(&presenter->criticalSectionScheduledFrame);

                if (entry.numFramesToPresent > 0)
                {
                    // we might have dropped entries so have to update here, otherwise we might deadlock
                    presenter->presentQueue->Signal(presenter->presentFence, entry.numFramesSentForPresentationBase);
                    presenter->presentQueue->Wait(presenter->gameFence, entry.gameFenceValue);

                    for (uint32_t frameID = 0; frameID < PacingData::FrameIdentifier::Count; frameID++)
                    {
                        const PacingData::FrameInfo& frameInfo = entry.frames[frameID];
                        if (frameInfo.doPresent)
                        {
                            compositeSwapChainFrame(presenter, &entry, frameID);

                            // signal replacement buffer availability
                            if (frameInfo.presentIndex == entry.replacementBufferFenceSignal)
                            {
                                presenter->presentQueue->Signal(presenter->replacementBufferFence, entry.replacementBufferFenceSignal);
                            }

                            waitForPerformanceCount(frameInfo.presentQpcTarget);

                            presentToSwapChain(presenter, &entry, frameID);
                        }
                    }

                    numFramesSentForPresentation = entry.numFramesSentForPresentationBase + entry.numFramesToPresent;
                }
            }
        }

        waitForFenceValue(presenter->presentFence, numFramesSentForPresentation);
    }

    return 0;
}

DWORD WINAPI interpolationThread(LPVOID param)
{
    FrameinterpolationPresentInfo* presenter = static_cast<FrameinterpolationPresentInfo*>(param);

    if (presenter)
    {
        HANDLE presenterThreadHandle = CreateThread(nullptr, 0, presenterThread, param, 0, nullptr);
        FFX_ASSERT(presenterThreadHandle != NULL);

        if (presenterThreadHandle != 0)
        {
            SetThreadPriority(presenterThreadHandle, THREAD_PRIORITY_HIGHEST);
            SetThreadDescription(presenterThreadHandle, L"AMD FSR Presenter Thread");

            SimpleMovingAverage<2, double> frameTime{};
            SimpleMovingAverage<2, double> compositionTime{};

            int64_t previousQpc = 0;

            while (!presenter->shutdown)
            {
                WaitForSingleObject(presenter->presentEvent, INFINITE);

                if (!presenter->shutdown)
                {
                    EnterCriticalSection(&presenter->criticalSectionScheduledFrame);

                    PacingData entry = presenter->scheduledInterpolations;
                    presenter->scheduledInterpolations.invalidate();

                    LeaveCriticalSection(&presenter->criticalSectionScheduledFrame);

                    waitForFenceValue(presenter->interpolationFence,
                        entry.frames[PacingData::FrameIdentifier::Interpolated_1].interpolationCompletedFenceValue);
                    SetEvent(presenter->interpolationEvent);

                    int64_t currentQpc = 0;
                    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentQpc));

                    const double deltaQpc = double(currentQpc - previousQpc) * (previousQpc > 0);
                    previousQpc           = currentQpc;

                    // reset pacing averaging if delta > 10 fps,
                    int64_t qpcFrequency;
                    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&qpcFrequency));
                    const float fTimeoutInSeconds       = 0.1f;
                    double      deltaQpcResetThreashold = double(qpcFrequency * fTimeoutInSeconds);
                    if (deltaQpc > deltaQpcResetThreashold)
                    {
                        frameTime.reset();
                        compositionTime.reset();
                    }
                    else
                    {
                        frameTime.update(deltaQpc);
                    }

                    // set presentation time for the real frame
                    const int64_t deltaToUse                                            = int64_t(frameTime.getAverage() * 0.5) + int64_t(compositionTime.getAverage());
                    entry.frames[PacingData::FrameIdentifier::Real].presentQpcTarget    = currentQpc + deltaToUse;

                    // schedule presents
                    EnterCriticalSection(&presenter->criticalSectionScheduledFrame);
                    presenter->scheduledPresents = entry;
                    LeaveCriticalSection(&presenter->criticalSectionScheduledFrame);
                    SetEvent(presenter->pacerEvent);

                    // estimate gpu composition time if both interpolated and real frames are to be presented
                    if (entry.vsync == false &&
                        entry.frames[PacingData::FrameIdentifier::Interpolated_1].doPresent &&
                        entry.frames[PacingData::FrameIdentifier::Real].doPresent)
                    {
                        int64_t compositionBeginQpc             = 0;
                        int64_t compositionEndQpc               = 0;

                        // wait for interpolated frame present to finish
                        waitForFenceValue(presenter->presentFence, entry.frames[PacingData::FrameIdentifier::Interpolated_1].presentIndex);
                        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&compositionBeginQpc));

                        // wait for real frame composition to finish
                        waitForFenceValue(presenter->compositionFence, entry.frames[PacingData::FrameIdentifier::Real].presentIndex);
                        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&compositionEndQpc));

                        const double duration = double(compositionEndQpc - compositionBeginQpc);
                        compositionTime.update(duration);
                    }
                    else
                    {
                        compositionTime.reset();
                    }
                }
            }

            // signal event to allow thread to finish
            SetEvent(presenter->pacerEvent);
            WaitForSingleObject(presenterThreadHandle, INFINITE);
            SafeCloseHandle(presenterThreadHandle);
        }
    }

    return 0;
}

bool FrameInterpolationSwapChainDX12::verifyBackbufferDuplicateResources()
{
    HRESULT hr = S_OK;

    ID3D12Device8*  device = nullptr;
    ID3D12Resource* buffer = nullptr;
    if (SUCCEEDED(real()->GetBuffer(0, IID_PPV_ARGS(&buffer))))
    {
        if (SUCCEEDED(buffer->GetDevice(IID_PPV_ARGS(&device))))
        {
            auto bufferDesc = buffer->GetDesc();

            D3D12_HEAP_PROPERTIES heapProperties{};
            D3D12_HEAP_FLAGS      heapFlags;
            buffer->GetHeapProperties(&heapProperties, &heapFlags);

            heapFlags &= ~D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
            heapFlags &= ~D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
            heapFlags &= ~D3D12_HEAP_FLAG_DENY_BUFFERS;
            heapFlags &= ~D3D12_HEAP_FLAG_ALLOW_DISPLAY;

            for (size_t i = 0; i < gameBufferCount_; i++)
            {
                if (replacementSwapBuffers_[i].resource == nullptr)
                {
                    // create game render output resource
                    if (FAILED(device->CreateCommittedResource(&heapProperties,
                                                                heapFlags,
                                                                &bufferDesc,
                                                                D3D12_RESOURCE_STATE_PRESENT,
                                                                nullptr,
                                                                IID_PPV_ARGS(&replacementSwapBuffers_[i].resource))))
                    {
                        hr |= E_FAIL;
                    }
                    else
                    {
                        replacementSwapBuffers_[i].resource->SetName(L"AMD FSR Replacement BackBuffer");
                    }
                }
            }

            for (size_t i = 0; i < _countof(interpolationOutputs_); i++)
            {
                // create interpolation output resource
                bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                if (interpolationOutputs_[i].resource == nullptr)
                {
                    if (FAILED(device->CreateCommittedResource(
                            &heapProperties, heapFlags, &bufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&interpolationOutputs_[i].resource))))
                    {
                        hr |= E_FAIL;
                    }
                    else
                    {
                        interpolationOutputs_[i].resource->SetName(L"AMD FSR Interpolation Output");
                    }
                }
            }

            SafeRelease(device);
        }

        SafeRelease(realBackBuffer0_);
        realBackBuffer0_ = buffer;
    }

    return SUCCEEDED(hr);
}

HRESULT FrameInterpolationSwapChainDX12::init(HWND                                  hWnd,
                                              const DXGI_SWAP_CHAIN_DESC1*          desc,
                                              const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
                                              ID3D12CommandQueue* queue,
                                              IDXGIFactory2* dxgiFactory)
{
    FFX_ASSERT(desc);
    FFX_ASSERT(queue);
    FFX_ASSERT(dxgiFactory);

    // store values we modify, to return when application asks for info
    gameBufferCount_    = desc->BufferCount;
    gameFlags_          = desc->Flags;
    gameSwapEffect_     = desc->SwapEffect;

    // set default ui composition / frame interpolation present function
    presentCallback_    = ffxFrameInterpolationUiComposition;

    HRESULT hr = E_FAIL;

    if (SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&presentInfo_.device))))
    {
        presentInfo_.gameQueue       = queue;

        InitializeCriticalSection(&criticalSection_);
        InitializeCriticalSection(&presentInfo_.criticalSectionScheduledFrame);
        presentInfo_.presentEvent       = CreateEvent(NULL, FALSE, FALSE, nullptr);
        presentInfo_.interpolationEvent = CreateEvent(NULL, FALSE, TRUE, nullptr);
        presentInfo_.pacerEvent         = CreateEvent(NULL, FALSE, FALSE,nullptr);
        tearingSupported_               = isTearingSupported(dxgiFactory);

        // Create presentation queue
        D3D12_COMMAND_QUEUE_DESC presentQueueDesc = queue->GetDesc();
        presentQueueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        presentQueueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        presentQueueDesc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
        presentQueueDesc.NodeMask                 = 0;
        presentInfo_.device->CreateCommandQueue(&presentQueueDesc, IID_PPV_ARGS(&presentInfo_.presentQueue));
        presentInfo_.presentQueue->SetName(L"AMD FSR PresentQueue");

        // Setup pass-through swapchain default state is disabled/passthrough
        IDXGISwapChain1* pSwapChain1 = nullptr;

        DXGI_SWAP_CHAIN_DESC1 realDesc = getInterpolationEnabledSwapChainDescription(desc);
        hr = dxgiFactory->CreateSwapChainForHwnd(presentInfo_.presentQueue, hWnd, &realDesc, fullscreenDesc, nullptr, &pSwapChain1);
        if (SUCCEEDED(hr) && queue)
        {
            if (SUCCEEDED(hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(&presentInfo_.swapChain))))
            {
                // Register proxy swapchain to the real swap chain object
                presentInfo_.swapChain->SetPrivateData(IID_IFfxFrameInterpolationSwapChain, sizeof(FrameInterpolationSwapChainDX12*), this);

                SafeRelease(pSwapChain1);
            }
            else
            {
                FFX_ASSERT(hr == S_OK);
                return hr;
            }
        }
        else
        {
            FFX_ASSERT(hr == S_OK);
            return hr;
        }

        // init min and lax luminance according to monitor metadata
        // in case app doesn't set it through SetHDRMetadata
        getMonitorLuminanceRange(presentInfo_.swapChain, &minLuminance_, &maxLuminance_);

        presentInfo_.device->CreateFence(gameFenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&presentInfo_.gameFence));
        presentInfo_.gameFence->SetName(L"AMD FSR GameFence");

        presentInfo_.device->CreateFence(interpolationFenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&presentInfo_.interpolationFence));
        presentInfo_.interpolationFence->SetName(L"AMD FSR InterpolationFence");

        presentInfo_.device->CreateFence(framesSentForPresentation_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&presentInfo_.presentFence));
        presentInfo_.presentFence->SetName(L"AMD FSR PresentFence");

        presentInfo_.device->CreateFence(framesSentForPresentation_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&presentInfo_.replacementBufferFence));
        presentInfo_.replacementBufferFence->SetName(L"AMD FSR ReplacementBufferFence");

        presentInfo_.device->CreateFence(framesSentForPresentation_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&presentInfo_.compositionFence));
        presentInfo_.compositionFence->SetName(L"AMD FSR CompositionFence");

        replacementFrameLatencyWaitableObjectHandle_ = CreateEvent(0, FALSE, TRUE, nullptr);

        // Create interpolation queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = queue->GetDesc();
        queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        queueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
        queueDesc.NodeMask                 = 0;
        presentInfo_.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&presentInfo_.asyncComputeQueue));
        presentInfo_.asyncComputeQueue->SetName(L"AMD FSR AsyncComputeQueue");

        // Default to dispatch interpolation workloads on the game queue
        presentInfo_.interpolationQueue = presentInfo_.gameQueue;
    }

    return hr;
}

FrameInterpolationSwapChainDX12::FrameInterpolationSwapChainDX12()
{

}

FrameInterpolationSwapChainDX12::~FrameInterpolationSwapChainDX12()
{
    shutdown();
}

UINT FrameInterpolationSwapChainDX12::getInterpolationEnabledSwapChainFlags(UINT nonAdjustedFlags)
{
    UINT flags = nonAdjustedFlags;
    flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    if (tearingSupported_)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    return flags;
}

DXGI_SWAP_CHAIN_DESC1 FrameInterpolationSwapChainDX12::getInterpolationEnabledSwapChainDescription(const DXGI_SWAP_CHAIN_DESC1* nonAdjustedDesc)
{
    DXGI_SWAP_CHAIN_DESC1 fiDesc = *nonAdjustedDesc;

    // adjust swap chain descriptor to fit FI requirements
    fiDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    fiDesc.BufferCount = 3;
    fiDesc.Flags = getInterpolationEnabledSwapChainFlags(fiDesc.Flags);

    return fiDesc;
}

IDXGISwapChain4* FrameInterpolationSwapChainDX12::real()
{
    return presentInfo_.swapChain;
}

HRESULT FrameInterpolationSwapChainDX12::shutdown()
{
    //m_pDevice will be nullptr already shutdown
    if (presentInfo_.device)
    {
        releaseUiBlitGpuResources();

        destroyReplacementResources();

        killPresenterThread();
        SafeCloseHandle(presentInfo_.presentEvent);
        SafeCloseHandle(presentInfo_.interpolationEvent);
        SafeCloseHandle(presentInfo_.pacerEvent);


        presentInfo_.interpolationQueue->Signal(presentInfo_.interpolationFence, ++interpolationFenceValue_);
        waitForFenceValue(presentInfo_.interpolationFence, interpolationFenceValue_);
        SafeRelease(presentInfo_.asyncComputeQueue);
        SafeRelease(presentInfo_.presentQueue);

        SafeRelease(presentInfo_.interpolationFence);
        SafeRelease(presentInfo_.presentFence);
        SafeRelease(presentInfo_.replacementBufferFence);
        SafeRelease(presentInfo_.compositionFence);

        UINT sc_refCount = SafeRelease(presentInfo_.swapChain);

        waitForFenceValue(presentInfo_.gameFence, gameFenceValue_);
        SafeRelease(presentInfo_.gameFence);

        DeleteCriticalSection(&criticalSection_);
        DeleteCriticalSection(&presentInfo_.criticalSectionScheduledFrame);

        UINT device_refCount = SafeRelease(presentInfo_.device);
    }

    return S_OK;
}

bool FrameInterpolationSwapChainDX12::killPresenterThread()
{
    if (interpolationThreadHandle_ != NULL)
    {
        // prepare present CPU thread for shutdown
        presentInfo_.shutdown = true;

        // signal event to allow thread to finish
        SetEvent(presentInfo_.presentEvent);
        WaitForSingleObject(interpolationThreadHandle_, INFINITE);
        SafeCloseHandle(interpolationThreadHandle_);
    }

    return interpolationThreadHandle_ == nullptr;
}

bool FrameInterpolationSwapChainDX12::spawnPresenterThread()
{
    if (interpolationThreadHandle_ == NULL)
    {
        presentInfo_.shutdown     = false;
        interpolationThreadHandle_ = CreateThread(nullptr, 0, interpolationThread, reinterpret_cast<void*>(&presentInfo_), 0, nullptr);
        
        FFX_ASSERT(interpolationThreadHandle_ != NULL);

        if (interpolationThreadHandle_ != 0)
        {
            SetThreadPriority(interpolationThreadHandle_, THREAD_PRIORITY_HIGHEST);
            SetThreadDescription(interpolationThreadHandle_, L"AMD FSR Interpolation Thread");
        }

        SetEvent(presentInfo_.interpolationEvent);
    }

    return interpolationThreadHandle_ != NULL;
}

void FrameInterpolationSwapChainDX12::discardOutstandingInterpolationCommandLists()
{
    // drop any outstanding interpolaton command lists
    for (int i = 0; i < _countof(registeredInterpolationCommandLists_); i++)
    {
        if (registeredInterpolationCommandLists_[i] != nullptr)
        {
            registeredInterpolationCommandLists_[i]->drop(true);
            registeredInterpolationCommandLists_[i] = nullptr;
        }
    }
}

void FrameInterpolationSwapChainDX12::setFrameGenerationConfig(FfxFrameGenerationConfig const* config)
{
    FFX_ASSERT(config);
    EnterCriticalSection(&criticalSection_);

    FfxPresentCallbackFunc inputPresentCallback = (nullptr != config->presentCallback) ? config->presentCallback : ffxFrameInterpolationUiComposition;
    ID3D12CommandQueue*    inputInterpolationQueue = config->allowAsyncWorkloads ? presentInfo_.asyncComputeQueue : presentInfo_.gameQueue;

    presentInterpolatedOnly_ = config->onlyPresentInterpolated;

    if (presentInfo_.interpolationQueue != inputInterpolationQueue)
    {
        waitForPresents();
        discardOutstandingInterpolationCommandLists();

        // change interpolation queue and reset fence value
        presentInfo_.interpolationQueue    = inputInterpolationQueue;
        interpolationFenceValue_            = 0;
        presentInfo_.interpolationQueue->Signal(presentInfo_.interpolationFence, interpolationFenceValue_);
    }

    if (interpolationEnabled_ != config->frameGenerationEnabled || 
        presentCallback_ != inputPresentCallback ||
        frameGenerationCallback_ != config->frameGenerationCallback)
    {
        waitForPresents();
        presentCallback_         = inputPresentCallback;
        frameGenerationCallback_ = config->frameGenerationCallback;

        // handle interpolation mode change
        if (interpolationEnabled_ != config->frameGenerationEnabled)
        {
            interpolationEnabled_ = config->frameGenerationEnabled;
            if (interpolationEnabled_)
            {
                frameInterpolationResetCondition_ = true;
                nextPresentWaitValue_             = framesSentForPresentation_;

                spawnPresenterThread();
            }
            else
            {
                killPresenterThread();
            }
        }
    }

    LeaveCriticalSection(&criticalSection_);
}

bool FrameInterpolationSwapChainDX12::destroyReplacementResources()
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&criticalSection_);

    waitForPresents();

    const bool recreatePresenterThread = interpolationThreadHandle_ != nullptr;
    if (recreatePresenterThread)
    {
        killPresenterThread();
    }

    discardOutstandingInterpolationCommandLists();

    {
        for (size_t i = 0; i < _countof(replacementSwapBuffers_); i++)
        {
            replacementSwapBuffers_[i].destroy();
        }
        SafeRelease(realBackBuffer0_);

        for (size_t i = 0; i < _countof(interpolationOutputs_); i++)
        {
            interpolationOutputs_[i].destroy();
        }
    }

    // reset counters used in buffer management
    framesSentForPresentation_        = 0;
    nextPresentWaitValue_             = 0;
    replacementSwapBufferIndex_       = 0;
    presentCount_                     = 0;
    interpolationFenceValue_          = 0;
    gameFenceValue_                   = 0;

    presentInfo_.gameFence->Signal(gameFenceValue_);
    presentInfo_.interpolationFence->Signal(interpolationFenceValue_);
    presentInfo_.presentFence->Signal(framesSentForPresentation_);
    presentInfo_.replacementBufferFence->Signal(framesSentForPresentation_);
    presentInfo_.compositionFence->Signal(framesSentForPresentation_);
    frameInterpolationResetCondition_ = true;

    if (recreatePresenterThread)
    {
        spawnPresenterThread();
    }

    discardOutstandingInterpolationCommandLists();

    LeaveCriticalSection(&criticalSection_);

    return SUCCEEDED(hr);
}

bool FrameInterpolationSwapChainDX12::waitForPresents()
{
    // wait for interpolation to finish
    waitForFenceValue(presentInfo_.gameFence, gameFenceValue_);
    waitForFenceValue(presentInfo_.interpolationFence, interpolationFenceValue_);
    waitForFenceValue(presentInfo_.presentFence, framesSentForPresentation_);

    return true;
}

FfxResource FrameInterpolationSwapChainDX12::interpolationOutput(int index)
{
    index = interpolationBufferIndex_;

    FfxResourceDescription interpolateDesc = GetFfxResourceDescriptionDX12(interpolationOutputs_[index].resource);
    return ffxGetResourceDX12(interpolationOutputs_[index].resource, interpolateDesc, nullptr, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
}

//IUnknown
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::QueryInterface(REFIID riid, void** ppvObject)
{
    const GUID guidReplacements[] = {
        __uuidof(this),
        IID_IUnknown,
        IID_IDXGIObject,
        IID_IDXGIDeviceSubObject,
        IID_IDXGISwapChain,
        IID_IDXGISwapChain1,
        IID_IDXGISwapChain2,
        IID_IDXGISwapChain3,
        IID_IDXGISwapChain4,
        IID_IFfxFrameInterpolationSwapChain
    };

    for (auto guid : guidReplacements)
    {
        if (IsEqualGUID(riid, guid) == TRUE)
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::AddRef()
{
    InterlockedIncrement(&refCount_);

    return refCount_;
}

ULONG STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::Release()
{
    ULONG ref = InterlockedDecrement(&refCount_);
    if (ref != 0)
    {
        return refCount_;
    }

    delete this;

    return 0;
}

// IDXGIObject
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return real()->SetPrivateData(Name, DataSize, pData);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return real()->SetPrivateDataInterface(Name, pUnknown);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return real()->GetPrivateData(Name, pDataSize, pData);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetParent(REFIID riid, void** ppParent)
{
    return real()->GetParent(riid, ppParent);
}

// IDXGIDeviceSubObject
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetDevice(REFIID riid, void** ppDevice)
{
    return real()->GetDevice(riid, ppDevice);
}

void FrameInterpolationSwapChainDX12::registerUiResource(FfxResource uiResource)
{
    EnterCriticalSection(&criticalSection_);

    presentInfo_.currentUiSurface = uiResource;

    LeaveCriticalSection(&criticalSection_);
}

void FrameInterpolationSwapChainDX12::presentPassthrough(UINT SyncInterval, UINT Flags)
{
    ID3D12Resource* dx12SwapchainBuffer    = nullptr;
    UINT            currentBackBufferIndex = presentInfo_.swapChain->GetCurrentBackBufferIndex();
    presentInfo_.swapChain->GetBuffer(currentBackBufferIndex, IID_PPV_ARGS(&dx12SwapchainBuffer));

    auto passthroughList = presentInfo_.commandPool.get(presentInfo_.presentQueue, L"passthroughList()");
    auto list            = passthroughList->reset();

    ID3D12Resource* dx12ResourceSrc = replacementSwapBuffers_[replacementSwapBufferIndex_].resource;
    ID3D12Resource* dx12ResourceDst = dx12SwapchainBuffer;

    D3D12_TEXTURE_COPY_LOCATION dx12SourceLocation = {};
    dx12SourceLocation.pResource                   = dx12ResourceSrc;
    dx12SourceLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dx12SourceLocation.SubresourceIndex            = 0;

    D3D12_TEXTURE_COPY_LOCATION dx12DestinationLocation = {};
    dx12DestinationLocation.pResource                   = dx12ResourceDst;
    dx12DestinationLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dx12DestinationLocation.SubresourceIndex            = 0;

    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource   = dx12ResourceSrc;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource   = dx12ResourceDst;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(_countof(barriers), barriers);

    list->CopyResource(dx12ResourceDst, dx12ResourceSrc);

    for (int i = 0; i < _countof(barriers); ++i)
    {
        D3D12_RESOURCE_STATES tmpStateBefore = barriers[i].Transition.StateBefore;
        barriers[i].Transition.StateBefore   = barriers[i].Transition.StateAfter;
        barriers[i].Transition.StateAfter    = tmpStateBefore;
    }

    list->ResourceBarrier(_countof(barriers), barriers);

    passthroughList->execute(true);

    presentInfo_.presentQueue->Signal(presentInfo_.replacementBufferFence, ++framesSentForPresentation_);

    setSwapChainBufferResourceInfo(presentInfo_.swapChain, false);
    presentInfo_.swapChain->Present(SyncInterval, Flags);

    presentInfo_.presentQueue->Signal(presentInfo_.presentFence, framesSentForPresentation_);
    presentInfo_.gameQueue->Wait(presentInfo_.presentFence, framesSentForPresentation_);

    SafeRelease(dx12SwapchainBuffer);
}

void FrameInterpolationSwapChainDX12::presentWithUiComposition(UINT SyncInterval, UINT Flags)
{
    auto uiCompositionList = presentInfo_.commandPool.get(presentInfo_.presentQueue, L"uiCompositionList()");
    auto list              = uiCompositionList->reset();

    ID3D12Resource* dx12SwapchainBuffer    = nullptr;
    UINT            currentBackBufferIndex = presentInfo_.swapChain->GetCurrentBackBufferIndex();
    presentInfo_.swapChain->GetBuffer(currentBackBufferIndex, IID_PPV_ARGS(&dx12SwapchainBuffer));

    FfxResourceDescription outBufferDesc = GetFfxResourceDescriptionDX12(dx12SwapchainBuffer);
    FfxResourceDescription inBufferDesc  = GetFfxResourceDescriptionDX12(replacementSwapBuffers_[replacementSwapBufferIndex_].resource);

    FfxPresentCallbackDescription desc{};
    desc.commandList                = ffxGetCommandListDX12(list);
    desc.device                     = presentInfo_.device;
    desc.isInterpolatedFrame        = false;
    desc.outputSwapChainBuffer      = ffxGetResourceDX12(dx12SwapchainBuffer, outBufferDesc, nullptr, FFX_RESOURCE_STATE_PRESENT);
    desc.currentBackBuffer          = ffxGetResourceDX12(replacementSwapBuffers_[replacementSwapBufferIndex_].resource, inBufferDesc, nullptr, FFX_RESOURCE_STATE_PRESENT);
    desc.currentUI                  = presentInfo_.currentUiSurface;
    presentCallback_(&desc);

    uiCompositionList->execute(true);

    presentInfo_.presentQueue->Signal(presentInfo_.replacementBufferFence, ++framesSentForPresentation_);

    setSwapChainBufferResourceInfo(presentInfo_.swapChain, false);
    presentInfo_.swapChain->Present(SyncInterval, Flags);

    presentInfo_.presentQueue->Signal(presentInfo_.presentFence, framesSentForPresentation_);
    presentInfo_.gameQueue->Wait(presentInfo_.presentFence, framesSentForPresentation_);

    SafeRelease(dx12SwapchainBuffer);
}

void FrameInterpolationSwapChainDX12::dispatchInterpolationCommands(FfxResource* pInterpolatedFrame, FfxResource* pRealFrame)
{
    FFX_ASSERT(pInterpolatedFrame);
    FFX_ASSERT(pRealFrame);
    
    const UINT             currentBackBufferIndex   = GetCurrentBackBufferIndex();
    ID3D12Resource*        pCurrentBackBuffer       = replacementSwapBuffers_[currentBackBufferIndex].resource;
    FfxResourceDescription gameFrameDesc            = GetFfxResourceDescriptionDX12(pCurrentBackBuffer);
    FfxResource backbuffer                          = ffxGetResourceDX12(replacementSwapBuffers_[currentBackBufferIndex].resource, gameFrameDesc, nullptr, FFX_RESOURCE_STATE_PRESENT);

    *pRealFrame = backbuffer;

    // interpolation queue must wait for output resource to become available
    presentInfo_.interpolationQueue->Wait(presentInfo_.compositionFence, interpolationOutputs_[interpolationBufferIndex_].availabilityFenceValue);

    auto pRegisteredCommandList = registeredInterpolationCommandLists_[currentBackBufferIndex];
    if (pRegisteredCommandList != nullptr)
    {
        pRegisteredCommandList->execute(true);

        presentInfo_.interpolationQueue->Signal(presentInfo_.interpolationFence, ++interpolationFenceValue_);

        *pInterpolatedFrame = interpolationOutput();
    }
    else {
        Dx12Commands* interpolationCommandList = presentInfo_.commandPool.get(presentInfo_.interpolationQueue, L"getInterpolationCommandList()");
        auto dx12CommandList = interpolationCommandList->reset();

        FfxFrameGenerationDispatchDescription desc{};
        desc.commandList = dx12CommandList;
        desc.outputs[0] = interpolationOutput();
        desc.presentColor = backbuffer;
        desc.reset = frameInterpolationResetCondition_;
        desc.numInterpolatedFrames = 1;
        desc.backBufferTransferFunction = static_cast<FfxBackbufferTransferFunction>(backBufferTransferFunction_);
        desc.minMaxLuminance[0] = minLuminance_;
        desc.minMaxLuminance[1] = maxLuminance_;

        if (frameGenerationCallback_(&desc) == FFX_OK)
        {
            interpolationCommandList->execute(true);

            presentInfo_.interpolationQueue->Signal(presentInfo_.interpolationFence, ++interpolationFenceValue_);
        }

        // reset condition if at least one frame was interpolated
        if (desc.numInterpolatedFrames > 0)
        {
            frameInterpolationResetCondition_ = false;
            *pInterpolatedFrame = interpolationOutput();
        }
    }
}

void FrameInterpolationSwapChainDX12::presentInterpolated(UINT SyncInterval, UINT Flags)
{
    const bool bVsync = SyncInterval > 0;

    // interpolation needs to wait for the game queue
    presentInfo_.gameQueue->Signal(presentInfo_.gameFence, ++gameFenceValue_);
    presentInfo_.interpolationQueue->Wait(presentInfo_.gameFence, gameFenceValue_);

    FfxResource interpolatedFrame{}, realFrame{};
    dispatchInterpolationCommands(&interpolatedFrame, &realFrame);

    EnterCriticalSection(&presentInfo_.criticalSectionScheduledFrame);

    PacingData entry{};
    entry.presentCallback                   = presentCallback_;
    entry.uiSurface                         = presentInfo_.currentUiSurface;
    entry.vsync                             = bVsync;
    entry.tearingSupported                  = tearingSupported_;
    entry.numFramesSentForPresentationBase  = framesSentForPresentation_;
    entry.gameFenceValue                    = gameFenceValue_;

    // interpolated
    PacingData::FrameInfo& fiInterpolated = entry.frames[PacingData::FrameIdentifier::Interpolated_1];
    if (interpolatedFrame.resource != nullptr)
    {
        fiInterpolated.doPresent                        = true;
        fiInterpolated.resource                         = interpolatedFrame;
        fiInterpolated.interpolationCompletedFenceValue = interpolationFenceValue_;
        fiInterpolated.presentIndex                     = ++framesSentForPresentation_;
    }

    // real
    if (!presentInterpolatedOnly_)
    {
        PacingData::FrameInfo& fiReal = entry.frames[PacingData::FrameIdentifier::Real];
        if (realFrame.resource != nullptr)
        {
            fiReal.doPresent    = true;
            fiReal.resource     = realFrame;
            fiReal.presentIndex = ++framesSentForPresentation_;
        }
    }

    entry.replacementBufferFenceSignal  = framesSentForPresentation_;
    entry.numFramesToPresent            = UINT32(framesSentForPresentation_ - entry.numFramesSentForPresentationBase);

    interpolationOutputs_[interpolationBufferIndex_].availabilityFenceValue = entry.numFramesSentForPresentationBase + fiInterpolated.doPresent;

    presentInfo_.scheduledInterpolations = entry;
    LeaveCriticalSection(&presentInfo_.criticalSectionScheduledFrame);

    // Set event to kick off async CPU present thread
    SetEvent(presentInfo_.presentEvent);

    // hold the replacement object back until previous frame or interpolated is presented
    nextPresentWaitValue_ = entry.numFramesSentForPresentationBase;
    
    UINT64 frameLatencyObjectWaitValue = (entry.numFramesSentForPresentationBase - 1) * (entry.numFramesSentForPresentationBase > 0);
    FFX_ASSERT(SUCCEEDED(presentInfo_.presentFence->SetEventOnCompletion(frameLatencyObjectWaitValue, replacementFrameLatencyWaitableObjectHandle_)));

}

// IDXGISwapChain1
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::Present(UINT SyncInterval, UINT Flags)
{
    if (Flags & DXGI_PRESENT_TEST)
    {
        return presentInfo_.swapChain->Present(SyncInterval, Flags);
    }

    EnterCriticalSection(&criticalSection_);

    const UINT currentBackBufferIndex = GetCurrentBackBufferIndex();

    // determine what present path to execute
    const bool fgCallbackConfigured = frameGenerationCallback_ != nullptr;
    const bool fgCommandListConfigured = registeredInterpolationCommandLists_[currentBackBufferIndex] != nullptr;
    const bool runInterpolation = interpolationEnabled_ && (fgCallbackConfigured || fgCommandListConfigured);

    if (runInterpolation)
    {
        WaitForSingleObject(presentInfo_.interpolationEvent, INFINITE);

        presentInfo_.gameQueue->Signal(presentInfo_.gameFence, ++gameFenceValue_);

        presentInterpolated(SyncInterval, Flags);
    }
    else
    {
        // if no interpolation, then we copied directly to the swapchain. Render UI, present and be done
        presentInfo_.gameQueue->Signal(presentInfo_.gameFence, ++gameFenceValue_);
        presentInfo_.presentQueue->Wait(presentInfo_.gameFence, gameFenceValue_);

        if (presentCallback_ != nullptr)
        {
            presentWithUiComposition(SyncInterval, Flags);
        }
        else
        {
            presentPassthrough(SyncInterval, Flags);
        }

        // respect game provided latency settings
        UINT64 frameLatencyObjectWaitValue = (framesSentForPresentation_ - gameMaximumFrameLatency_) * (framesSentForPresentation_ >= gameMaximumFrameLatency_);
        FFX_ASSERT(SUCCEEDED(presentInfo_.presentFence->SetEventOnCompletion(frameLatencyObjectWaitValue, replacementFrameLatencyWaitableObjectHandle_)));
    }

    replacementSwapBuffers_[currentBackBufferIndex].availabilityFenceValue = framesSentForPresentation_;

    // Unregister any potential command list
    registeredInterpolationCommandLists_[currentBackBufferIndex] = nullptr;
    presentCount_++;
    interpolationBufferIndex_                                                   = presentCount_ % _countof(interpolationOutputs_);

    //update active backbuffer and block when no buffer is available
    replacementSwapBufferIndex_ = presentCount_ % gameBufferCount_;

    LeaveCriticalSection(&criticalSection_);

    waitForFenceValue(presentInfo_.replacementBufferFence, replacementSwapBuffers_[replacementSwapBufferIndex_].availabilityFenceValue);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    EnterCriticalSection(&criticalSection_);

    HRESULT hr = E_FAIL;

    if (riid == IID_ID3D12Resource || riid == IID_ID3D12Resource1 || riid == IID_ID3D12Resource2)
    {
        if (verifyBackbufferDuplicateResources())
        {
            ID3D12Resource* pBuffer = replacementSwapBuffers_[Buffer].resource;

            pBuffer->AddRef();
            *ppSurface = pBuffer;

            hr = S_OK;
        }
    }

    LeaveCriticalSection(&criticalSection_);

    return hr;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
    return real()->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
    return real()->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
{
    HRESULT hr = real()->GetDesc(pDesc);

    // hide interpolation swapchaindesc to keep FI transparent for ISV
    if (SUCCEEDED(hr))
    {
        //update values we changed
        pDesc->BufferCount  = gameBufferCount_;
        pDesc->Flags        = gameFlags_;
        pDesc->SwapEffect   = gameSwapEffect_;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    destroyReplacementResources();

    EnterCriticalSection(&criticalSection_);

    const UINT fiAdjustedFlags = getInterpolationEnabledSwapChainFlags(SwapChainFlags);

    // update params expected by the application
    gameBufferCount_ = BufferCount;
    gameFlags_       = SwapChainFlags;

    HRESULT hr = real()->ResizeBuffers(0 /* preserve count */, Width, Height, NewFormat, fiAdjustedFlags);

    LeaveCriticalSection(&criticalSection_);

    return hr;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
    return real()->ResizeTarget(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetContainingOutput(IDXGIOutput** ppOutput)
{
    HRESULT hr = DXGI_ERROR_INVALID_CALL;

    if (ppOutput)
    {
        *ppOutput = getMostRelevantOutputFromSwapChain(real());
        hr        = S_OK;
    }
    
    return hr;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
    return real()->GetFrameStatistics(pStats);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetLastPresentCount(UINT* pLastPresentCount)
{
    return real()->GetLastPresentCount(pLastPresentCount);
}

// IDXGISwapChain1
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc)
{
    HRESULT hr = real()->GetDesc1(pDesc);

    // hide interpolation swapchaindesc to keep FI transparent for ISV
    if (SUCCEEDED(hr))
    {
        //update values we changed
        pDesc->BufferCount  = gameBufferCount_;
        pDesc->Flags        = gameFlags_;
        pDesc->SwapEffect   = gameSwapEffect_;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc)
{
    return real()->GetFullscreenDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetHwnd(HWND* pHwnd)
{
    return real()->GetHwnd(pHwnd);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetCoreWindow(REFIID refiid, void** ppUnk)
{
    return real()->GetCoreWindow(refiid, ppUnk);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    return Present(SyncInterval, PresentFlags);
}

BOOL STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::IsTemporaryMonoSupported(void)
{
    return real()->IsTemporaryMonoSupported();
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput)
{
    return real()->GetRestrictToOutput(ppRestrictToOutput);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetBackgroundColor(const DXGI_RGBA* pColor)
{
    return real()->SetBackgroundColor(pColor);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetBackgroundColor(DXGI_RGBA* pColor)
{
    return real()->GetBackgroundColor(pColor);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetRotation(DXGI_MODE_ROTATION Rotation)
{
    return real()->SetRotation(Rotation);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetRotation(DXGI_MODE_ROTATION* pRotation)
{
    return real()->GetRotation(pRotation);
}

// IDXGISwapChain2
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetSourceSize(UINT Width, UINT Height)
{
    return real()->SetSourceSize(Width, Height);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetSourceSize(UINT* pWidth, UINT* pHeight)
{
    return real()->GetSourceSize(pWidth, pHeight);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetMaximumFrameLatency(UINT MaxLatency)
{
    // store value, so correct value is returned if game asks for it
    gameMaximumFrameLatency_ = MaxLatency;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetMaximumFrameLatency(UINT* pMaxLatency)
{
    if (pMaxLatency)
    {
        *pMaxLatency = gameMaximumFrameLatency_;
    }

    return pMaxLatency ? S_OK : DXGI_ERROR_INVALID_CALL;
}

HANDLE STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetFrameLatencyWaitableObject(void)
{
    return replacementFrameLatencyWaitableObjectHandle_;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix)
{
    return real()->SetMatrixTransform(pMatrix);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetMatrixTransform(DXGI_MATRIX_3X2_F* pMatrix)
{
    return real()->GetMatrixTransform(pMatrix);
}

// IDXGISwapChain3
UINT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::GetCurrentBackBufferIndex(void)
{
    EnterCriticalSection(&criticalSection_);

    UINT result = (UINT)replacementSwapBufferIndex_;

    LeaveCriticalSection(&criticalSection_);

    return result;
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT* pColorSpaceSupport)
{
    return real()->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace)
{
    switch (ColorSpace)
    {
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
        backBufferTransferFunction_ = FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
        break;
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        backBufferTransferFunction_ = FFX_BACKBUFFER_TRANSFER_FUNCTION_PQ;
        break;
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        backBufferTransferFunction_ = FFX_BACKBUFFER_TRANSFER_FUNCTION_SCRGB;
        break;
    default:
        break;
    }

    return real()->SetColorSpace1(ColorSpace);
}

HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::ResizeBuffers1(
    UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue)
{
    FFX_ASSERT_MESSAGE(false, "AMD FSR Frame interpolaton proxy swapchain: ResizeBuffers1 currently not supported.");

    return S_OK;
}

// IDXGISwapChain4
HRESULT STDMETHODCALLTYPE FrameInterpolationSwapChainDX12::SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void* pMetaData)
{
    if (Size > 0 && pMetaData != nullptr)
    {
        DXGI_HDR_METADATA_HDR10* HDR10MetaData = NULL;

        switch (Type)
        {
        case DXGI_HDR_METADATA_TYPE_NONE:
            break;
        case DXGI_HDR_METADATA_TYPE_HDR10:
            HDR10MetaData = static_cast<DXGI_HDR_METADATA_HDR10*>(pMetaData);
            break;
        case DXGI_HDR_METADATA_TYPE_HDR10PLUS:
            break;
        }

        FFX_ASSERT_MESSAGE(HDR10MetaData != NULL, "FSR3 Frame interpolaton pxory swapchain: could not initialize HDR metadata");

        if (HDR10MetaData)
        {
            minLuminance_ = HDR10MetaData->MinMasteringLuminance / 10000.0f;
            maxLuminance_ = float(HDR10MetaData->MaxMasteringLuminance);
        }
    }

    return real()->SetHDRMetaData(Type, Size, pMetaData);
}

ID3D12GraphicsCommandList* FrameInterpolationSwapChainDX12::getInterpolationCommandList()
{
    EnterCriticalSection(&criticalSection_);

    ID3D12GraphicsCommandList* dx12CommandList = nullptr;

    // store active backbuffer index to the command list, used to verify list usage later
    if (interpolationEnabled_) {
        ID3D12Resource* currentBackBuffer = nullptr;
        const UINT      currentBackBufferIndex = GetCurrentBackBufferIndex();
        if (SUCCEEDED(GetBuffer(currentBackBufferIndex, IID_PPV_ARGS(&currentBackBuffer))))
        {
            Dx12Commands* registeredCommands = registeredInterpolationCommandLists_[currentBackBufferIndex];

            // drop if already existing
            if (registeredCommands != nullptr)
            {
                registeredCommands->drop(true);
                registeredCommands = nullptr;
            }

            registeredCommands = presentInfo_.commandPool.get(presentInfo_.interpolationQueue, L"getInterpolationCommandList()");
            FFX_ASSERT(registeredCommands);

            dx12CommandList = registeredCommands->reset();

            registeredInterpolationCommandLists_[currentBackBufferIndex] = registeredCommands;

            SafeRelease(currentBackBuffer);
        }
    }
    
    LeaveCriticalSection(&criticalSection_);

    return dx12CommandList;
}
