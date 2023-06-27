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
echo Cleaning all stale generated data
echo.

rmdir /S /Q bin
rmdir /S /Q build
rmdir /S /Q sdk\bin
rmdir /S /Q sdk\build
rmdir /S /Q sdk\tools\ffx_shader_compiler\bin
rmdir /S /Q sdk\tools\ffx_shader_compiler\build

echo.
echo Cleaning complete
echo.
pause