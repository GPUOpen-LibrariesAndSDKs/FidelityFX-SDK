# Declare project
if (FFX_API STREQUAL CAULDRON)
	project(FFX_HYBRIDSHADOWS)
else()
	project(FFX_HYBRIDSHADOWS_NATIVE)
endif()

# ---------------------------------------------
# Fidelity FX Hybrid Shadows
# ---------------------------------------------

# Hybrid Shadows
file(GLOB hybridshadows_src
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB hybridshadows_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)
set_source_files_properties(${hybridshadows_shaders} PROPERTIES VS_TOOL_OVERRIDE "Text")
copyCommand("${hybridshadows_shaders}" ${SHADER_OUTPUT})

# Add config file
set(config_file ${CMAKE_CURRENT_SOURCE_DIR}/config/hybridshadowsconfig.json)
copyCommand("${config_file}" ${CONFIG_OUTPUT})

# Add the sample to the solution
add_executable(${PROJECT_NAME} WIN32 ${default_icon_src} ${default_entry_point} ${default_sample_files} ${config_file} ${hybridshadows_src} ${hybridshadows_shaders} ${ffx_remap})

# Setup the correct exe based on backend name
set(EXE_OUT_NAME ${PROJECT_NAME}_)

# Link everything (including the compiler for now)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Framework RenderModules d3dcompiler ffx_classifier_${FFX_PLATFORM_NAME} ffx_denoiser_${FFX_PLATFORM_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
                    OUTPUT_NAME_DEBUGDX12 "${EXE_OUT_NAME}DX12D"
                    OUTPUT_NAME_DEBUGVK "${EXE_OUT_NAME}VKD"
                    OUTPUT_NAME_RELEASEDX12 "${EXE_OUT_NAME}DX12"
                    OUTPUT_NAME_RELEASEVK "${EXE_OUT_NAME}VK"
                    VS_DEBUGGER_WORKING_DIRECTORY "${BIN_OUTPUT}")

# Add compile definitions to identify the sample
target_compile_definitions(${PROJECT_NAME} PRIVATE SampleName=L"FidelityFX Hybrid Shadows"
                                           ConfigFileName=L"configs/hybridshadowsconfig.json"
                                           ConfigSampleName="FidelityFX Hybrid Shadows"
#Add RenderModuleClass definition only if this sample will use a custom rendermodule
#This will translate to <RenderModuleName>RenderModule as a class name / include file name
                                            RenderModuleName=HybridShadows
#Add RenderModuleConfig (path to config file) only if this sample uses a custom rendermodule requiring a config file
                                            #RenderModuleConfig=L"${CMAKE_CURRENT_SOURCE_DIR}/config/config.json"
                                            )

# Add manifest info
addManifest(${PROJECT_NAME})

# Add dependency information
add_dependencies(${PROJECT_NAME} Framework)
add_dependencies(${PROJECT_NAME} RenderModules)
add_dependencies(${PROJECT_NAME} ffx_classifier_${FFX_PLATFORM_NAME})
add_dependencies(${PROJECT_NAME} ffx_denoiser_${FFX_PLATFORM_NAME})

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
source_group(""							FILES ${default_entry_point})
source_group(""							FILES ${ffx_remap})
source_group("Sample" 					FILES ${default_sample_files})
source_group("Icon"    					FILES ${default_icon_src})
source_group("Config" 					FILES ${config_file})
source_group("RenderModule"				FILES ${hybridshadows_src})
source_group("RenderModule\\Shaders"	FILES ${hybridshadows_shaders})
