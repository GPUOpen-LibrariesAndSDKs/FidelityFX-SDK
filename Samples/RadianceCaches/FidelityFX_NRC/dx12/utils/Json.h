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

#include <format>
#include <OpenSource/nlohmann/json_fwd.hpp>
#include "Assert.h"

namespace fsr
{
	class Json
	{
	public:
		Json(const std::string& filePath);
		~Json();

		operator bool() const;

		nlohmann::json& operator*() { AssertMsg(m_node, "Uninitialised Json object."); return *m_node; }
		const nlohmann::json& operator*() const { AssertMsg(m_node, "Uninitialised Json object."); return *m_node; }
		nlohmann::json* operator->() { AssertMsg(m_node, "Uninitialised Json object.");  return m_node; }
		const nlohmann::json* operator->() const { AssertMsg(m_node, "Uninitialised Json object."); return m_node; }

		bool Contains(const std::string& ptr) const;

		Json Child(const std::string& ptr);
		inline Json operator[](const char* ptr) { return Child(ptr); }

	private:
		Json(Json& parent, nlohmann::json& child);

		std::shared_ptr<nlohmann::json> m_document;
		nlohmann::json* m_node;
	};
}
