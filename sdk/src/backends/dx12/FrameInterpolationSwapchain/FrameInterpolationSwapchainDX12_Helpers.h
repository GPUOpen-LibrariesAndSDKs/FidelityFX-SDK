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

#include <FidelityFX/host/ffx_assert.h>

IDXGIFactory*           getDXGIFactoryFromSwapChain(IDXGISwapChain* swapChain);
bool                    isExclusiveFullscreen(IDXGISwapChain* swapChain);
void                    waitForPerformanceCount(const int64_t targetCount);
bool                    waitForFenceValue(ID3D12Fence* fence, UINT64 value, DWORD dwMilliseconds = INFINITE);
bool                    isTearingSupported(IDXGIFactory* dxgiFactory);
bool                    getMonitorLuminanceRange(IDXGISwapChain* swapChain, float* outMinLuminance, float* outMaxLuminance);
inline bool             isValidHandle(HANDLE handle);
IDXGIOutput6*           getMostRelevantOutputFromSwapChain(IDXGISwapChain* swapChain);

    // Safe release for interfaces
template<class Interface>
inline UINT SafeRelease(Interface*& pInterfaceToRelease)
{
    UINT refCount = -1;
    if (pInterfaceToRelease != nullptr)
    {
        refCount = pInterfaceToRelease->Release();

        pInterfaceToRelease = nullptr;
    }

    return refCount;
}

inline void SafeCloseHandle(HANDLE& handle)
{
    if (handle)
    {
        CloseHandle(handle);
        handle = 0;
    }
}

class Dx12Commands
{
    ID3D12CommandQueue*        queue_               = nullptr;
    ID3D12CommandAllocator*    allocator_           = nullptr;
    ID3D12GraphicsCommandList* list_                = nullptr;
    ID3D12Fence*               fence_               = nullptr;
    UINT64                     availableFenceValue_ = 0;

public:
    void release()
    {
        SafeRelease(allocator_);
        SafeRelease(list_);
        SafeRelease(fence_);
    }

public:
    ~Dx12Commands()
    {
        release();
    }

    bool initiated()
    {
        return allocator_ != nullptr;
    }

    bool verify(ID3D12CommandQueue* pQueue)
    {
        HRESULT hr = initiated() ? S_OK : E_FAIL;

        if (FAILED(hr))
        {
            ID3D12Device* device = nullptr;
            if (SUCCEEDED(pQueue->GetDevice(IID_PPV_ARGS(&device))))
            {
                D3D12_COMMAND_QUEUE_DESC queueDesc = pQueue->GetDesc();
                if (SUCCEEDED(device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&allocator_))))
                {
                    allocator_->SetName(L"Dx12CommandPool::Allocator");
                    if (SUCCEEDED(device->CreateCommandList(queueDesc.NodeMask, queueDesc.Type, allocator_, nullptr, IID_PPV_ARGS(&list_))))
                    {
                        allocator_->SetName(L"Dx12CommandPool::Commandlist");
                        list_->Close();

                        if (SUCCEEDED(device->CreateFence(availableFenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_))))
                        {
                            hr = S_OK;
                        }
                    }
                }

                SafeRelease(device);
            }
        }

        if (FAILED(hr))
        {
            release();
        }

        return SUCCEEDED(hr);
    }

    void occupy(ID3D12CommandQueue* pQueue, const wchar_t* name)
    {
        availableFenceValue_++;
        queue_ = pQueue;
        allocator_->SetName(name);
        list_->SetName(name);
        fence_->SetName(name);
    }

    ID3D12GraphicsCommandList* reset()
    {
        if (SUCCEEDED(allocator_->Reset()))
        {
            if (SUCCEEDED(list_->Reset(allocator_, nullptr)))
            {
            }
        }

        return list_;
    }

    ID3D12GraphicsCommandList* list()
    {
        return list_;
    }

    void execute(bool listIsOpen = false)
    {
        if (listIsOpen)
        {
            list_->Close();
        }

        ID3D12CommandList* pListsToExec[] = {list_};
        queue_->ExecuteCommandLists(_countof(pListsToExec), pListsToExec);
        queue_->Signal(fence_, availableFenceValue_);
    }

    void drop(bool listIsOpen = false)
    {
        if (listIsOpen)
        {
            list()->Close();
        }
        queue_->Signal(fence_, availableFenceValue_);
    }

    bool available()
    {
        return fence_->GetCompletedValue() >= availableFenceValue_;
    }
};

template <size_t Capacity>
class Dx12CommandPool
{
public:

private:
    CRITICAL_SECTION criticalSection_{};
    Dx12Commands     buffer[4 /* D3D12_COMMAND_LIST_TYPE_COPY == 3 */][Capacity] = {};

public:

    Dx12CommandPool()
    {
        InitializeCriticalSection(&criticalSection_);
    }

    ~Dx12CommandPool()
    {
        EnterCriticalSection(&criticalSection_);

        for (size_t type = 0; type < 4; type++)
        {
            for (size_t idx = 0; idx < Capacity; idx++)
            {
                auto& cmds = buffer[type][idx];
                while (cmds.initiated() && !cmds.available())
                {
                    // wait for list to be idling
                }
                cmds.release();
            }
        }

        LeaveCriticalSection(&criticalSection_);

        DeleteCriticalSection(&criticalSection_);
    }

    Dx12Commands* get(ID3D12CommandQueue* pQueue, const wchar_t* name)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = pQueue->GetDesc();

        EnterCriticalSection(&criticalSection_);

        Dx12Commands* pCommands = nullptr;
        for (size_t idx = 0; idx < Capacity && (pCommands == nullptr); idx++)
        {
            auto& cmds = buffer[queueDesc.Type][idx];
            if (cmds.verify(pQueue) && cmds.available())
            {
                pCommands = &cmds;
            }
        }

        FFX_ASSERT(pCommands);

        pCommands->occupy(pQueue, name);
        LeaveCriticalSection(&criticalSection_);

        return pCommands;
    }
};

template <const int Size, typename Type = double>
struct SimpleMovingAverage
{
    Type                    history[Size] = {};
    unsigned int            idx           = 0;
    unsigned int            updateCount   = 0;

    Type getAverage()
    {
        Type          average    = 0.f;
        unsigned int  iterations = (updateCount >= Size) ? Size : updateCount;
        
        if (iterations > 0)
        {
            for (size_t i = 0; i < iterations; i++)
            {
                average += history[i];
            }
            average /= iterations;
        }

        return average;
    }

    void reset()
    {
        updateCount = 0;
        idx         = 0;
    }

    void update(Type newValue)
    {
        history[idx] = newValue;
        idx          = (idx + 1) % Size;
        updateCount++;
    }
};
