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

#include <string>
#include <format>

namespace fsr
{
    // Converts an 8-bit ASCII string into a 16-bit wide unicode string
    std::wstring Widen(const std::string& str);

    // Converts a 16-bit wide unicode string into an 8-bit wide ASCII string
    std::string Narrow(const std::wstring& wstr);

    // Capitalises the first letter of the input string
    std::string CapitaliseFirst(const std::string& input);

    // Removes training whitespace from the end of a string
    void ClipTrailingWhitespace(std::string& input);

    // Makes a string lowercase
    void MakeLowercase(std::string& input);

    // Returns a lowercase copy of a string
    std::string Lowercase(const std::string& input);

    // Checks to see whether a string is all lowercase
    bool IsLowercase(const std::string& input);
    
    // Makes a string uppercase
    void MakeUppercase(std::string& input);

    // Returns an uppercase copy of a string
    std::string Uppercase(const std::string& input);
    
    // Checks to see whether a string is all uppercase
    bool IsUppercase(const std::string& input);

    // Formats a floating point value with digit grouping to dp decimal places
    std::string FormatPrettyFloat(const float value, int dp);

    // Formats a time value in seconds in the style hh:mm::ss.xxx
    std::string FormatElapsedTime(const float time);

    // Formats a data size in using the nearest denomination of B, kB, MB, GB
    std::string FormatDataSize(const float inputMB, const int dp);   

    // Pads the string with whitespace
    template<typename... Pack>
    std::string Pad(const int minLength, const char ws, const char* fmt, Pack... args)
    {
        std::string str = std::vformat(fmt, std::make_format_args(args...));
        if(str.length() < minLength)
        {
            str.resize(minLength, ws);
        }
        return str;
    }

    template<typename... Pack>
    inline std::string Pad(const int minLength, const char* fmt, Pack... args)
    {
        return Pad(minLength, ' ', fmt, args...);
    }
}
