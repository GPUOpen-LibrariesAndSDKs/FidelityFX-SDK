# This file is part of the FidelityFX SDK.
#
# Copyright (C) 2023 Advanced Micro Devices, Inc.
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

file(GLOB imguifiles
     ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui/*.h
     ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui/*.cpp
	 ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui/backends/*.h
     ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui/backends/*.cpp)

file(GLOB miscfiles
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/misc/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/*.cpp)

file(GLOB miscfiles_dx12
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/dx12/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/dx12/*.cpp)

file(GLOB miscfiles_vk
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/vk/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/vk/*.cpp)

file(GLOB corefiles
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/core/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)

file (GLOB corefiles_components
	${CMAKE_CURRENT_SOURCE_DIR}/../inc/core/components/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/components/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/components/*.cpp)

file (GLOB corefiles_loaders
	${CMAKE_CURRENT_SOURCE_DIR}/../inc/core/loaders/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/loaders/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/loaders/*.cpp)

file(GLOB corefiles_windows
    ${CMAKE_CURRENT_SOURCE_DIR}/core/win/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/win/*.cpp)

file(GLOB renderfiles
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/render/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp)

file(GLOB renderfiles_dx12
    ${CMAKE_CURRENT_SOURCE_DIR}/render/dx12/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/dx12/*.cpp)

file(GLOB renderfiles_vk
    ${CMAKE_CURRENT_SOURCE_DIR}/render/vk/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/vk/*.cpp)

file(GLOB renderfiles_win
    ${CMAKE_CURRENT_SOURCE_DIR}/render/win/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/win/*.cpp)

file(GLOB rendermodules_swapchain
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/swapchain/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/swapchain/*.cpp)

file(GLOB rendermodules_ui
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/ui/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/ui/*.cpp)

file(GLOB rendermodules_tonemapping
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/tonemapping/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/tonemapping/*.cpp)
	
file(GLOB rendermodules_fpslimiter
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/fpslimiter/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render/rendermodules/fpslimiter/*.cpp)

file(GLOB memoryallocator_dx12
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/memoryallocator/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/memoryallocator/*.cpp)

file(GLOB shaderfiles
	${CMAKE_CURRENT_SOURCE_DIR}/../inc/shaders/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/../inc/shaders/*.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/shaders/tonemapping/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/../inc/shaders/tonemapping/*.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../inc/shaders/fpslimiter/*.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)

set_source_files_properties(${shaderfiles} PROPERTIES VS_TOOL_OVERRIDE "Text")
# Copy shaders to bin directory
copyCommand("${shaderfiles}" ${SHADER_OUTPUT})

set(cauldronconfigfiles ${CMAKE_CURRENT_SOURCE_DIR}/../config/cauldronconfig.json)
copyCommand("${cauldronconfigfiles}" ${CONFIG_OUTPUT})

# CUSTOM_CHANGES_TO_FSR3 Build FSR3 DLL with RelWithDebInfo configuration
add_library(Framework STATIC
    ${imguifiles} ${miscfiles} ${corefiles} ${cauldronconfigfiles} ${corefiles_components} ${corefiles_loaders} ${corefiles_windows} ${renderfiles} ${renderfiles_win} ${rendermodules_swapchain} ${rendermodules_ui} ${rendermodules_tonemapping} ${rendermodules_fpslimiter} ${shaderfiles}
    $<$<OR:$<CONFIG:DebugVK>,$<CONFIG:ReleaseVK>>: ${miscfiles_vk} ${renderfiles_vk}>
    $<$<OR:$<CONFIG:DebugDX12>,$<CONFIG:ReleaseDX12>,$<CONFIG:RelWithDebInfoDX12>>: ${miscfiles_dx12} ${renderfiles_dx12} ${memoryallocator_dx12}>)

target_compile_definitions(Framework PRIVATE RenderModuleRoot="${RENDERMODULE_ROOT}")

target_link_libraries (Framework stb imgui
    $<$<OR:$<CONFIG:DebugVK>,$<CONFIG:ReleaseVK>>: amd_acs dxc "Vulkan::Vulkan" Xinput Shcore>
    $<$<OR:$<CONFIG:DebugDX12>,$<CONFIG:ReleaseDX12>,$<CONFIG:RelWithDebInfoDX12>>: amd_acs dxc DXGI amd_ags memoryallocator
	D3D12 pixlib agilitysdk dxheaders Xinput Shcore>)

source_group("Config"							             FILES ${cauldronconfigfiles})
source_group("Core"                 			             FILES ${corefiles})
source_group("Core\\Components"    				             FILES ${corefiles_components})
source_group("Core\\Loaders"					             FILES ${corefiles_loaders})
source_group("Core\\Win"            			             FILES ${corefiles_windows})
source_group("Misc"                 			             FILES ${miscfiles})
source_group("Misc\\DX12"              			             FILES ${miscfiles_dx12})
source_group("Misc\\VK"                 		             FILES ${miscfiles_vk})
source_group("Render"               			             FILES ${renderfiles})
source_group("Render\\Rendermodules")
source_group("Render\\Rendermodules\\Swapchain"	             FILES ${rendermodules_swapchain})
source_group("Render\\Rendermodules\\UI"		             FILES ${rendermodules_ui})
source_group("Render\\Rendermodules\\Tonemapping"            FILES ${rendermodules_tonemapping})
source_group("Render\\Rendermodules\\FPSLimiter"             FILES ${rendermodules_fpslimiter})
source_group("Render\\DX12"         			             FILES ${renderfiles_dx12})
source_group("Render\\VK"           			             FILES ${renderfiles_vk})
source_group("Render\\Win"          			             FILES ${renderfiles_win})
source_group("Shaders"              			             FILES ${shaderfiles})
source_group("ThirdParty")
source_group("ThirdParty\\ImGui"        		             FILES ${imguifiles})
source_group("ThirdParty\\MemoryAllocator"  	             FILES ${memoryallocator_dx12})
