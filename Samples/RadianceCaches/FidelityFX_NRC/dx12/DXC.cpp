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

#include "DXC.h"
#include "utils/Log.h"
#include "utils/Json.h"
#include "utils/StringUtils.h"
#include "utils/FilesystemUtils.h"

#include <string>
#include <regex>

#include <OpenSource/nlohmann/json.hpp>

namespace fsr
{
    DXC::DXC(Json& json) :
        m_ignoreWarnings(false),
        m_warningsAsErrors(false),
        m_debugMode(false)
    {
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
        m_utils->CreateDefaultIncludeHandler(&m_includeHandler);

        if (json->contains("ignoreWarnings")) { m_ignoreWarnings = json["ignoreWarnings"]; }
        if (json->contains("warningsAsErrors")) { m_warningsAsErrors = json["warningsAsErrors"]; }
        if (json->contains("debugMode")) { m_debugMode = json["debugMode"]; }

    }

    DXC::~DXC()
    {
        ReleaseResource(m_includeHandler);
        ReleaseResource(m_compiler);
        ReleaseResource(m_utils);
    }

    void DXC::SetIncludeDirectories(const std::vector<std::string> includeDirs)
    {
        m_fileIncludeDirs.clear();
        for (auto& dir : includeDirs)
        {
            m_fileIncludeDirs.push_back(Widen(dir));
        }
    }

    int DXC::CompileFromFile(const std::string& path, const std::wstring& entryPoint, const std::wstring& target, ComPtr<IDxcBlob>& shaderBlob)
    {
        ComPtr<IDxcBlobEncoding> sourceBlob;
        AssertMsg(SUCCEEDED(m_utils->LoadFile(Widen(path).c_str(), nullptr, &sourceBlob)), "Failed to load shader source file '{}'", path.c_str());

        return CompileBlob(sourceBlob, entryPoint, target, Widen(GetParentDirectory(path)), shaderBlob);
    }

    int DXC::CompileFromSource(const std::string& source, const std::wstring& entryPoint, const std::wstring& target, ComPtr<IDxcBlob>& shaderBlob)
    {
        ComPtr<IDxcBlobEncoding> sourceBlob;
        m_utils->CreateBlob(source.c_str(), UINT32(source.length()), DXC_CP_ACP, &sourceBlob);

        return CompileBlob(sourceBlob, entryPoint, target, L"", shaderBlob);
    }

    std::pair<int, int> DXC::ParseErrors(const std::string& errorBlob, std::vector<ErrorData>& errorList)
    {
        std::istringstream stream(errorBlob);
        std::string line;
        int numWarnings = 0, numErrors = 0;

        while (std::getline(stream, line))
        {
            if (Lowercase(line.substr(0, 7)) == "in file") { continue; }

            if (line.find("warning:") != std::string::npos || line.find("error:") != std::string::npos)
            {
                if (line.find("error:") != std::string::npos)
                {
                    errorList.push_back(ErrorData(ErrorData::kError));
                    ++numErrors;
                }
                else
                {
                    errorList.push_back(ErrorData(ErrorData::kWarning));
                    ++numWarnings;
                }

                std::smatch match;
                std::regex pattern("(.+):([0-9]+):([0-9]+): (warning|error): (.+)");
                if (std::regex_search(line, match, pattern))
                {
                    std::sregex_token_iterator iter(line.begin(), line.end(), pattern, { 1, 2, 5 });
                    std::sregex_token_iterator end;
                    int idx = 0;
                    while (iter != end)
                    {
                        switch (idx++)
                        {
                        case 0: errorList.back().filePath = *iter; break;
                        case 1: errorList.back().lineNumber = std::stoi(*iter); break;
                        case 2: errorList.back().summary = *iter; break;
                        };
                        ++iter;
                    }
                }
            }
            else if (!errorList.empty() && !line.empty())
            {
                errorList.back().details += "          " + line + "\n";
            }

            errorList.back().errorBlob += line + "\n";

        }
        return { numErrors, numWarnings };
    }

    int DXC::CompileBlob(ComPtr<IDxcBlobEncoding>& sourceBlob, const std::wstring& entryPoint, const std::wstring& target, const std::wstring& rootDir, ComPtr<IDxcBlob>& shaderBlob)
    {
        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        // Set compiler arguments
        std::vector<LPCWSTR> arguments = {
            L"-E", entryPoint.c_str(),
            L"-T", target.c_str(),
            DXC_ARG_ALL_RESOURCES_BOUND
        };

        if (m_warningsAsErrors) { arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); }

        // Append file root directory
        if (!rootDir.empty())
        {
            arguments.push_back(L"-I");
            arguments.push_back(rootDir.c_str());
        }

        // Append file include directories
        for (auto& dir : m_fileIncludeDirs)
        {
            arguments.push_back(L"-I");
            arguments.push_back(dir.c_str());
        }

        // Switch to debug mode
        #if defined(_DEBUG)
            //arguments.push_back(DXC_ARG_DEBUG);
            //arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
        #endif 

        // Compile
        ComPtr<IDxcResult> result;
        ThrowIfFailed(m_compiler->Compile(&sourceBuffer, arguments.data(), UINT32(arguments.size()), m_includeHandler.Get(), IID_PPV_ARGS(&result)));

        if (!result.Get())
        {
            Log::Error("DXC returned null object.");
            return kFailed;
        }

        // Display error data
        ComPtr<IDxcBlobUtf8> errorData;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorData), nullptr);
        if (errorData && errorData->GetBufferSize())
        {
            const std::string errorStr((const char*)errorData->GetBufferPointer(), errorData->GetBufferSize());
            errorData->Release();

            std::vector<ErrorData> parsedErrors;
            auto [numErrors, numWarnings] = ParseErrors(errorStr, parsedErrors);

            if (numErrors > 0 || (numWarnings > 0 && !m_ignoreWarnings))
            {
                Log::Warning("\n");
                Log::Warning("Shader compilation complete: {} errors, {} warnings:", numErrors, numWarnings);
                for (auto& error : parsedErrors)
                {
                    const std::string errorDetail = std::format("{}:{}: {}", error.filePath, error.lineNumber, error.summary);
                    if (!m_ignoreWarnings && error.type == ErrorData::kWarning) { Log::Warning("  - Warning: {}", errorDetail); Log::Write(error.details); }
                    if (error.type == ErrorData::kError) { Log::Error("  - Error: {}", errorDetail); Log::Write(error.details); }
                }
            }
        }

        // Return if compilation failed
        HRESULT hr;
        if (FAILED(result->GetStatus(&hr)) || FAILED(hr))
        {
            return kFailed;
        }

        // Get compiled shader bytecode
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        return (errorData && errorData->GetBufferSize()) ? kSucceededWithWarnings : kSucceeded;
    }
}
