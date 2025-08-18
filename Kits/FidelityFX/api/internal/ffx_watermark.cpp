// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
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

#include "ffx_watermark.h"
#include <ffx_watermark_permutations.h>

#include <cstdlib>

constexpr size_t MAX_LINE_LENGTH = 120;
constexpr size_t LINE_COUNT = 4;
constexpr uint32_t GLYPH_WIDTH = 7;
constexpr uint32_t GLYPH_HEIGHT = 9;

struct WatermarkConstantBuffer
{
    uint32_t offsetX, offsetY;
    uint32_t fontSize;
    uint32_t _padding;
    uint8_t message[LINE_COUNT][MAX_LINE_LENGTH];
};

FfxWatermark::FfxWatermark(FfxInterface* backendInterface, uint32_t effectContextId)
    : backendInterface(backendInterface), effectContextId(effectContextId)
{
    FfxShaderBlob blob = POPULATE_SHADER_BLOB_FFX(g_ffx_watermark_PermutationInfo, 0);

    FfxPipelineDescription desc{};
    desc.stage = FFX_BIND_COMPUTE_SHADER_STAGE;
    desc.rootConstantBufferCount = 1;
    wcscpy_s(desc.name, L"Watermark");

    FfxRootConstantDescription rootConstDescs[1] = {
        {sizeof(WatermarkConstantBuffer) / sizeof(uint32_t), FFX_BIND_COMPUTE_SHADER_STAGE}
    };
    desc.rootConstants = rootConstDescs;

    backendInterface->fpCreatePipeline(backendInterface, &blob, &desc, effectContextId, &watermarkPipeline);
}

FfxWatermark::~FfxWatermark()
{
    backendInterface->fpDestroyPipeline(backendInterface, &watermarkPipeline, effectContextId);
}

void FfxWatermark::Dispatch(FfxResourceInternal target, std::string_view message, uint32_t fontSize)
{
    WatermarkConstantBuffer cbData{};
    cbData.offsetX = 5;
    cbData.offsetY = 5;
    cbData.fontSize = fontSize;
    std::fill_n(reinterpret_cast<uint8_t*>(cbData.message), sizeof(cbData.message), UINT8_MAX);

    // convert string to char codes
    // font char map is: 0=space, 1-10 = digits, 11-36 = letters, 37-40 = special chars.
    size_t curRow = 0;
    size_t curCol = 0;
    size_t maxRowLength = 0;
    for (char c : message)
    {
        if (c == '\n' && curRow < LINE_COUNT - 1)
        {
            curRow++;
            curCol = 0;
            continue; // avoid incrementing curCol
        }
        else if (c >= 'A' && c <= 'Z')
        {
            cbData.message[curRow][curCol] = c - 'A' + 11;
        }
        else if (c >= 'a' && c <= 'z')
        {
            // lowercase letters get converted to uppercase font
            cbData.message[curRow][curCol] = c - 'a' + 11;
        }
        else if (c >= '0' && c <= '9')
        {
            cbData.message[curRow][curCol] = c - '0' + 1;
        }
        else switch (c)
        {
        case '.':
            cbData.message[curRow][curCol] = 37;
            break;
        case '-':
            cbData.message[curRow][curCol] = 38;
            break;
        case ':':
            cbData.message[curRow][curCol] = 40;
            break;
        default:
            // any other character (including newline on last line) gets mapped as space.
            cbData.message[curRow][curCol] = 0;
            break;
        }

        curCol++;
        if (curCol > maxRowLength)
            maxRowLength = curCol;
        if (curCol >= MAX_LINE_LENGTH)
        {
            curCol = 0;
            curRow++;
            if (curRow >= LINE_COUNT)
            {
                // end of the last line.
                break;
            }
        }
    }

    FfxConstantBuffer cbHandle;
    backendInterface->fpStageConstantBufferDataFunc(backendInterface, &cbData, static_cast<uint32_t>(sizeof(cbData)), &cbHandle);

    size_t lastRow = curRow;
    if (curCol > 0)
    {
        lastRow++;
    }

    FfxGpuJobDescription jobDesc{};
    jobDesc.jobType = FFX_GPU_JOB_COMPUTE;
    wcscpy_s(jobDesc.jobLabel, L"Watermark");
    jobDesc.computeJobDescriptor.cbs[0] = cbHandle;
    jobDesc.computeJobDescriptor.dimensions[0] = static_cast<uint32_t>((maxRowLength * fontSize * GLYPH_WIDTH + 7) / 8);
    jobDesc.computeJobDescriptor.dimensions[1] = static_cast<uint32_t>((lastRow * fontSize * GLYPH_HEIGHT + 7) / 8);
    jobDesc.computeJobDescriptor.dimensions[2] = 1;
    jobDesc.computeJobDescriptor.pipeline = watermarkPipeline;
    jobDesc.computeJobDescriptor.uavTextures[0].resource = target;
    jobDesc.computeJobDescriptor.flags = FFX_GPU_JOB_FLAGS_NONE;

    backendInterface->fpScheduleGpuJob(backendInterface, &jobDesc);
}
