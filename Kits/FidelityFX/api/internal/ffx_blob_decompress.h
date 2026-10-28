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

#include "ffx_blob_compression.h"

#include <miniz.h>

#include <cstring>
#include <vector>

// Backend-side helper for the two shader-module choke points (CreatePipelineVK /
// CreatePipelineDX12). Given an embedded shader blob, returns the pointer/size to
// hand to the graphics API:
//   * If the blob begins with FFX_BLOB_COMPRESS_MAGIC, it is inflated into `scratch`
//     and *outData/*outSize point at the decompressed bytecode.
//   * Otherwise the input is passed through unchanged (raw, uncompressed bytecode).
//
// `scratch` must outlive the use of *outData (i.e. until vkCreateShaderModule /
// CreateComputePipelineState has consumed the bytecode).
static inline void ffxDecompressShaderBlob(const void*           data,
                                           uint32_t              size,
                                           std::vector<uint8_t>& scratch,
                                           const void**          outData,
                                           uint32_t*             outSize)
{
    *outData = data;
    *outSize = size;

    if (data == nullptr || size < sizeof(FfxBlobCompressHeader))
        return;

    FfxBlobCompressHeader header;
    memcpy(&header, data, sizeof(header));

    if (header.magic != FFX_BLOB_COMPRESS_MAGIC)
        return;  // not compressed - pass through

    const mz_uint8* payload    = static_cast<const mz_uint8*>(data) + sizeof(FfxBlobCompressHeader);
    mz_ulong        payloadLen = size - sizeof(FfxBlobCompressHeader);

    scratch.resize(header.uncompressedSize);
    mz_ulong destLen = header.uncompressedSize;

    int status = mz_uncompress(scratch.data(), &destLen, payload, payloadLen);
    if (status == MZ_OK && destLen == header.uncompressedSize)
    {
        *outData = scratch.data();
        *outSize = header.uncompressedSize;
    }
    // On failure, leave the passthrough values in place; the subsequent shader
    // module / PSO creation will fail loudly rather than silently misbehave.
}
