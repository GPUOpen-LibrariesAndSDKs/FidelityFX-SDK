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

#include "Json.h"

#include <fstream>
#include <OpenSource/nlohmann/json.hpp>
#include "FilesystemUtils.h"
#include "Assert.h"
#include "Log.h"

namespace fsr
{
	using json = nlohmann::json;
	
	Json::Json(Json& parent, nlohmann::json& node) :
		m_document(parent.m_document),
		m_node(&node)
	{

	}

	Json::Json(const std::string& filePath)
	{
		std::string resolvedPath = filePath;
		if(!IsAbsolutePath(resolvedPath) || !FileExists(resolvedPath))
		{
			resolvedPath = GetModuleDirectory() + "\\" + filePath;
			AssertFmt(FileExists(resolvedPath), "Error: Json file '{}' does not exist.", filePath.c_str());
		}

		const std::string rawInput = ReadTextFile(resolvedPath);
		try
		{
			m_document.reset(new json);
			*m_document = json::parse(rawInput);
		}
		catch (const std::exception& err)
		{
			Log::Error("Error: loading failed: {}", err.what());
			m_document.reset();
		}

		m_node = m_document.get();
	}

	Json::~Json() {}

	bool Json::Contains(const std::string& key) const
	{
		Assert(m_node);
		return m_node->contains(key);
	}

	Json Json::Child(const std::string& key)
	{
		Assert(m_node);
		AssertFmt(Contains(key), "Json error: child node {} not found", key);

		return Json(*this, (*m_node)[key]);
	}

	Json::operator bool() const
	{
		return m_node && !m_node->is_null();
	}
}
