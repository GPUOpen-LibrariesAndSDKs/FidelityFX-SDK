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
#include <fstream>
#include <iostream>
#include <mutex>
#include <array>
#include <set>
#include <format>
//#include "core/math/Constants.h"

namespace fsr
{
    enum LogLevel : uint32_t
    {
        kLogDebug = 0,
        kLogNormal,
        kLogWarning,
        kLogError,
        kLogCritical,
        kLogSystem,
        kNumLogLevels
    };

    namespace Log
    {
        enum ANSIColourCode : uint32_t
        {
            kFgBlack = 30,
            kFgRed = 31,
            kFgGreen = 32,
            kFgYellow = 33,
            kFgBlue = 34,
            kFgPurple = 35,
            kFgTeal = 36,
            kFgWhite = 37,
            kFgDefault = 39,
            kFgBrightBlack = 90,
            kFgBrightRed = 91,
            kFgBrightGreen = 92,
            kFgBrightYellow = 93,
            kFgBrightBlue = 94,
            kFgBrightMagenta = 95,
            kFgBrightCyan = 96,
            kFgBrightWhite = 97,
            kBgRed = 41,
            kBgGreen = 42,
            kBgYellow = 43,
            kBgBlue = 44,
            kBgPurple = 45,
            kBgTeal = 46,
            kBgWhite = 47,
            kBgDefault = 49,
            kBgBrightBlack = 100,
            kBgBrightRed = 101,
            kBgBrightGreen = 102,
            kBgBrightYellow = 103,
            kBgBrightBlue = 104,
            kBgBrightMagenta = 105,
            kBgBrightCyan = 106,
            kBgBrightWhite = 107
        };

        // Little class to indent the log and un-indent automatically on destruction
        class Indent
        {
        public:
            Indent(const std::string& onIndent = "",
                const std::string& onRestore = "",
                const std::string& onException = "");
            ~Indent();
            void Restore();
        private:
            int32_t             m_logIndentation;
            const std::string   m_onRestore;
            const std::string   m_onException;
        };

        // Keeps track of the number of errors and warnings
        class Snapshot
        {
        private:
            std::array<uint32_t, kNumLogLevels>               m_numMessages;
        public:
            Snapshot();
            uint32_t operator[](const int i) const;
            uint32_t& operator[](const int i);
            Snapshot operator-(const Snapshot& rhs) const;
        };

        void        NL();
        Snapshot    GetMessageState();
        void        EnableLevel(const uint32_t flags, bool set);
        void        WriteImpl(const char* file, const int line, const std::string& formatted, const uint32_t colour, const LogLevel level);

#define LOG_TYPE(Name, Colour, Type) template<typename... Args> \
                                     inline void Name(const std::string& message, const Args&... args) { WriteImpl(nullptr, -1, std::vformat(message, std::make_format_args(args...)), Colour, Type); } \
                                     inline void Name(const std::string& message) { WriteImpl(nullptr, -1, message, Colour, Type); }

#define LOG_TYPE_ONCE(Name, Colour, Type) template<typename... Args> \
                                     inline void Name(const std::string& message, const Args&... args) { WriteImpl(__FILE__, __LINE__, std::vformat(message, std::make_format_args(args...)), Colour, Type); } \
                                     inline void Name(const std::string& message) { WriteImpl(__FILE__, __LINE__, message, Colour, Type); }

        LOG_TYPE(Write, kFgDefault, kLogNormal)
        LOG_TYPE(Success, kFgGreen, kLogNormal)
        LOG_TYPE(Debug, kFgBrightCyan, kLogDebug)
        LOG_TYPE(Warning, kFgYellow, kLogWarning)
        LOG_TYPE(Error, kBgRed, kLogError)
        LOG_TYPE(System, kFgTeal, kLogSystem)

        LOG_TYPE_ONCE(WriteOnce, kFgDefault, kLogNormal)
        LOG_TYPE_ONCE(SuccessOnce, kFgGreen, kLogNormal)
        LOG_TYPE_ONCE(DebugOnce, kFgBrightCyan, kLogDebug)
        LOG_TYPE_ONCE(WarningOnce, kFgYellow, kLogWarning)
        LOG_TYPE_ONCE(ErrorOnce, kBgRed, kLogError)
        LOG_TYPE_ONCE(SystemOnce, kFgTeal, kLogSystem)
    }
}
