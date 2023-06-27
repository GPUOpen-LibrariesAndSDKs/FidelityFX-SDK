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


/// The GLSL (GSLang) specialization of <c><i>IShaderBinary</i></c> interface.
/// Handles everything necessary to export DXC compiled binary shader data.
///
/// @ingroup ShaderCompiler
struct GLSLShaderBinary : public IShaderBinary
{
    std::vector<uint8_t> spirv;

    uint8_t* BufferPointer() override;
    size_t   BufferSize() override;
};


/// The GLSLCompiler specialization of <c><i>ICompiler</i></c> interface.
/// Handles everything necessary to compile and extract shader reflection data
/// for GSLS and then exports the binary and reflection data for consumption
/// by GLSL-specific backends.
///
/// @ingroup ShaderCompiler
class GLSLCompiler : public ICompiler
{
public:
    GLSLCompiler(const std::string& glslangExe,
                 const std::string& shaderPath,
                 const std::string& shaderName,
                 const std::string& shaderFileName,
                 const std::string& outputPath,
                 bool               disableLogs);
    ~GLSLCompiler();
    bool Compile(Permutation& permutation, const std::vector<std::string>& arguments, std::mutex& writeMutex) override;
    bool ExtractReflectionData(Permutation& permutation) override;
    void WriteBinaryHeaderReflectionData(FILE* fp, const Permutation& permutation, std::mutex& writeMutex) override;
    void WritePermutationHeaderReflectionStructMembers(FILE* fp) override;
    void WritePermutationHeaderReflectionData(FILE* fp, const Permutation& permutation) override;

private:
    std::string m_GlslangExe;
};
