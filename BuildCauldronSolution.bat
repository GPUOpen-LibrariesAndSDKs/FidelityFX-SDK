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

echo.
echo Building Cauldron (dev) solution
echo.

:: Check directories exist and create if not
if not exist build\ (
	mkdir build 
)

cd build
cmake -A x64 .. -D BUILD_TYPE=CAULDRON %*

:: Come back to root level
cd ..
pause
