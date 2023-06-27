@echo off

:: Known bundle SHA256 hash to attempt to download
SET default_bundle_sha256=48f8b87302950208cfea7619a03235a448b913d4e4836422714696192ae1511f

SET ffx_sdk_root=%~dp0

SET ARG=%1
IF DEFINED ARG (
    echo SDK root: %ffx_sdk_root%
    echo.

    pushd %ffx_sdk_root%
    .\sdk\tools\media_delivery\MediaDelivery.exe %*
    popd

    GOTO :eof
)

echo Default bundle SHA-256: %default_bundle_sha256%
echo SDK root: %ffx_sdk_root%
echo.

pushd %ffx_sdk_root%
.\sdk\tools\media_delivery\MediaDelivery.exe --target-sha256=%default_bundle_sha256%
popd
