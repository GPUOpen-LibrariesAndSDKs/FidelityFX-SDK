:: This file is part of the FidelityFX SDK.
::
:: Copyright (C) 2024 Advanced Micro Devices, Inc.
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files(the "Software"), to deal
:: in the Software without restriction, including without limitation the rights
:: to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
:: copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions :
::
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
::
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
:: OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
:: THE SOFTWARE.

@echo off

if not exist build\ (
	mkdir build
)

cd build
:: Clear out CMakeCache
if exist CMakeFiles\ (
	rmdir /S /Q CMakeFiles
)
if exist CMakeCache.txt (
	del /S /Q CMakeCache.txt
)
cmake -DFFX_API_BACKEND=VK_X64 -A x64 .. %*%
cmake --build ./ --config Debug --parallel 4 -- /p:CL_MPcount=16
cmake --build ./ --config Release --parallel 4 -- /p:CL_MPcount=16
cmake --build ./ --config RelWithDebInfo --parallel 4 -- /p:CL_MPcount=16

:: Come back to root level
cd ..