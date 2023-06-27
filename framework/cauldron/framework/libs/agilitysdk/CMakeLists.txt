add_library(agilitysdk INTERFACE)
target_include_directories(agilitysdk INTERFACE BEFORE "include/")

set(agilitysdk_binaries
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/D3D12Core.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/d3d12SDKLayers.dll
	${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/d3dconfig.exe)

copyTargetCommand("${agilitysdk_binaries}" ${BIN_OUTPUT}/D3D12 copied_agilitysdk_bin)
add_dependencies(agilitysdk copied_agilitysdk_bin)