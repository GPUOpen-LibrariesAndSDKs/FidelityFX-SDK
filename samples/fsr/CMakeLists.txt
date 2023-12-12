# This file is part of the FidelityFX SDK.
# 
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Declare project
if (FFX_API STREQUAL CAULDRON)
	project(FFX_FSR)
else()
	project(FFX_FSR_NATIVE)
endif()

# ---------------------------------------------
# Fidelity FX FSR Sample
# ---------------------------------------------

# Sample files
set(fsr_sample_files
	${CMAKE_CURRENT_SOURCE_DIR}/fsrsample.h
	${CMAKE_CURRENT_SOURCE_DIR}/fsrsample.cpp)

# FSR render module files
file(GLOB hlslCompile_src
	${CMAKE_CURRENT_SOURCE_DIR}/HLSLCompile/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/HLSLCompile/*.cpp)

# FSR render module files
file(GLOB fsr_src
	${CMAKE_CURRENT_SOURCE_DIR}/*rendermodule.h
	${CMAKE_CURRENT_SOURCE_DIR}/*rendermodule.cpp)

file(GLOB fsr_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)
set_source_files_properties(${fsr_shaders} PROPERTIES VS_TOOL_OVERRIDE "Text")
copyCommand("${fsr_shaders}" ${SHADER_OUTPUT})

# Add config file
set(config_file ${CMAKE_CURRENT_SOURCE_DIR}/config/fsrconfig.json)
copyCommand("${config_file}" ${CONFIG_OUTPUT})

# Copy media file
set(animated_media_src ${CMAKE_CURRENT_SOURCE_DIR}/media/checkerboard.dds ${CMAKE_CURRENT_SOURCE_DIR}/media/composition_text.dds ${CMAKE_CURRENT_SOURCE_DIR}/media/lion.jpg)
copyCommand("${animated_media_src}" ${BIN_OUTPUT}/media/textures)

# Add the sample to the solution
add_executable(${PROJECT_NAME} WIN32 ${default_icon_src} ${default_entry_point} ${fsr_sample_files} ${config_file} ${fsr_src} ${hlslCompile_src} ${fsr_shaders} ${ffx_remap} ${animated_media_src})

# Setup the correct exe based on backend name
set(EXE_OUT_NAME ${PROJECT_NAME}_)

# Link everything
if (FFX_API STREQUAL CAULDRON)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Framework RenderModules d3dcompiler ffx_backend_cauldron_${FFX_PLATFORM_NAME} ffx_fsr3_${FFX_PLATFORM_NAME} ffx_fsr3upscaler_${FFX_PLATFORM_NAME} ffx_frameinterpolation_${FFX_PLATFORM_NAME} ffx_opticalflow_${FFX_PLATFORM_NAME} ffx_fsr2_${FFX_PLATFORM_NAME} ffx_fsr1_${FFX_PLATFORM_NAME})
else()
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Framework RenderModules d3dcompiler ffx_backend_native ffx_fsr3_${FFX_PLATFORM_NAME} ffx_fsr3upscaler_${FFX_PLATFORM_NAME} ffx_frameinterpolation_${FFX_PLATFORM_NAME} ffx_opticalflow_${FFX_PLATFORM_NAME} ffx_fsr2_${FFX_PLATFORM_NAME} ffx_fsr1_${FFX_PLATFORM_NAME})
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
						OUTPUT_NAME_DEBUGDX12 "${EXE_OUT_NAME}DX12D"
						OUTPUT_NAME_DEBUGVK "${EXE_OUT_NAME}VKD"
						OUTPUT_NAME_RELEASEDX12 "${EXE_OUT_NAME}DX12"
						OUTPUT_NAME_RELEASEVK "${EXE_OUT_NAME}VK"
						VS_DEBUGGER_WORKING_DIRECTORY "${BIN_OUTPUT}")

# Add dependency information
add_dependencies(${PROJECT_NAME} Framework)
add_dependencies(${PROJECT_NAME} RenderModules)
add_dependencies(${PROJECT_NAME} ffx_fsr3_${FFX_PLATFORM_NAME})
add_dependencies(${PROJECT_NAME} ffx_fsr3upscaler_${FFX_PLATFORM_NAME})
add_dependencies(${PROJECT_NAME} ffx_frameinterpolation_${FFX_PLATFORM_NAME})
add_dependencies(${PROJECT_NAME} ffx_opticalflow_${FFX_PLATFORM_NAME})
add_dependencies(${PROJECT_NAME} ffx_fsr2_${FFX_PLATFORM_NAME})
add_dependencies(${PROJECT_NAME} ffx_fsr1_${FFX_PLATFORM_NAME})

# Add manifest info
addManifest(${PROJECT_NAME})

# Add compile definitions to identify the sample
target_compile_definitions(${PROJECT_NAME} PRIVATE SampleName=L"FidelityFX FSR"
										   SampleInclude=fsr)

# Link the correct backend in
if (FFX_API STREQUAL CAULDRON)
	target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ffx_backend_cauldron_${FFX_PLATFORM_NAME})
	add_dependencies(${PROJECT_NAME} ffx_backend_cauldron_${FFX_PLATFORM_NAME})
	target_include_directories(${PROJECT_NAME} PUBLIC ${FFX_API_CAULDRON_ROOT})
else()
	target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ffx_backend_native)
	add_dependencies(${PROJECT_NAME} ffx_backend_native)
	target_include_directories(${PROJECT_NAME} PUBLIC ${FFX_HOST_BACKENDS_PATH})
endif()

# And solution layout definitions
source_group(""					FILES ${default_entry_point})
source_group(""					FILES ${ffx_remap})
source_group("Sample" 				FILES ${fsr_sample_files})
source_group("Icon"    				FILES ${default_icon_src})
source_group("Config" 				FILES ${config_file})
source_group("RenderModule"			FILES ${fsr_src})
source_group("RenderModule\\HLSLCompile"	FILES ${hlslCompile_src})
source_group("RenderModule\\Shaders"		FILES ${fsr_shaders})
source_group("Media"                    	FILES ${particle_media_src} ${animated_media_src})
