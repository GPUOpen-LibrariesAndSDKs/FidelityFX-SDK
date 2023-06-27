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

project (Framework)

# this check is temporary until we can all switch to CMAKE 3.23 that will give us the version of Vulkan directly
function (ParseVulkanHeader)

    # Check that vulkan_core.h exists
    if(NOT EXISTS "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_core.h")
        message(SEND_ERROR "Unable to find vulkan_core.h.")
        return()
    endif()
    # Fetch header version from vulkan_core.h
    file(STRINGS "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_core.h" lines REGEX "^#define VK_HEADER_VERSION_COMPLETE VK_MAKE(_API)?_VERSION\\(([0-9]+,)+")
    list(LENGTH lines len)
    if(${len} EQUAL 1)
        #message(STATUS "vulkan version in line ${lines}")
        string(REGEX MATCHALL
                    "[0-9]+"
                    vk_header_version_complete
                    ${lines})
        list(LENGTH vk_header_version_complete len)
        if(${len} GREATER_EQUAL 2)
            if(${len} EQUAL 2)
                list(GET vk_header_version_complete 0 vk_header_version_major)
                list(GET vk_header_version_complete 1 vk_header_version_minor)
            else()
                # the first one is the variant
                list(GET vk_header_version_complete 1 vk_header_version_major)
                list(GET vk_header_version_complete 2 vk_header_version_minor)
            endif()

            set(vulkan_header_version_major ${vk_header_version_major} PARENT_SCOPE)
            set(vulkan_header_version_minor ${vk_header_version_minor} PARENT_SCOPE)
        else()
            message(SEND_ERROR "Regex unable to parse the VK_HEADER_VERSION_COMPLETE. Version has ${len} numbers.")
        endif()
    elseif(${len} EQUAL 0)
        message(SEND_ERROR "Unable to find VK_HEADER_VERSION_COMPLETE from vulkan_core.h.")
    else()
        message(SEND_ERROR "Multiple VK_HEADER_VERSION_COMPLETE found in vulkan_core.h.")
    endif()

endfunction()

function (CheckVulkanSDKVersion)
    # set the desired version
    set(vulkan_min_version_major 1)
    set(vulkan_min_version_minor 3)

    # variables to be set
    set(vulkan_header_version_major 0)
    set(vulkan_header_version_minor 0)

    # find version in the header
    ParseVulkanHeader()

    # check the SDK version is enough
    if((${vulkan_header_version_major} LESS ${vulkan_min_version_major}) OR ((${vulkan_header_version_major} EQUAL ${vulkan_min_version_major}) AND (${vulkan_header_version_minor} LESS ${vulkan_min_version_minor})))
        if(NOT((${vulkan_header_version_major} EQUAL 0) OR (${vulkan_header_version_minor} EQUAL 0)))
            message(SEND_ERROR "Found Vulkan ${vulkan_header_version_major}.${vulkan_header_version_minor}.")
        endif()
        message(FATAL_ERROR "Vulkan SDK ${vulkan_min_version_major}.${vulkan_min_version_minor} or above is required.")
    endif()

endfunction()

# Reference libs used
add_subdirectory(libs/agilitysdk)
add_subdirectory(libs/acs)
add_subdirectory(libs/ags)
add_subdirectory(libs/dxc)
add_subdirectory(libs/dxheaders)
add_subdirectory(libs/imgui)
add_subdirectory(libs/json)
add_subdirectory(libs/memoryallocator)
add_subdirectory(libs/renderdoc)
add_subdirectory(libs/stb)
add_subdirectory(libs/vectormath)
add_subdirectory(libs/pix)

#Add sources
find_package(Vulkan REQUIRED)
CheckVulkanSDKVersion()
add_subdirectory(src)
