:: This file is part of the FidelityFX SDK.
::
:: Copyright (C) 2025 Advanced Micro Devices, Inc.
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

:: Known bundle SHA256 hash to attempt to download
SET default_bundle_sha256=76e5016a04a5842b521c33d780e225b28c40526fee9fd56f6e7c06b76778b4ea

SET ffx_sdk_root="%~dp0"

SET media_path="./media/cauldronmedia"

SET ARG=%1
IF DEFINED ARG (
    echo SDK root: %ffx_sdk_root%
    echo.

    pushd %ffx_sdk_root%
    .\tools\media_delivery\MediaDelivery.exe --media-sub-dir %media_path% %*
    popd

    GOTO :eof
)

echo Default bundle SHA-256: %default_bundle_sha256%
echo SDK root: %ffx_sdk_root%
echo.

pushd %ffx_sdk_root%
.\tools\media_delivery\MediaDelivery.exe --media-sub-dir %media_path% --target-sha256=%default_bundle_sha256%
popd
