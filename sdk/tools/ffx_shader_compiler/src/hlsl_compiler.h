// This file is part of the FidelityFX SDK.
//
// Copyright © 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "compiler.h"


typedef HRESULT (*pD3DGetBlobPart)
    (LPCVOID pSrcData,
     SIZE_T SrcDataSize,
     D3D_BLOB_PART Part,
     UINT Flags,
     ID3DBlob** ppPart);

typedef HRESULT (*pD3DReflect)
    (LPCVOID pSrcData,
     SIZE_T SrcDataSize,
     REFIID pInterface,
     void** ppReflector);

/// The DXC (HLSL) specialization of <c><i>IShaderBinary</i></c> interface.
/// Handles everything necessary to export DXC compiled binary shader data.
///
/// @ingroup ShaderCompiler
struct HLSLDxcShaderBinary : public IShaderBinary
{
    CComPtr<IDxcResult> pResults;
    CComPtr<IDxcBlob>   pShader;

    uint8_t* BufferPointer() override;
    size_t   BufferSize() override;
};

/// The FXC (HLSL) specialization of <c><i>IShaderBinary</i></c> interface.
/// Handles everything necessary to export DXC compiled binary shader data.
///
/// @ingroup ShaderCompiler
struct HLSLFxcShaderBinary : public IShaderBinary
{
    CComPtr<ID3DBlob> pShader;

    uint8_t* BufferPointer() override;
    size_t   BufferSize() override;
};

/// The HLSLCompiler specialization of <c><i>ICompiler</i></c> interface.
/// Handles everything necessary to compile and extract shader reflection data
/// for HSLS and then exports the binary and reflection data for consumption
/// by HLSL-specific backends.
///
/// @ingroup ShaderCompiler
class HLSLCompiler : public ICompiler
{
public:
    enum Backend
    {
        DXC,
        FXC
    };

public:
    HLSLCompiler(Backend            backend,
                 const std::string& dll,
                 const std::string& shaderPath,
                 const std::string& shaderName,
                 const std::string& shaderFileName,
                 const std::string& outputPath,
                 bool               disableLogs);
    ~HLSLCompiler();
    bool Compile(Permutation&                    permutation,
                 const std::vector<std::string>& arguments,
                 std::mutex&                     writeMutex) override;
    bool ExtractReflectionData(Permutation& permutation)                              override;
    void WriteBinaryHeaderReflectionData(FILE* fp, const Permutation& permutation, std::mutex& writeMutex) override;
    void WritePermutationHeaderReflectionStructMembers(FILE* fp) override;
    void WritePermutationHeaderReflectionData(FILE* fp, const Permutation& permutation) override;

private:
    bool CompileDXC(Permutation&                    permutation,
                    const std::vector<std::string>& arguments,
                    std::mutex&                     writeMutex);

    bool CompileFXC(Permutation&                    permutation,
                    const std::vector<std::string>& arguments,
                    std::mutex&                     writeMutex);

    bool ExtractDXCReflectionData(Permutation& permutation);
    bool ExtractFXCReflectionData(Permutation& permutation);

private:
    Backend                     m_backend;
    std::string                 m_Source;

    // DXC backend
    CComPtr<IDxcUtils>          m_DxcUtils;
    CComPtr<IDxcCompiler3>      m_DxcCompiler;
    CComPtr<IDxcIncludeHandler> m_DxcDefaultIncludeHandler;
    DxcCreateInstanceProc       m_DxcCreateInstanceFunc;

    // FXC backend
    pD3DCompile                 m_FxcD3DCompile;
    pD3DGetBlobPart             m_FxcD3DGetBlobPart;
    pD3DReflect                 m_FxcD3DReflect;

    HMODULE                     m_DllHandle;
};
