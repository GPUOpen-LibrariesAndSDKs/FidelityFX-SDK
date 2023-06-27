# This file is part of the FidelityFX SDK.
#
# Copyright © 2023 Advanced Micro Devices, Inc.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.17)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../common.cmake)

project(FidelityFX_SC)

# Enable multi-threaded compilation
add_compile_options(/MP)

# Check for Visual Studio 2019's build tooling
if(MSVC_TOOLSET_VERSION VERSION_LESS 142)
    message(FATAL_ERROR "Cannot find MSVC toolset version 142 or greater. Please make sure Visual Studio 2019 or newer installed")
endif()

# Generate the output binary in the /bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

# Set sources
file(GLOB sources
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

# Append lib sources	
list(APPEND sources 
	${CMAKE_CURRENT_SOURCE_DIR}/libs/MD5/md5.h 
	${CMAKE_CURRENT_SOURCE_DIR}/libs/MD5/md5.cpp  
	${CMAKE_CURRENT_SOURCE_DIR}/libs/SPIRV-Reflect/spirv_reflect.h 
	${CMAKE_CURRENT_SOURCE_DIR}/libs/SPIRV-Reflect/spirv_reflect.c)

# Setup target binary
add_executable(${PROJECT_NAME} ${sources})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Add external libs
add_subdirectory(libs/agilitysdk)
add_subdirectory(libs/dxc)
add_subdirectory(libs/glslangValidator)
add_subdirectory(libs/tiny-process-library)

# Add link time dependencies
target_link_libraries (${PROJECT_NAME} dxguid agilitysdk dxc glslangValidator tiny-process-library)
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/MD5
                                                   ${CMAKE_CURRENT_SOURCE_DIR}/libs/SPIRV-Reflect
                                                   ${CMAKE_CURRENT_SOURCE_DIR}/libs/tiny-process-library)
