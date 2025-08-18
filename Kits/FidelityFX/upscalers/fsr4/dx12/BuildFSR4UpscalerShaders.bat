@ECHO OFF

set resolution=%1
set projectdir=%2
set basedir=%3
set outputdir=%4
set filename=%5

:: Remove all quotes because arguments can contain them
set resolution=%resolution:"=%
set projectdir=%projectdir:"=%
set basedir=%basedir:"=%
set outputdir=%outputdir:"=%
set filename=%filename:"=%

:: Set the absolute path to the FidelityFX directory to avoid issues with long relative path names
pushd %projectdir%%basedir%
set sdkdir=%cd%
popd

:: Set additional variables with quotes to support paths with spaces
set outdir="%projectdir%%outputdir%ffx_sc_output"
set FFX_SC="%sdkdir%\tools\ffx_sc\bin\FidelityFX_SC.exe"
set FFX_SC_ARGS=-deps=gcc -enable-16bit-types -HV 2021 -O3 -T cs_6_4 -no-warnings -embed-arguments -reflection

set FFX_MODEL_NAME_WMMA=fsr4_model_v07_fp8_no_scale

set FSR4_SOURCE_PATH=%sdkdir%/upscalers/fsr4/dx12
set FSR4_INCLUDE_ARGS=-I %FSR4_SOURCE_PATH% -I%sdkdir%/api/internal/dx12

::WMMA version shaders
for /L %%A in (0,1,12) do (call :compileWMMA %%A)

:done
exit /b 0

:compileWMMA
:: NOTE TO STEPHAN, I'm printing out the shader compiler commands here for validation
:: To compare with ffx-api, add -debugcmdline in <root>\ffx-api\src\fsr4\CMakeLists.txt
:: before -name=${SHADER_NAME} for 1-12 WMMA and 0-12 padding reset builds
if %1 == 0 goto :PaddingReset
set FSR4WWMA_SC_ARGS=%FFX_SC_ARGS% -DWMMA_ENABLED=1 -DMLSR_PASS_%1=1 -E %FFX_MODEL_NAME_WMMA%_pass%1
set FFX_SHADER_NAME=%FFX_MODEL_NAME_WMMA%_%resolution%_%1
::Compile WMMA pass %1 (1-12)
REM echo %FFX_SC% %FSR4WWMA_SC_ARGS% -name=%FFX_SHADER_NAME% %FSR4_INCLUDE_ARGS% -output=%outdir% "%sdkdir%\upscalers\fsr4\internal\shaders\%filename%.hlsl"
%FFX_SC% %FSR4WWMA_SC_ARGS% -name=%FFX_SHADER_NAME% %FSR4_INCLUDE_ARGS% -output=%outdir% "%sdkdir%\upscalers\fsr4\internal\shaders\%filename%.hlsl"

:PaddingReset

::Compile Padding reset
set FFX_SHADER_NAME=%FFX_MODEL_NAME_WMMA%_%resolution%_%1_post
set FSR4PADDING_RESET_SC_ARGS=%FFX_SC_ARGS% -DMLSR_PASS_%1_POST=1 -E %FFX_MODEL_NAME_WMMA%_pass%1_post
REM echo %FFX_SC% %FSR4PADDING_RESET_SC_ARGS% -name=%FFX_SHADER_NAME% %FSR4_INCLUDE_ARGS% -output=%outdir% "%sdkdir%\upscalers\fsr4\internal\shaders\%filename%.hlsl"
%FFX_SC% %FSR4PADDING_RESET_SC_ARGS% -name=%FFX_SHADER_NAME% %FSR4_INCLUDE_ARGS% -output=%outdir% "%sdkdir%\upscalers\fsr4\internal\shaders\%filename%.hlsl"
