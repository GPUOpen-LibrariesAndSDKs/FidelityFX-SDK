# This file is part of the FidelityFX SDK.
#
# Copyright (C) 2023 Advanced Micro Devices, Inc.
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

if (NOT DEFINED BUILD_TYPE)
    message(FATAL_ERROR "No BUILD_TYPE specified!)")
endif()

if (FFX_API STREQUAL CAULDRON)
    set(SOLUTION_NAME "FidelityFX SDK")
else()
    set(SOLUTION_NAME "FidelityFX Native SDK")
endif()

project ("${SOLUTION_NAME}" VERSION 1.0.0 LANGUAGES C CXX)
message(STATUS "Building FidelityFX SDK")

# Pull in common definitions and functions
list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )
include(common)
include(sample)

# Configuration setup
set(CMAKE_GENERATOR_PLATFORM x64)

if (CMAKE_GENERATOR_PLATFORM STREQUAL x64)
    message(STATUS "Creating sample configs ...")
    createConfig(DX12 DebugDX12 DEBUG)
    createConfig(VK DebugVK DEBUG)
    createConfig(DX12 ReleaseDX12 RELEASE)
    createConfig(DX12 RelWithDebInfoDX12 RELWITHDEBINFO)
    createConfig(VK ReleaseVK RELEASE)
    set(CMAKE_CONFIGURATION_TYPES "DebugDX12;DebugVK;ReleaseDX12;ReleaseVK;RelWithDebInfoDX12")
    message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

    # Set compile definitions as well
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        $<$<CONFIG:DebugDX12>:_DX12 _WIN>
        $<$<CONFIG:DebugVK>:_VK _WIN>
        $<$<CONFIG:ReleaseDX12>:_DX12 _WIN _RELEASE>
        $<$<CONFIG:RelWithDebInfoDX12>:_DX12 _WIN _RELEASE>
        $<$<CONFIG:ReleaseVK>:_VK _WIN _RELEASE>
        NOMINMAX
    )
else()
    message(FATAL_ERROR "Requested architecture not yet supported!)")
endif()

# Ouput exe to bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT})
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Setup various FFX build options - but clear them out first
unset(FFX_ALL CACHE)
unset(FFX_FSR CACHE)
unset(FFX_FSR1 CACHE)
unset(FFX_FSR2 CACHE)
unset(FFX_FSR3 CACHE)
unset(FFX_SPD CACHE)
unset(FFX_CACAO CACHE)
unset(FFX_LPM CACHE)
unset(FFX_BLUR CACHE)
unset(FFX_VRS CACHE)
unset(FFX_CAS CACHE)
unset(FFX_DOF CACHE)
unset(FFX_LENS CACHE)
unset(FFX_PARALLEL_SORT CACHE)
unset(FFX_DENOISER CACHE)
unset(FFX_SSSR CACHE)
unset(FFX_CLASSIFIER CACHE)
unset(FFX_HYBRIDSHADOWS CACHE)
unset(FFX_HYBRIDREFLECTIONS CACHE)
unset(FFX_API_CUSTOM CACHE)

if (BUILD_TYPE STREQUAL ALL_SAMPLES)
    message(STATUS "Enabling build of all Effects")
    option(FFX_ALL "Build All FidelityFX Samples and APIs" ON)
