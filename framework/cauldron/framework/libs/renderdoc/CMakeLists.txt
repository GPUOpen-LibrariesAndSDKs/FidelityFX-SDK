add_library(renderdoc INTERFACE)
target_include_directories(renderdoc INTERFACE BEFORE "include/")

set(renderdoc_binaries
    ${CMAKE_CURRENT_SOURCE_DIR}/renderdoc.dll)

copyTargetCommand("${renderdoc_binaries}" ${BIN_OUTPUT} copied_renderdoc_bin)
add_dependencies(renderdoc copied_renderdoc_bin)