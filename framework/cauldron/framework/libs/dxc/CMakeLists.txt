add_library(dxc INTERFACE)
target_include_directories(dxc INTERFACE BEFORE "inc/")

set(dxc_binaries
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/dxcompiler.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/dxil.dll)

copyTargetCommand("${dxc_binaries}" ${BIN_OUTPUT} copied_dxc_bin)
add_dependencies(dxc copied_dxc_bin)