# This file is part of the FidelityFX SDK.
#
# Copyright � 2023 Advanced Micro Devices, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the �Software�), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.17)

# Set configuration types based on what we are building for
if (NOT FFX_API_CUSTOM)
	message(STATUS "Building FidelityFX SDK with DX12 & VK backends")
	set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)
	set(CMAKE_DEBUG_POSTFIX d)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	
	# Get warnings for everything
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
endif()
if (MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W3")
endif()

	option (FFX_API_DX12 "Build FidelityFX DX12 backend" ON)
	option (FFX_API_VK "Build FidelityFX Vulkan backend" ON)
endif()

# Set platform
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64")
    set(FFX_PLATFORM_NAME x64)
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86")
    set(FFX_PLATFORM_NAME x86)
else()
    message(FATAL_ERROR "Unsupported target platform - only supporting x64 and Win32 currently")
endif()

# Pre-compile shaders
set(FFX_AUTO_COMPILE_SHADERS ON CACHE BOOL "Compile shaders automatically as a prebuild step.")

if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(USE_DEPFILE TRUE)
else()
    set(USE_DEPFILE FALSE)
endif()

# Set Visual Studio version
if(CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015")
    set(FFX_VS_VERSION 2015)
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 15 2017")
	set(FFX_VS_VERSION 2017)
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 16 2019")
    set(FFX_VS_VERSION 2019)
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022")
	set(FFX_VS_VERSION 2022)
endif()

# Set configuration types based on what we are building for
if (NOT FFX_API_CUSTOM)

	# Embed PDBs in the debug versions of the libs
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")

	# Write both debug and release versions of the static libs to the /bin folder as they are uniquely named
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_HOME_DIRECTORY}/bin/ffx_sdk/)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_HOME_DIRECTORY}/bin/ffx_sdk/)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_HOME_DIRECTORY}/bin/ffx_sdk/)

else()

	# Ouput the static libs to the /bin folder as they are uniquely named
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT}/ffx_sdk)
	foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
		set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT}/ffx_sdk/ )
	endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

endif()

# Add requred compile definitions
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

if(FFX_VS_VERSION STREQUAL 2015 OR FFX_VS_VERSION STREQUAL 2017)
    message(NOTICE "Forcing the SDK path for VS 2015 and VS 2017")
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.18362.0")
endif()

# Setup common variables
set(FFX_SC_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/tools/binary_store/FidelityFX_SC.exe)
set(FFX_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(FFX_SHARED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/shared)
set(FFX_HOST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/FidelityFX/host)
set(FFX_GPU_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/FidelityFX/gpu)
set(FFX_COMPONENTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/components)
set(FFX_HOST_BACKENDS_PATH ${FFX_HOST_PATH}/backends)
set(FFX_BACKENDS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/backends)
set(FFX_SRC_BACKENDS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/backends)

# Promote common variables up so the common code can also access it if needed
if (FFX_API_CUSTOM)

	set(FFX_PLATFORM_NAME ${FFX_PLATFORM_NAME} PARENT_SCOPE)
	set(FFX_VS_VERSION ${FFX_VS_VERSION} PARENT_SCOPE)
	set(FFX_SC_EXECUTABLE ${FFX_SC_EXECUTABLE} PARENT_SCOPE)
	set(FFX_INCLUDE_PATH ${FFX_INCLUDE_PATH} PARENT_SCOPE)
	set(FFX_SHARED_PATH ${FFX_SHARED_PATH} PARENT_SCOPE)
	set(FFX_HOST_PATH ${FFX_HOST_PATH} PARENT_SCOPE)
	set(FFX_GPU_PATH ${FFX_GPU_PATH} PARENT_SCOPE)
	set(FFX_COMPONENTS_PATH ${FFX_COMPONENTS_PATH} PARENT_SCOPE)
	set(FFX_HOST_BACKENDS_PATH ${FFX_HOST_BACKENDS_PATH} PARENT_SCOPE)
	set(FFX_SRC_BACKENDS_PATH ${FFX_SRC_BACKENDS_PATH} PARENT_SCOPE)

endif()

# ------------------------------------------------------
# Define common source groupings used by all effects

# Set shared and global public sources
file(GLOB FFX_SHARED_SOURCES
    "${FFX_SHARED_PATH}/*.cpp"
    "${FFX_SHARED_PATH}/*.h")

set(FFX_PUBLIC_SOURCES
    "${FFX_HOST_PATH}/ffx_assert.h"
    "${FFX_HOST_PATH}/ffx_error.h"
	"${FFX_HOST_PATH}/ffx_fx.h"
	"${FFX_HOST_PATH}/ffx_interface.h"
    "${FFX_HOST_PATH}/ffx_types.h"
    "${FFX_HOST_PATH}/ffx_util.h")

# Init shader sources (for easy viewing)
set(FFX_PUBLIC_SHADER_SOURCES
    "${FFX_GPU_PATH}/ffx_common_types.h"
    "${FFX_GPU_PATH}/ffx_core.h"
    "${FFX_GPU_PATH}/ffx_core_cpu.h"
    "${FFX_GPU_PATH}/ffx_core_glsl.h"
    "${FFX_GPU_PATH}/ffx_core_gpu_common.h"
    "${FFX_GPU_PATH}/ffx_core_gpu_common_half.h"
    "${FFX_GPU_PATH}/ffx_core_hlsl.h"
    "${FFX_GPU_PATH}/ffx_core_portability.h")

project (FidelityFX-SDK)

# Components
add_subdirectory(${FFX_COMPONENTS_PATH}/opticalflow)
add_subdirectory(${FFX_COMPONENTS_PATH}/frameinterpolation)
add_subdirectory(${FFX_COMPONENTS_PATH}/fsr3)
add_subdirectory(${FFX_COMPONENTS_PATH}/fsr3upscaler)
add_subdirectory(${FFX_COMPONENTS_PATH}/fsr2)
add_subdirectory(${FFX_COMPONENTS_PATH}/fsr1)
add_subdirectory(${FFX_COMPONENTS_PATH}/spd)
add_subdirectory(${FFX_COMPONENTS_PATH}/cacao)
add_subdirectory(${FFX_COMPONENTS_PATH}/lpm)
add_subdirectory(${FFX_COMPONENTS_PATH}/blur)
add_subdirectory(${FFX_COMPONENTS_PATH}/vrs)
add_subdirectory(${FFX_COMPONENTS_PATH}/cas)
add_subdirectory(${FFX_COMPONENTS_PATH}/dof)
add_subdirectory(${FFX_COMPONENTS_PATH}/lens)
add_subdirectory(${FFX_COMPONENTS_PATH}/parallelsort)
add_subdirectory(${FFX_COMPONENTS_PATH}/denoiser)
add_subdirectory(${FFX_COMPONENTS_PATH}/sssr)
add_subdirectory(${FFX_COMPONENTS_PATH}/classifier)

# Add appropriate graphics backend if requested
if(FFX_API_DX12)
    add_subdirectory(${FFX_SRC_BACKENDS_PATH}/dx12)
endif()

if(FFX_API_VK)
    add_subdirectory(${FFX_SRC_BACKENDS_PATH}/vk)
endif()