else()
	#Only enable the FFX API for the effect requested
	option(FFX_ALL "Build All FidelityFX Samples and APIs" OFF)
	if (BUILD_TYPE STREQUAL FFX_FSR)
		option(FFX_FSR "Build FSR Sample" ON)
		option(FFX_FSR1 "Build FFX FSR1 API" ON)
		option(FFX_FSR2 "Build FFX FSR2 API" ON)
		option(FFX_FSR3 "Build FFX FSR3 API" ON)
	endif()

    if (BUILD_TYPE STREQUAL FFX_CACAO)
        option(FFX_CACAO "Build CACAO Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_SPD)
        option(FFX_SPD "Build SPD Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_LPM)
        option(FFX_LPM "Build LPM Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_BLUR)
        option(FFX_BLUR "Build BLUR Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_PARALLEL_SORT)
        option(FFX_PARALLEL_SORT "Build Parallel Sort Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_VRS)
        option(FFX_VRS "Build VRS Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_CAS)
        option(FFX_CAS "Build CAS Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_DOF)
        option(FFX_DOF "Build DoF Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_HYBRIDSHADOWS)
        option(FFX_HYBRIDSHADOWS "Build Hybrid Shadows Sample" ON)
        option(FFX_CLASSIFIER "Build Classifier" ON)
        option(FFX_DENOISER "Build Denoiser" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_LENS)
        option(FFX_LENS "Build LENS Sample" ON)
    endif()

    if (BUILD_TYPE STREQUAL FFX_SSSR)
	option(FFX_SSSR "Build SSSR Sample" ON)
	option(FFX_DENOISER "Build Denoiser Sample" ON)
    endif()
	
    if (BUILD_TYPE STREQUAL FFX_HYBRIDREFLECTIONS)
	option(FFX_HYBRIDREFLECTIONS "Build Hybrid Reflections Sample" ON)
	option(FFX_CLASSIFIER "Build Classifier Sample" ON)
	option(FFX_DENOISER "Build Denoiser Sample" ON)
	option(FFX_SPD "Build SPD Sample" ON)
    endif()
endif()

# Pull in cauldron
add_subdirectory(${CAULDRON_ROOT})
set_target_properties(Framework PROPERTIES FOLDER Framework)

# Build rendermodules used by samples as a separate lib
add_subdirectory(${RENDERMODULE_ROOT})
set_target_properties(RenderModules PROPERTIES FOLDER Framework)

if( BUILD_TYPE STREQUAL CAULDRON)
    # Set as startup project
    set_property(DIRECTORY ${CMAKE_PROJECT_DIR} PROPERTY VS_STARTUP_PROJECT Cauldron)
    return()
endif()

# Pull in SDK
option(FFX_API_CUSTOM "Use custom ffx build targets" ON)
add_subdirectory(sdk)

# Are we building with cauldron backend, or defaults (VK/DX12/...)
if (FFX_API STREQUAL CAULDRON)
    # Set compile definitions for used API
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS FFX_API_CAULDRON)

    # Build cauldron backend
    add_subdirectory(${FFX_API_CAULDRON_ROOT})
else()
	# Set compile definitions for used API
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
		$<$<OR:$<CONFIG:DebugDX12>,$<CONFIG:ReleaseDX12>,$<CONFIG:RelWithDebInfoDX12>>:FFX_API_DX12>
		$<$<OR:$<CONFIG:DebugVK>,$<CONFIG:ReleaseVK>>:FFX_API_VK>)	
		
	# Import libs
	if (FFX_BUILD_AS_DLL)
		add_library(ffx_backend_native SHARED IMPORTED)
        	set_target_properties(ffx_backend_native PROPERTIES 
				IMPORTED_CONFIGURATIONS "DebugDX12;DebugVK;ReleaseDX12;ReleaseVK;RelWithDebInfoDX12"  
				IMPORTED_IMPLIB_DEBUGDX12 "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_dx12_x64d.lib"
                        	IMPORTED_LOCATION_DEBUGDX12 "${SDK_ROOT}/build/src/backends/dx12/debug/ffx_backend_dx12_x64d.dll"
				IMPORTED_IMPLIB_RELEASEDX12 "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_dx12_x64.lib"
                        	IMPORTED_LOCATION_RELEASEDX12 "${SDK_ROOT}/build/src/backends/dx12/release/ffx_backend_dx12_x64.dll"
                IMPORTED_IMPLIB_RELWITHDEBINFODX12 "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_dx12_x64.lib"
                        	IMPORTED_LOCATION_RELWITHDEBINFODX12 "${SDK_ROOT}/build/src/backends/dx12/RelWithDebInfo/ffx_backend_dx12_x64.dll"
				IMPORTED_IMPLIB_DEBUGVK "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_vk_x64d.lib"
                        	IMPORTED_LOCATION_DEBUGVK "${SDK_ROOT}/build/src/backends/vk/debug/ffx_backend_vk_x64d.dll"
				IMPORTED_IMPLIB_RELEASEVK "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_vk_x64.lib"
                        	IMPORTED_LOCATION_RELEASEVK "${SDK_ROOT}/build/src/backends/vk/release/ffx_backend_vk_x64.dll")

		ADD_CUSTOM_TARGET(CopyDLL ALL
                   	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
			$<$<CONFIG:DebugDX12>:"${SDK_ROOT}/build/src/backends/dx12/debug/ffx_backend_dx12_x64d.dll"> 
            $<$<CONFIG:DebugDX12>:"${SDK_ROOT}/build/src/backends/dx12/debug/ffx_backend_dx12_x64d.pdb"> 
			$<$<CONFIG:ReleaseDX12>:"${SDK_ROOT}/build/src/backends/dx12/release/ffx_backend_dx12_x64.dll">
            $<$<CONFIG:RelWithDebInfoDX12>:"${SDK_ROOT}/build/src/backends/dx12/RelWithDebInfo/ffx_backend_dx12_x64.dll">
            $<$<CONFIG:RelWithDebInfoDX12>:"${SDK_ROOT}/build/src/backends/dx12/RelWithDebInfo/ffx_backend_dx12_x64.pdb">
			$<$<CONFIG:DebugVK>:"${SDK_ROOT}/build/src/backends/vk/debug/ffx_backend_vk_x64d.dll"> 
			$<$<CONFIG:ReleaseVK>:"${SDK_ROOT}/build/src/backends/vk/release/ffx_backend_vk_x64.dll">
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	else()
		add_library(ffx_backend_native STATIC IMPORTED)
	    set_target_properties(ffx_backend_native PROPERTIES
							IMPORTED_CONFIGURATIONS "DebugDX12;DebugVK;ReleaseDX12;ReleaseVK"  
							IMPORTED_LOCATION_DEBUGDX12 "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_dx12_x64d.lib"
							IMPORTED_LOCATION_RELEASEDX12 "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_dx12_x64.lib"
                            IMPORTED_LOCATION_RELWITHDEBINFODX12 "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_dx12_x64.lib"
							IMPORTED_LOCATION_DEBUGVK "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_vk_x64d.lib"
							IMPORTED_LOCATION_RELEASEVK "${SDK_ROOT}/bin/ffx_sdk/ffx_backend_vk_x64.lib")
	endif()
	

	ADD_CUSTOM_TARGET(symlink-shaders-FSR3 ALL
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${SDK_ROOT}/src/backends/dx12/shaders/fsr3upscaler ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ffx_sdk/fsr3upscaler)
	ADD_CUSTOM_TARGET(symlink-shaders-FSR2 ALL
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${SDK_ROOT}/src/backends/dx12/shaders/fsr2 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ffx_sdk/fsr2)
	ADD_CUSTOM_TARGET(symlink-shaders-OF ALL
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${SDK_ROOT}/src/backends/dx12/shaders/opticalflow ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ffx_sdk/opticalflow)
	ADD_CUSTOM_TARGET(symlink-shaders-FI ALL
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${SDK_ROOT}/src/backends/dx12/shaders/frameinterpolation ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ffx_sdk/frameinterpolation)
	ADD_CUSTOM_TARGET(symlink-shader-includes ALL
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${SDK_ROOT}/include/FidelityFX/gpu ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ffx_sdk/fidelityfx-sdk-gpu)

	add_dependencies(symlink-shaders-FSR3 Framework)
	add_dependencies(symlink-shaders-FSR2 Framework)
	add_dependencies(symlink-shaders-OF Framework)
	add_dependencies(symlink-shaders-FI Framework)
	add_dependencies(symlink-shader-includes Framework)
endif()

# Build all other effect samples
add_subdirectory(samples)

# Set a startup project
message(STATUS "Default project set to ${START_PROJECT}")
set_property(DIRECTORY ${CMAKE_PROJECT_DIR} PROPERTY VS_STARTUP_PROJECT ${START_PROJECT})
