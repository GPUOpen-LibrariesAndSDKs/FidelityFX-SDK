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
// Sample Defines
//------------------------------------------------------------------------------

#define CBV_REGISTER(slot)     b##slot
#define SRV_REGISTER(slot)     t##slot
#define UAV_REGISTER(slot)     u##slot
#define SAMPLER_REGISTER(slot) s##slot

#define CBV_BIND(slot)     register(CBV_REGISTER(slot))
#define SRV_BIND(slot)     register(SRV_REGISTER(slot))
#define UAV_BIND(slot)     register(UAV_REGISTER(slot))
#define SAMPLER_BIND(slot) register(SAMPLER_REGISTER(slot))

#define DECLARE_CBV_BINDING(slot, type, name)                                  \
    ConstantBuffer< type > name : CBV_BIND(slot)
#define DECLARE_SAMPLER_BINDING(slot, type, name)                              \
    type name : SAMPLER_BIND(slot)
#define DECLARE_SAMPLER_ARRAY_BINDING(slot, type, count, name)                 \
    type name[count] : SAMPLER_BIND(slot)
#define DECLARE_SRV_BINDING(slot, type, name)                                  \
    type name : SRV_BIND(slot)
#define DECLARE_SRV_ARRAY_BINDING(slot, type, count, name)                     \
    type name[count] : SRV_BIND(slot)
#define DECLARE_UAV_BINDING(slot, type, name)                                  \
    type name : UAV_BIND(slot)
#define DECLARE_UAV_ARRAY_BINDING(slot, type, count, name)                     \
    type name[count] : UAV_BIND(slot)

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x)      STRINGIFY_IMPL(x)

#define DECLARE_CBV_ARRAY_DESCRIPTOR_RANGE(slot, count)                        \
    "CBV(b" STRINGIFY(slot) ", numDescriptors = " STRINGIFY(count) ")"
#define DECLARE_SAMPLER_ARRAY_DESCRIPTOR_RANGE(slot, count)                    \
    "Sampler(s" STRINGIFY(slot) ", numDescriptors = " STRINGIFY(count) ")"
#define DECLARE_SRV_ARRAY_DESCRIPTOR_RANGE(slot, count)                        \
    "SRV(t" STRINGIFY(slot) ", numDescriptors = " STRINGIFY(count) ")"
#define DECLARE_UAV_ARRAY_DESCRIPTOR_RANGE(slot, count)                        \
    "UAV(u" STRINGIFY(slot) ", numDescriptors = " STRINGIFY(count) ")"
