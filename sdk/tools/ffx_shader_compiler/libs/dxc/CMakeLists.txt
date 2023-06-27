add_library(dxc INTERFACE)
target_include_directories(dxc INTERFACE BEFORE "inc/")

set(dxc_binaries
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/dxcompiler.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/dxil.dll)

copyTargetCommand("${dxc_binaries}" ${CMAKE_HOME_DIRECTORY}/bin/Debug copied_dxc_bin_debug)
copyTargetCommand("${dxc_binaries}" ${CMAKE_HOME_DIRECTORY}/bin/Release copied_dxc_bin_release)
add_dependencies(dxc copied_dxc_bin_debug copied_dxc_bin_release)