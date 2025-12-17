set(SOURCES
    main.cpp
    shader.ps
    shader.vs
    )

add_executable(HelloTriangle ${SOURCES})
target_link_libraries(HelloTriangle PRIVATE GpuApi glfw3::glfw3)
set_target_properties(
    HelloTriangle PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# Copy DXC runtime DLLs next to the built executable (per-config output dir)
set(DXC_BIN_DIR "${CMAKE_SOURCE_DIR}/third_party/dxc_2022_12_16/bin/x64")
add_custom_command(TARGET HelloTriangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxcompiler.dll"
            "$<TARGET_FILE_DIR:HelloTriangle>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxil.dll"
            "$<TARGET_FILE_DIR:HelloTriangle>/dxil.dll")
