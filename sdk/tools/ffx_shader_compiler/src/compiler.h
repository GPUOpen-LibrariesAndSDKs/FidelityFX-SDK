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

#include "pch.hpp"

/// @defgroup ShaderCompiler Shader Compiler
/// Documentation for the FidelityFX Shader Compiler tool
/// 
/// @ingroup ffxSDK

/// A structure encapsulating a platform agnostic shader binary interface.
/// Override for each language representation needed (i.e. HLSL, GLSL, etc.)
///
/// @ingroup ShaderCompiler
struct IShaderBinary
{
    IShaderBinary()
    {
    }
    virtual ~IShaderBinary()
    {
    }

    virtual uint8_t* BufferPointer() = 0;
    virtual size_t   BufferSize()    = 0;
};

/// A structure defining an element of shader reflection data to be exported.
///
/// @ingroup ShaderCompiler
struct ShaderResourceInfo
{
    std::string name;
    uint32_t    binding;
    uint32_t    count;
    uint32_t    space;
};

/// A structure defining the reflection data to be exported. ReflectionData is 
/// composed of a number of <c><i>ShaderResourceInfo</i></c> structs for all required
/// resource types.
///
/// @ingroup ShaderCompiler
struct IReflectionData
{
    IReflectionData()
    {
    }
    virtual ~IReflectionData()
    {
    }

    std::vector<ShaderResourceInfo> constantBuffers;            ///< Constant buffer resource reflection data representation.
    std::vector<ShaderResourceInfo> srvTextures;                ///< SRV-based texture resource reflection data representation.
    std::vector<ShaderResourceInfo> uavTextures;                ///< UAV-based texture resource reflection data representation.
    std::vector<ShaderResourceInfo> srvBuffers;                 ///< SRV-based buffer resource reflection data representation.
    std::vector<ShaderResourceInfo> uavBuffers;                 ///< UAV-based buffer resource reflection data representation.
    std::vector<ShaderResourceInfo> samplers;                   ///< Sampler resource reflection data representation (currently unused).
    std::vector<ShaderResourceInfo> rtAccelerationStructures;   ///< Acceleration structure resource reflection data representation.
};

/// A structure defining a shader permutation representation. Each permutation compiled
/// generates this structure for export.
///
/// @ingroup ShaderCompiler
struct Permutation
{
    uint32_t                            key = 0;                    ///< Shader permutation key identifier.
    std::string                         hashDigest;                 ///< Shader permutation hash key.
    std::string                         name;                       ///< Shader permutation name.
    std::string                         headerFileName;             ///< Shader permutation header file name.
    std::vector<std::wstring>           defines;                    ///< Shader permutation defines.
    std::shared_ptr<IShaderBinary>      shaderBinary = nullptr;     ///< Shader permutation compiled binary data.
    std::shared_ptr<IReflectionData>    reflectionData = nullptr;   ///< Shader permutation <c><i>IReflectionData</i></c> data.

    fs::path                            sourcePath;                 ///< Shader source file path for this permutation.    
    std::unordered_set<std::string>     dependencies;               ///< List of shader dependencies for this permutation.
};

/// A structure defining the compiler interface. Should be sub-classed for each
/// language supported (i.e. HLSL, GLSL, etc.)
///
/// @ingroup ShaderCompiler
class ICompiler
{
protected:
    std::string m_ShaderPath;
    std::string m_ShaderName;
    std::string m_ShaderFileName;
    std::string m_OutputPath;
    bool        m_DisableLogs;

public:
    ICompiler(const std::string& shaderPath, const std::string& shaderName, const std::string& shaderFileName, const std::string& outputPath, bool disableLogs)
        : m_ShaderPath(shaderPath)
        , m_ShaderName(shaderName)
        , m_ShaderFileName(shaderFileName)
        , m_OutputPath(outputPath)
        , m_DisableLogs(disableLogs)
    {
    }
    virtual ~ICompiler()
    {
    }
    virtual bool Compile(Permutation&                    permutation,
                         const std::vector<std::string>& arguments,
                         std::mutex&                     writeMutex)                          = 0;
    virtual bool ExtractReflectionData(Permutation& permutation)                              = 0;
    virtual void WriteBinaryHeaderReflectionData(FILE* fp, const Permutation& permutation, std::mutex& writeMutex) = 0;
    virtual void WritePermutationHeaderReflectionStructMembers(FILE* fp)                                           = 0;
    virtual void WritePermutationHeaderReflectionData(FILE* fp, const Permutation& permutation)                    = 0;
};
