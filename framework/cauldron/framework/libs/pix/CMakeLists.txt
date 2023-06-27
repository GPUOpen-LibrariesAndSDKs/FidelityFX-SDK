add_library(pixlib SHARED IMPORTED GLOBAL)
set_property(TARGET pixlib PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/WinPixEventRuntime.lib)

set(winpixevent_binary
    ${CMAKE_CURRENT_SOURCE_DIR}/WinPixEventRuntime.dll)

set(winpixgpucapturer_binary
    ${CMAKE_CURRENT_SOURCE_DIR}/WinPixGpuCapturer.dll)


copyTargetCommand("${winpixevent_binary}" ${BIN_OUTPUT} copied_winpixevent_bin)
copyTargetCommand("${winpixgpucapturer_binary}" ${BIN_OUTPUT} winpixgpucapturer_binary_bin)

add_dependencies(pixlib copied_winpixevent_bin winpixgpucapturer_binary_bin)
