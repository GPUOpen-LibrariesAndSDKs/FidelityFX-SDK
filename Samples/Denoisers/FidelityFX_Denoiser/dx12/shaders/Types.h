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

//------------------------------------------------------------------------------
// Sample Includes
//------------------------------------------------------------------------------

#if __cplusplus
// std::array
#include <array>
// std::size_t
#include <cstddef>
// std::int32_t
// std::int64_t
// std::uint32_t
// std::uint64_t
#include <cstdint>
#endif // __cplusplus

//------------------------------------------------------------------------------
// Sample Declarations
//------------------------------------------------------------------------------

namespace rr
{
    #if __cplusplus

    //--------------------------------------------------------------------------
    // C++
    //--------------------------------------------------------------------------

    template< typename T, std::size_t N >
    using vector = std::array< T, N >;
    template< typename T, std::size_t M, std::size_t N >
    using matrix = std::array< std::array< T, N >, M >; // row-major packing order

    using F32 = float;
    using F64 = double;

    using S32 = std::int32_t;
    using S64 = std::int64_t;

    using U32 = std::uint32_t;
    using U64 = std::uint64_t;

    #else  // __cplusplus

    //--------------------------------------------------------------------------
    // HLSL
    //--------------------------------------------------------------------------

    typedef float32_t F32;

    typedef int32_t   S32;

    typedef uint32_t  U32;

    #endif // __cplusplus

    //--------------------------------------------------------------------------
    // C++/HLSL
    //--------------------------------------------------------------------------

    typedef vector< F32, 2u >     F32x2;
    typedef vector< F32, 3u >     F32x3;
    typedef vector< F32, 4u >     F32x4;
    typedef matrix< F32, 3u, 3u > F32x3x3;
    typedef matrix< F32, 3u, 4u > F32x3x4;
    typedef matrix< F32, 4u, 4u > F32x4x4;

    typedef vector< S32, 2u >     S32x2;
    typedef vector< S32, 3u >     S32x3;
    typedef vector< S32, 4u >     S32x4;

    typedef vector< U32, 2u >     U32x2;
    typedef vector< U32, 3u >     U32x3;
    typedef vector< U32, 4u >     U32x4;
}
