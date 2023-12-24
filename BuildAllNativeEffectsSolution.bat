@echo off

:: Start by building the backend SDK
echo Building native backend
cd sdk
call BuildFidelityFXSDK.bat %*
cd ..

echo.
echo Building native SDK sample solutions
echo.

:: Check directories exist and create if not
if not exist build\ (
	mkdir build 
)

cd build
cmake -A x64 .. -DBUILD_TYPE=ALL_SAMPLES -DFFX_API=NATIVE %*

:: Come back to root level
cd ..
pause
