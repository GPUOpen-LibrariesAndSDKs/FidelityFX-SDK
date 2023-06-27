// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
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

#include <codecvt>
#include <locale>
#include <string>

/// @defgroup CauldronHelpers Helpers
/// Helper functions used throughout <c><i>Cauldron</i></c>.
///
/// @ingroup CauldronMisc

/// \def NO_COPY(typeName)
/// Deletes copy constructors from \typeName class so the class can not be copied.
///
/// @ingroup CauldronHelpers
#define NO_COPY(typeName) \
    typeName(const typeName&) = delete; \
    typeName& operator=(const typeName&) = delete;

/// \def NO_MOVE(typeName)
/// Deletes move constructors from \typeName class so the class can not be moved.
///
/// @ingroup CauldronHelpers
#define NO_MOVE(typeName) \
    typeName(const typeName&&) = delete; \
    typeName& operator=(const typeName&&) = delete;

/// \def NO_MOVE(typeName)
/// Deletes move constructors from \typeName class so the class can not be moved.
/// 
/// @ingroup CauldronHelpers
#define ENUM_FLAG_OPERATORS(ENUMTYPE) \
    inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a)|((int)b)); } \
    inline ENUMTYPE operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
    inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a)&((int)b)); } \
    inline ENUMTYPE operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
    inline ENUMTYPE operator ~ (ENUMTYPE a) { return (ENUMTYPE)(~((int)a)); } \
    inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a)^((int)b)); } \
    inline ENUMTYPE operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \

/// Converts an std::wstring to std::string
///
/// @param [in] string  The std::wstring to convert to std::string.
///
/// @returns            The std::string representation of the std::wstring.
///
/// @ingroup CauldronHelpers
inline std::string WStringToString(const std::wstring& string)
{
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> stringConverter;
    return stringConverter.to_bytes(string);
}

/// Converts an std::string to std::wstring
///
/// @param [in] string  The std::string to convert to std::wstring.
///
/// @returns            The std::wstring representation of the std::string.
///
/// @ingroup CauldronHelpers
inline std::wstring StringToWString(const std::string& string)
{
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> stringConverter;
    return stringConverter.from_bytes(string);
}

/// Aligns a size up to the specified amount.
///
/// @param [in] val         The value to align.
/// @param [in] alignment   The alignment to align the value to.
///
/// @returns                The new aligned (if not already aligned) value.
///
/// @ingroup CauldronHelpers
template<typename T> inline T AlignUp(T val, T alignment)
{
    return (val + alignment - (T)1) & ~(alignment - (T)1);
}

/// Computes the rounded-up integer division of two unsigned integers.
///
/// @param [in] a The dividend.
/// @param [in] b The divisor.
///
/// @return The smallest integer greater than or equal to the exact division of a and b.
///
/// @ingroup CauldronHelpers
constexpr uint32_t DivideRoundingUp(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}

inline float CalculateMipBias(float upscalerRatio)
{
    return std::log2f(1.f / upscalerRatio) - 1.f + std::numeric_limits<float>::epsilon();
}
