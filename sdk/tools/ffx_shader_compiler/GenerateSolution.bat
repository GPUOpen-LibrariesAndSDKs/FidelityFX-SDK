@echo off
setlocal enabledelayedexpansion

echo Checking pre-requisites... 

:: Check if CMake is installed
cmake --version > nul 2>&1
if %errorlevel% NEQ 0 (
    echo Cannot find path to cmake. Is CMake installed? Exiting...
    exit /b -1
) else (
    echo    CMake      - Ready.
) 

echo.
echo Building FidelityFX Shader Compiler Tool
echo.

:: Check directories exist and create if not
if not exist build\ (
	mkdir build 
)

cd build
cmake -A x64 ..
cd..

:: Pause so the user can acknowledge any errors or other outputs from the build process
pause