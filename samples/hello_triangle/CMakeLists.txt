set(SOURCES main.cpp)

add_executable(HelloTriangle ${SOURCES})
target_link_libraries(HelloTriangle PRIVATE GpuApi glfw3::glfw3)
set_target_properties(
    HelloTriangle PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
