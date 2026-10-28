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

#include "utils/Json.h"
#include "D3DBase.h"

#include <directx-dxc/dxcapi.h>
#include <directx-dxc/d3d12shader.h>

namespace fsr
{
	class DXC
	{
	private:
		struct ErrorData
		{
			ErrorData() = default;
			ErrorData(const int t) : type(t) {}

			enum : int { kWarning, kError };

			int type;
			int lineNumber;
			std::string filePath;
			std::string summary;
			std::string details;
			std::string errorBlob;
		};

	public:
		DXC(Json& json);
		~DXC();

		enum : int { kSucceeded, kSucceededWithWarnings, kFailed };

		int CompileFromFile(const std::string& path, const std::wstring& entryPoint, const std::wstring& target, ComPtr<IDxcBlob>& blob);
		int CompileFromSource(const std::string& source, const std::wstring& entryPoint, const std::wstring& target, ComPtr<IDxcBlob>& blob);
		void SetIncludeDirectories(const std::vector<std::string> includeDirs);

	private:
		int CompileBlob(ComPtr<IDxcBlobEncoding>& sourceBlob, const std::wstring& entryPoint, const std::wstring& target, const std::wstring& rootDir, ComPtr<IDxcBlob>& blob);
		std::pair<int, int> ParseErrors(const std::string& errorBlob, std::vector<ErrorData>&);

	private:

		ComPtr<IDxcUtils>			m_utils;
		ComPtr<IDxcCompiler3>		m_compiler;
		ComPtr<IDxcIncludeHandler>	m_includeHandler;
		std::vector<std::wstring>	m_fileIncludeDirs;

		bool						m_ignoreWarnings;
		bool						m_warningsAsErrors;
		bool						m_debugMode;
	};
}
