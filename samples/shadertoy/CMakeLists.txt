set(SOURCES
    main.cpp
    shader.cs
    )

add_executable(Shadertoy ${SOURCES})
target_link_libraries(Shadertoy PRIVATE GpuApi glfw3::glfw3)
set_target_properties(
    Shadertoy PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(TARGET Shadertoy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GPU_API_DXC_BIN_DIR}/dxcompiler.dll"
            "$<TARGET_FILE_DIR:Shadertoy>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GPU_API_DXC_BIN_DIR}/dxil.dll"
            "$<TARGET_FILE_DIR:Shadertoy>/dxil.dll")
