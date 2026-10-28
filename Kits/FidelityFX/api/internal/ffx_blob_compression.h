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

#include <stdint.h>

// Shared on-disk format for compressed embedded shader blobs.
//
// When FidelityFX_SC is run with -compress, each generated g_<name>_data[] array
// holds an FfxBlobCompressHeader immediately followed by a raw-deflate payload,
// instead of the raw SPIR-V/DXIL bytecode. g_<name>_size is the total byte count
// (sizeof(FfxBlobCompressHeader) + compressed payload).
//
// The runtime detects this by checking the leading magic word; blobs that do not
// begin with the magic are treated as uncompressed and passed through unchanged.
// This keeps the format backward compatible and lets compressed and uncompressed
// effects share the same backend decompress hook.

// 'FFXZ' little-endian. Chosen so it never collides with real bytecode headers:
// SPIR-V starts with 0x07230203, DXIL/DXBC ('DXBC') with 0x43425844.
#define FFX_BLOB_COMPRESS_MAGIC 0x465A5846u

typedef struct FfxBlobCompressHeader
{
    uint32_t magic;             // FFX_BLOB_COMPRESS_MAGIC
    uint32_t uncompressedSize;  // size in bytes of the original bytecode
} FfxBlobCompressHeader;
