add_library(amd_ags SHARED IMPORTED GLOBAL)
set_property(TARGET amd_ags PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/amd_ags_x64.lib)

copyTargetCommand(${CMAKE_CURRENT_SOURCE_DIR}/amd_ags_x64.dll ${BIN_OUTPUT} copied_amd_ags_bin)
add_dependencies(amd_ags copied_amd_ags_bin)
