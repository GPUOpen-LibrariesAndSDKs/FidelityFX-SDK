@echo off

SET ffx_sdk_root=%~dp0

echo Clearing media delivery cache for SDK root: %ffx_sdk_root%

pushd %ffx_sdk_root%
.\sdk\tools\media_delivery\MediaDelivery.exe --clear-cache
popd
