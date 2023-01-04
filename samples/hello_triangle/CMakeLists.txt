set(SOURCES main.cpp)

add_executable(HelloTriangle ${SOURCES})
target_link_libraries(HelloTriangle PRIVATE GpuApi glfw3::glfw3)
