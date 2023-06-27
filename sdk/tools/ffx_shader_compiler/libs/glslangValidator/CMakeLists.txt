add_library(glslangValidator INTERFACE)
target_include_directories(dxc INTERFACE BEFORE "inc/")

set(glslangValidator_binaries
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/glslangValidator.exe)

copyTargetCommand("${glslangValidator_binaries}" ${CMAKE_HOME_DIRECTORY}/bin/Debug copied_glslangValidator_bin_debug)
copyTargetCommand("${glslangValidator_binaries}" ${CMAKE_HOME_DIRECTORY}/bin/Release copied_glslangValidator_bin_release)
add_dependencies(glslangValidator copied_glslangValidator_bin_debug copied_glslangValidator_bin_release)