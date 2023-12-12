// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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



//--------------------------------------------------------------------------------------
// ParallelSort Shaders/Includes
//--------------------------------------------------------------------------------------
#define FFX_HLSL
#include "ffx_parallel_sort.h"

ConstantBuffer<FFX_ParallelSortCB> CBuffer    : register(b0);         // Constant buffer
cbuffer SetupIndirectCB                       : register(b1)          // Setup Indirect Constant buffer
{
   uint NumKeysIndex;
   uint MaxThreadGroups;
};

struct RootConstantData {
   uint CShiftBit;
};

#ifdef VK_Const
   [[vk::push_constant]] RootConstantData rootConstData;                                            // Store the shift bit directly in the root signature
#else
   ConstantBuffer<RootConstantData> rootConstData   : register(b2);                                 // Store the shift bit directly in the root signature
#endif // VK_Const

RWStructuredBuffer<uint>   SrcBuffer          : register(u0, space0);             // The unsorted keys or scan data
RWStructuredBuffer<uint>   SrcPayload         : register(u0, space1);             // The payload data
             
RWStructuredBuffer<uint>   SumTable           : register(u0, space2);             // The sum table we will write sums to
RWStructuredBuffer<uint>   ReduceTable        : register(u0, space3);             // The reduced sum table we will write sums to
             
RWStructuredBuffer<uint>   DstBuffer          : register(u0, space4);             // The sorted keys or prefixed data
RWStructuredBuffer<uint>   DstPayload         : register(u0, space5);             // the sorted payload data
             
RWStructuredBuffer<uint>   ScanSrc            : register(u0, space6);             // Source for Scan Data
RWStructuredBuffer<uint>   ScanDst            : register(u0, space7);             // Destination for Scan Data
RWStructuredBuffer<uint>   ScanScratch        : register(u0, space8);             // Scratch data for Scan

RWStructuredBuffer<uint>   NumKeysBuffer      : register(u0, space9);             // Number of keys to sort for indirect execution
RWStructuredBuffer<FFX_ParallelSortCB>   CBufferUAV   : register(u0, space10);    // UAV for constant buffer parameters for indirect execution
RWStructuredBuffer<uint>   CountScatterArgs   : register(u0, space11);            // Count and Scatter Args for indirect execution
RWStructuredBuffer<uint>   ReduceScanArgs     : register(u0, space12);            // Reduce and Scan Args for indirect execution


// FPS Count
[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void FPS_Count(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
   // Call the uint version of the count part of the algorithm
   FFX_ParallelSort_Count_uint(localID, groupID, CBuffer, rootConstData.CShiftBit, SrcBuffer, SumTable);
}

// FPS Reduce
[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void FPS_CountReduce(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
   // Call the reduce part of the algorithm
   FFX_ParallelSort_ReduceCount(localID, groupID, CBuffer,  SumTable, ReduceTable);
}

// FPS Scan
[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void FPS_Scan(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
   uint BaseIndex = FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE * groupID;
   FFX_ParallelSort_ScanPrefix(CBuffer.NumScanValues, localID, groupID, 0, BaseIndex, false, CBuffer, ScanSrc, ScanDst, ScanScratch);
}

// FPS ScanAdd
[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void FPS_ScanAdd(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
   // When doing adds, we need to access data differently because reduce 
   // has a more specialized access pattern to match optimized count
   // Access needs to be done similarly to reduce
   // Figure out what bin data we are reducing
   uint BinID       = groupID / CBuffer.NumReduceThreadgroupPerBin;
   uint BinOffset   = BinID * CBuffer.NumThreadGroups;

   // Get the base index for this thread group
   uint BaseIndex = (groupID % CBuffer.NumReduceThreadgroupPerBin) * FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;

   FFX_ParallelSort_ScanPrefix(CBuffer.NumThreadGroups, localID, groupID, BinOffset, BaseIndex, true, CBuffer, ScanSrc, ScanDst, ScanScratch);
}

// FPS Scatter
[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void FPS_Scatter(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
   FFX_ParallelSort_Scatter_uint(localID, groupID, CBuffer, rootConstData.CShiftBit, SrcBuffer, DstBuffer, SumTable
#ifdef kRS_ValueCopy
    ,SrcPayload, DstPayload
#endif // kRS_ValueCopy
   );
}

[numthreads(1, 1, 1)]
void FPS_SetupIndirectParameters(uint localID : SV_GroupThreadID)
{
   FFX_ParallelSort_SetupIndirectParams(NumKeysBuffer[NumKeysIndex], MaxThreadGroups, CBufferUAV, CountScatterArgs, ReduceScanArgs);
}
