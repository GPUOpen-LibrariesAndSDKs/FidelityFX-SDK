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


#include "ffx_core.h"

#if FFX_PARALLELSORT_OPTION_HAS_PAYLOAD
    #define FFX_PARALLELSORT_COPY_VALUE 1
#endif // FFX_PARALLELSORT_OPTION_HAS_PAYLOAD

void FfxParallelSortSetupIndirectArgs(FfxUInt32 LocalThreadId)
{
    // Just copy the appropriate data from the constant buffer to control number of jobs to dispatch
    // for count, reduce, scan, and scatter passes
    StoreCountScatterArgs(NumThreadGroups(), 1, 1);
    StoreReduceScanArgs(NumScanValues(), 1, 1);
}
