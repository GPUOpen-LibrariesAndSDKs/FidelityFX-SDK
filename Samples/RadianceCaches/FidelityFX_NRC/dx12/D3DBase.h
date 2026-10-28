// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
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

#include <windows.h>
#include <windowsx.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <stdexcept>
#include <string>
#include <wrl.h>

#include "utils/Assert.h"

namespace fsr
{            
    using XMFLOAT3 = DirectX::XMFLOAT3;
    using XMFLOAT4 = DirectX::XMFLOAT4;
    using XMFLOAT2 = DirectX::XMFLOAT2;
    using XMUINT2 = DirectX::XMUINT2;
        
    struct Vertex
    {
        Vertex(const XMFLOAT3& p, const XMFLOAT4& c) : position(p), color(c) {}
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    struct VertexUV
    {
        VertexUV(const XMFLOAT3& p, const XMFLOAT2& u) : position(p), uv(u) {}
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };
    
    // Note that while ComPtr is used to manage the lifetime of resources on the CPU,
    // it has no understanding of the lifetime of resources on the GPU. Apps must account
    // for the GPU lifetime of resources to avoid destroying objects that may still be
    // referenced by the GPU.
    template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    inline std::string HrToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }

    class HrException : public std::runtime_error
    {
    public:
        HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
        HRESULT Error() const { return m_hr; }
    private:
        const HRESULT m_hr;
    };

#define SAFE_RELEASE(p) if (p) (p)->Release()

    inline void ThrowIfFailed(HRESULT hr)
    {       
        if (FAILED(hr))
        {
            throw HrException(hr);
        }
    }

    // Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
    {
        WCHAR fullName[50];
        if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
        {
            pObject->SetName(fullName);
        }
    }
#else
    inline void SetName(ID3D12Object*, LPCWSTR)
    {
    }
    inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
    {
    }
#endif

    // Naming helper for ComPtr<T>.
    // Assigns the name of the variable as the name of the object.
    // The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

    inline UINT CalculateConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffer size is required to be aligned.
        return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    }

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
    inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target)
    {
        UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr;

        Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

        if (errors != nullptr)
        {
            OutputDebugStringA((char*)errors->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        return byteCode;
    }
#endif

    // Resets all elements in a ComPtr array.
    template<class T>
    void ResetComPtrArray(T* comPtrArray)
    {
        for (auto& i : *comPtrArray)
        {
            i.Reset();
        }
    }


    // Resets all elements in a unique_ptr array.
    template<class T>
    void ResetUniquePtrArray(T* uniquePtrArray)
    {
        for (auto& i : *uniquePtrArray)
        {
            i.reset();
        }
    }

    template<typename T>
    inline void ReleaseResource(ComPtr<T>& resource)
    {
        if (resource)
        {
            resource.Reset();
        }
    }

    template<typename T>
    inline void ReleaseResource(T*& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }

    // RAII-scoped transition barrier to temporarily flip objects into the required state
    class ScopedTransitionBarrier
    {
    public:
        ScopedTransitionBarrier(ComPtr<ID3D12GraphicsCommandList>& commandList, std::initializer_list<ComPtr<ID3D12Resource>> d3dResources, const D3D12_RESOURCE_STATES baseState, const D3D12_RESOURCE_STATES scopedState) :
            m_commandList(commandList), 
            m_d3dResources(d3dResources),
            m_baseState(baseState),
            m_scopedState(scopedState)
        {
            for(auto& resource : m_d3dResources)
            {
                CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), baseState, scopedState);
                m_commandList->ResourceBarrier(1, &resBarrier);
            }
        }   

        ScopedTransitionBarrier(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12Resource>& d3dResources, const D3D12_RESOURCE_STATES baseState, const D3D12_RESOURCE_STATES scopedState) :
            ScopedTransitionBarrier(commandList, { d3dResources }, baseState, scopedState) {}

        ~ScopedTransitionBarrier()
        {
            for (auto& resource : m_d3dResources)
            {
                CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), m_scopedState, m_baseState);
                m_commandList->ResourceBarrier(1, &resBarrier);
            }
        }   

    private:
        ComPtr<ID3D12GraphicsCommandList>& m_commandList;
        std::vector<ComPtr<ID3D12Resource>> m_d3dResources;
        const D3D12_RESOURCE_STATES m_baseState;
        const D3D12_RESOURCE_STATES m_scopedState;
    };
}
