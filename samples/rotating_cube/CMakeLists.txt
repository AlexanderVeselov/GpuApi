set(SOURCES
    main.cpp
    shader.ps
    shader.vs
    )

add_executable(RotatingCube ${SOURCES})
target_link_libraries(RotatingCube PRIVATE GpuApi glfw3::glfw3)
set_target_properties(
    RotatingCube PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(TARGET RotatingCube POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GPU_API_DXC_BIN_DIR}/dxcompiler.dll"
            "$<TARGET_FILE_DIR:RotatingCube>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GPU_API_DXC_BIN_DIR}/dxil.dll"
            "$<TARGET_FILE_DIR:RotatingCube>/dxil.dll")
