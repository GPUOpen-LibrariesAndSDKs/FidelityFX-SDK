@REM regenerates the header files containing the shaderblobs for the ui composition blit
@REM only needs to be run when the shader is changed
..\..\..\..\tools\ffx_shader_compiler\libs\dxc\bin\x64\dxc.exe -Fh FrameInterpolationSwapchainUiCompositionVS.h -T vs_6_0 -E mainVS FrameInterpolationSwapchainUiComposition.hlsl 
..\..\..\..\tools\ffx_shader_compiler\libs\dxc\bin\x64\dxc.exe -Fh FrameInterpolationSwapchainUiCompositionPS.h -T ps_6_0 -E mainPS FrameInterpolationSwapchainUiComposition.hlsl 
