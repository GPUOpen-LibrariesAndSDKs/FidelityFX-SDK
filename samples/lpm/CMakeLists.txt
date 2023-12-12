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
	project(FFX_LPM)
else()
	project(FFX_LPM_NATIVE)
endif()

# ---------------------------------------------
# Fidelity FX LPM Sample
# ---------------------------------------------

# LPM render module files
file(GLOB lpm_src
    ${CMAKE_CURRENT_SOURCE_DIR}/*rendermodule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*rendermodule.cpp)

# Add config file
set(config_file ${CMAKE_CURRENT_SOURCE_DIR}/config/lpmconfig.json)
copyCommand("${config_file}" ${CONFIG_OUTPUT})

# Add the sample to the solution
add_executable(${PROJECT_NAME} WIN32 ${default_icon_src} ${default_entry_point} ${default_sample_files} ${config_file} ${lpm_src} ${ffx_remap})

# Setup the correct exe based on backend name
set(EXE_OUT_NAME ${PROJECT_NAME}_)

# Link everything (including the compiler for now)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Framework RenderModules d3dcompiler ffx_lpm_${FFX_PLATFORM_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
                                        OUTPUT_NAME_DEBUGDX12 "${EXE_OUT_NAME}DX12D"
                                        OUTPUT_NAME_DEBUGVK "${EXE_OUT_NAME}VKD"
                                        OUTPUT_NAME_RELEASEDX12 "${EXE_OUT_NAME}DX12"
                                        OUTPUT_NAME_RELEASEVK "${EXE_OUT_NAME}VK"
                                        VS_DEBUGGER_WORKING_DIRECTORY "${BIN_OUTPUT}")

# Add compile definitions to identify the sample
target_compile_definitions(${PROJECT_NAME} PRIVATE SampleName=L"FidelityFX LPM"
											ConfigFileName=L"configs/lpmconfig.json"
											ConfigSampleName="FidelityFX LPM"
#Add RenderModuleClass definition only if this sample will use a custom rendermodule
#This will translate to <RenderModuleName>RenderModule as a class name / include file name
											RenderModuleName=LPM
#Add RenderModuleConfig (path to config file) only if this sample uses a custom rendermodule requiring a config file
											#RenderModuleConfig=L"${CMAKE_CURRENT_SOURCE_DIR}/config/config.json"
											)

# Add dependency information
add_dependencies(${PROJECT_NAME} Framework)
add_dependencies(${PROJECT_NAME} RenderModules)
add_dependencies(${PROJECT_NAME} ffx_lpm_${FFX_PLATFORM_NAME})

# Add manifest info
addManifest(${PROJECT_NAME})

# Link the correct backend in
if (FFX_API STREQUAL CAULDRON)
    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ffx_backend_cauldron_${FFX_PLATFORM_NAME})
    add_dependencies(${PROJECT_NAME} ffx_backend_cauldron_${FFX_PLATFORM_NAME})
    target_include_directories(${PROJECT_NAME} PUBLIC ${FFX_API_CAULDRON_ROOT})
else()
    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ffx_backend_native)
    target_include_directories(${PROJECT_NAME} PUBLIC ${FFX_HOST_BACKENDS_PATH})
endif()

# And solution layout definitions
source_group(""                         FILES ${default_entry_point})
source_group(""                         FILES ${ffx_remap})
source_group("Sample"                   FILES ${lpm_sample_files})
source_group("Icon"                     FILES ${default_icon_src})
source_group("Config"                   FILES ${config_file})
source_group("RenderModule"             FILES ${lpm_src})
