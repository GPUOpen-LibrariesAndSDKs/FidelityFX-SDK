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

#if __cplusplus
    #pragma once
    #include "misc/assert.h"
#else
    #include "parallelsort/ffx_parallelsort.h"
#endif  // __cplusplus

#if __cplusplus
struct SetupIndirectCB
{
    uint32_t maxThreadGroups;
    uint32_t shift;
};
#else
cbuffer SetupIndirectCB : register(b0)  // Setup Indirect Constant buffer
{
    uint maxThreadGroups;
    uint shift;
};
#endif  // __cplusplus

#ifndef __cplusplus

struct ParallelSortConstants
{
    uint numKeys;                              ///< The number of keys to sort
    int  numBlocksPerThreadGroup;              ///< How many blocks of keys each thread group needs to process
    uint numThreadGroups;                      ///< How many thread groups are being run concurrently for sort
    uint numThreadGroupsWithAdditionalBlocks;  ///< How many thread groups need to process additional block data
    uint numReduceThreadgroupPerBin;           ///< How many thread groups are summed together for each reduced bin entry
    uint numScanValues;                        ///< How many values to perform scan prefix (+ add) on
    uint shift;                                ///< What bits are being sorted (4 bit increments)
    uint padding;                              ///< Padding - unused
};

StructuredBuffer<uint>                       NumKeys          : register(t0);
RWStructuredBuffer<ParallelSortConstants>    CBufferUAV       : register(u0);  // UAV for constant buffer parameters for indirect execution
RWStructuredBuffer<uint>                     CountScatterArgs : register(u1);  // Count and Scatter Args for indirect execution
RWStructuredBuffer<uint>                     ReduceScanArgs   : register(u2);  // Reduce and Scan Args for indirect execution

void ParallelSortSetConstantAndDispatchData(uint numKeys, uint maxThreadGroups, uint shift, RWStructuredBuffer<ParallelSortConstants> constantBuffer, RWStructuredBuffer<uint> countScatterArgs, RWStructuredBuffer<uint> reduceScanArgs)
{
    constantBuffer[0].numKeys = numKeys;
    constantBuffer[0].shift   = shift;

    uint BlockSize = FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;
    uint NumBlocks = (numKeys + BlockSize - 1) / BlockSize;

    // Figure out data distribution
    uint NumThreadGroupsToRun                             = maxThreadGroups;
    uint BlocksPerThreadGroup                             = (NumBlocks / NumThreadGroupsToRun);
    constantBuffer[0].numThreadGroupsWithAdditionalBlocks = NumBlocks % NumThreadGroupsToRun;

    if (NumBlocks < NumThreadGroupsToRun)
    {
        BlocksPerThreadGroup                                  = 1;
        NumThreadGroupsToRun                                  = NumBlocks;
        constantBuffer[0].numThreadGroupsWithAdditionalBlocks = 0;
    }

    constantBuffer[0].numThreadGroups         = NumThreadGroupsToRun;
    constantBuffer[0].numBlocksPerThreadGroup = BlocksPerThreadGroup;

    // Calculate the number of thread groups to run for reduction (each thread group can process BlockSize number of entries)
    uint NumReducedThreadGroupsToRun =
        FFX_PARALLELSORT_SORT_BIN_COUNT * ((BlockSize > NumThreadGroupsToRun) ? 1 : (NumThreadGroupsToRun + BlockSize - 1) / BlockSize);
    constantBuffer[0].numReduceThreadgroupPerBin = NumReducedThreadGroupsToRun / FFX_PARALLELSORT_SORT_BIN_COUNT;
    constantBuffer[0].numScanValues =
        NumReducedThreadGroupsToRun;  // The number of reduce thread groups becomes our scan count (as each thread group writes out 1 value that needs scan prefix)

    // Setup dispatch arguments
    countScatterArgs[0] = NumThreadGroupsToRun;
    countScatterArgs[1] = 1;
    countScatterArgs[2] = 1;

    reduceScanArgs[0] = NumReducedThreadGroupsToRun;
    reduceScanArgs[1] = 1;
    reduceScanArgs[2] = 1;
}

void ParallelSortSetupIndirectArgs(uint LocalThreadId)
{
    ParallelSortSetConstantAndDispatchData(NumKeys[0], maxThreadGroups, shift, CBufferUAV, CountScatterArgs, ReduceScanArgs);
}
#endif  // !__cplusplus
