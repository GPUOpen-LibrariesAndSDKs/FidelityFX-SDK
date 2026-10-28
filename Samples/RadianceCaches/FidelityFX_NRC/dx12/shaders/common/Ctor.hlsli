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

#define CTOR1(type, var, param1) { const type __ctor_temp = { param1 }; var = __ctor_temp; }
#define CTOR2(type, var, param1, param2) { const type __ctor_temp = { param1, param2 }; var = __ctor_temp; }
#define CTOR3(type, var, param1, param2, param3) { const type __ctor_temp = { param1, param2, param3 }; var = __ctor_temp; }
#define CTOR4(type, var, param1, param2, param3, param4) { const type __ctor_temp = { param1, param2, param3, param4 }; var = __ctor_temp; }
#define CTOR5(type, var, param1, param2, param3, param4, param5) { const type __ctor_temp = { param1, param2, param3, param4, param5 }; var = __ctor_temp; }
