@echo off

echo ===============================================================
echo.
echo  FidelityFX Build System
echo.
echo ===============================================================
echo Checking pre-requisites...

:: Check if cmake is installed
cmake --version > nul 2>&1
if %errorlevel% NEQ 0 (
    echo Cannot find path to CMake. Is CMake installed? Exiting...
    exit /b -1
) else (
    echo CMake Ready.
)

:: Start by building the backend SDK
:: CUSTOM_CHANGES_TO_FSR3 build FSR3 DLL with RelWithDebInfo configuration
echo Building native backend
cd sdk
call BuildFidelityFXSDK.bat -DFFX_BUILD_AS_DLL=1
cd ..

echo.
echo Building FSR sample solution
echo.

:: Check directories exist and create if not
if not exist build\ (
	mkdir build
)

cd build
cmake -A x64 .. -DBUILD_TYPE=FFX_FSR -DFFX_API=NATIVE -DFFX_BUILD_AS_DLL=1

:: Come back to root level
cd ..
pause