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

# Pull in all the of the samples available and put them in a sample folder

# To aid with remapping FFX samples across cauldron/native backends
set(ffx_remap ${CMAKE_CURRENT_SOURCE_DIR}/validation_remap.h)

# FidelityFX Samples
if (FFX_FSR OR FFX_ALL)
	add_subdirectory(fsr)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_FSR PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_FSR_NATIVE PARENT_SCOPE)
	endif()
endif()

if ((FFX_SPD AND (NOT FFX_HYBRIDREFLECTIONS)) OR FFX_ALL)
	add_subdirectory(spd)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_SPD PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_SPD_NATIVE PARENT_SCOPE)
	endif()	
endif()

if (FFX_CACAO OR FFX_ALL)
	add_subdirectory(cacao)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_CACAO PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_CACAO_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_LPM OR FFX_ALL)
	add_subdirectory(lpm)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_LPM PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_LPM_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_BLUR OR FFX_ALL)
	add_subdirectory(blur)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_BLUR PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_BLUR_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_PARALLEL_SORT OR FFX_ALL)
	add_subdirectory(parallelsort)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_PARALLEL_SORT PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_PARALLEL_SORT_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_VRS OR FFX_ALL)
	add_subdirectory(vrs)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_VRS PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_VRS_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_CAS OR FFX_ALL)
	add_subdirectory(cas)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_CAS PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_CAS_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_SSSR OR FFX_ALL)
	add_subdirectory(sssr)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_SSSR PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_SSSR_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_DOF OR FFX_ALL)
	add_subdirectory(dof)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_DOF PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_DOF_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_LENS OR FFX_ALL)
	add_subdirectory(lens)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_LENS PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_LENS_NATIVE PARENT_SCOPE)
	endif()
endif()

if (FFX_HYBRIDSHADOWS OR FFX_ALL)
	add_subdirectory(hybridshadows)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_HYBRIDSHADOWS PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_HYBRIDSHADOWS_NATIVE PARENT_SCOPE)
	endif()
endif()


if (FFX_HYBRIDREFLECTIONS OR FFX_ALL)
	add_subdirectory(hybridreflections)
	if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_HYBRIDREFLECTIONS PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_HYBRIDREFLECTIONS_NATIVE PARENT_SCOPE)
	endif()
endif()

# If we are building everything, default FFX_FSR as the startup project
if (BUILD_TYPE STREQUAL ALL_SAMPLES)
if (FFX_API STREQUAL CAULDRON)
		set(START_PROJECT FFX_FSR PARENT_SCOPE)
	else()
		set(START_PROJECT FFX_FSR_NATIVE PARENT_SCOPE)
	endif()
endif()
