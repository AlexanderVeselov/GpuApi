cmake_minimum_required(VERSION 3.22)

project(GpuApi)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(GPU_API_BUILD_VULKAN "Build the Vulkan backend" ON)
option(GPU_API_BUILD_SAMPLES "Build GpuApi samples" ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GPU_API_ARCH_DIR x64)
else()
    set(GPU_API_ARCH_DIR x86)
endif()

set(VULKAN_SOURCES
    src/vulkan/vulkan_acceleration_structure.cpp
    src/vulkan/vulkan_acceleration_structure.hpp
    src/vulkan/vulkan_api.cpp
    src/vulkan/vulkan_api.hpp
    src/vulkan/vulkan_buffer.cpp
    src/vulkan/vulkan_buffer.hpp
    src/vulkan/vulkan_command_buffer.cpp
    src/vulkan/vulkan_command_buffer.hpp
    src/vulkan/vulkan_descriptor_set.cpp
    src/vulkan/vulkan_descriptor_set.hpp
    src/vulkan/vulkan_device.cpp
    src/vulkan/vulkan_device.hpp
    src/vulkan/vulkan_exception.hpp
    src/vulkan/vulkan_image.cpp
    src/vulkan/vulkan_image.hpp
    src/vulkan/vulkan_imgui_renderer.cpp
    src/vulkan/vulkan_imgui_renderer.hpp
    src/vulkan/vulkan_memory_manager.cpp
    src/vulkan/vulkan_memory_manager.hpp
    src/vulkan/vulkan_pipeline.cpp
    src/vulkan/vulkan_pipeline.hpp
    src/vulkan/vulkan_pipeline_layout.cpp
    src/vulkan/vulkan_pipeline_layout.hpp
    src/vulkan/vulkan_queue.cpp
    src/vulkan/vulkan_queue.hpp
    src/vulkan/vulkan_sampler.cpp
    src/vulkan/vulkan_sampler.hpp
    src/vulkan/vulkan_shader_manager.cpp
    src/vulkan/vulkan_shader_manager.hpp
    src/vulkan/vulkan_shader_reflection.cpp
    src/vulkan/vulkan_shader_reflection.hpp
    src/vulkan/vulkan_shared_object.hpp
    src/vulkan/vulkan_swapchain.cpp
    src/vulkan/vulkan_swapchain.hpp
    src/vulkan/vulkan_validation.hpp
)

set(INCLUDE_FILES
    inc/gpu_acceleration_structure.hpp
    inc/gpu_api.hpp
    inc/gpu_buffer.hpp
    inc/gpu_command_buffer.hpp
    inc/gpu_descriptor_set.hpp
    inc/gpu_device.hpp
    inc/gpu_image.hpp
    inc/gpu_imgui.hpp
    inc/gpu_pipeline.hpp
    inc/gpu_queue.hpp
    inc/gpu_sampler.hpp
    inc/gpu_swapchain.hpp
    inc/gpu_types.hpp
)

set(D3D12_FILES
    src/d3d12/d3d12_acceleration_structure.cpp
    src/d3d12/d3d12_acceleration_structure.hpp
    src/d3d12/d3d12_api.cpp
    src/d3d12/d3d12_api.hpp
    src/d3d12/d3d12_buffer.cpp
    src/d3d12/d3d12_buffer.hpp
    src/d3d12/d3d12_command_buffer.cpp
    src/d3d12/d3d12_command_buffer.hpp
    src/d3d12/d3d12_common.hpp
    src/d3d12/d3d12_descriptor_manager.cpp
    src/d3d12/d3d12_descriptor_manager.hpp
    src/d3d12/d3d12_descriptor_set.cpp
    src/d3d12/d3d12_descriptor_set.hpp
    src/d3d12/d3d12_device.cpp
    src/d3d12/d3d12_device.hpp
    src/d3d12/d3d12_exception.hpp
    src/d3d12/d3d12_image.cpp
    src/d3d12/d3d12_image.hpp
    src/d3d12/d3d12_imgui_renderer.cpp
    src/d3d12/d3d12_imgui_renderer.hpp
    src/d3d12/d3d12_pipeline.cpp
    src/d3d12/d3d12_pipeline.hpp
    src/d3d12/d3d12_pipeline_layout.cpp
    src/d3d12/d3d12_pipeline_layout.hpp
    src/d3d12/d3d12_queue.cpp
    src/d3d12/d3d12_queue.hpp
    src/d3d12/d3d12_sampler.cpp
    src/d3d12/d3d12_sampler.hpp
    src/d3d12/d3d12_shader_reflection.cpp
    src/d3d12/d3d12_shader_reflection.hpp
    src/d3d12/d3d12_shader_manager.cpp
    src/d3d12/d3d12_shader_manager.hpp
    src/d3d12/d3d12_swapchain.cpp
    src/d3d12/d3d12_swapchain.hpp
)

set(COMMON_SOURCES
    src/common/gpu_acceleration_structure.cpp
    src/common/gpu_api.cpp
    src/common/gpu_device.cpp
    src/common/gpu_pipeline.cpp
    src/common/shader_reflection.hpp
    src/common/utils.hpp
)

set(SPIRV_REFLECT_SOURCES
    third_party/SPIRV-Reflect/spirv_reflect.c
    third_party/SPIRV-Reflect/spirv_reflect.h
)

set(IMGUI_SOURCES
    third_party/imgui/imconfig.h
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui.h
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_internal.h
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imstb_rectpack.h
    third_party/imgui/imstb_textedit.h
    third_party/imgui/imstb_truetype.h
    third_party/imgui/backends/imgui_impl_dx12.cpp
    third_party/imgui/backends/imgui_impl_dx12.h
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_glfw.h
)

set(IMGUI_VULKAN_SOURCES
    third_party/imgui/backends/imgui_impl_vulkan.cpp
    third_party/imgui/backends/imgui_impl_vulkan.h
)

set(DXC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc_2022_12_16)
set(GPU_API_DXC_BIN_DIR ${DXC_PATH}/bin/${GPU_API_ARCH_DIR})
set(GPU_API_DXC_LIB_DIR ${DXC_PATH}/lib/${GPU_API_ARCH_DIR})
set(GPU_API_GLFW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/lib/${GPU_API_ARCH_DIR})

add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES
    IMPORTED_LOCATION ${GPU_API_DXC_BIN_DIR}/dxcompiler.dll
    IMPORTED_IMPLIB ${GPU_API_DXC_LIB_DIR}/dxcompiler.lib
)

if(GPU_API_BUILD_VULKAN)
    find_package(Vulkan REQUIRED)
    list(APPEND IMGUI_SOURCES ${IMGUI_VULKAN_SOURCES})
else()
    set(VULKAN_SOURCES)
    set(SPIRV_REFLECT_SOURCES)
endif()

find_package(glfw3 CONFIG QUIET)
if(NOT TARGET glfw3::glfw3)
    if(EXISTS ${GPU_API_GLFW_LIB_DIR}/glfw3.lib)
        add_library(glfw3::glfw3 STATIC IMPORTED GLOBAL)
        set_target_properties(glfw3::glfw3 PROPERTIES
            IMPORTED_LOCATION_DEBUG ${GPU_API_GLFW_LIB_DIR}/glfw3.lib
            IMPORTED_LOCATION_RELEASE ${GPU_API_GLFW_LIB_DIR}/glfw3.lib
            IMPORTED_LOCATION_MINSIZEREL ${GPU_API_GLFW_LIB_DIR}/glfw3.lib
            IMPORTED_LOCATION_RELWITHDEBINFO ${GPU_API_GLFW_LIB_DIR}/glfw3.lib
            INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
        )
    else()
        message(FATAL_ERROR
            "GpuApi requires glfw3 for ImGui support, but no ${GPU_API_ARCH_DIR} GLFW library was found. "
            "Install a matching glfw3 package or add bundled libs under third_party/glfw/lib/${GPU_API_ARCH_DIR}.")
    endif()
endif()

if(NOT TARGET imgui)
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_compile_features(imgui PUBLIC cxx_std_17)
    target_include_directories(imgui PUBLIC third_party/imgui)
    target_include_directories(imgui PRIVATE third_party/glfw/include)
    target_link_libraries(imgui PRIVATE d3d12.lib dxgi.lib d3dcompiler.lib)
    if(GPU_API_BUILD_VULKAN)
        target_compile_definitions(imgui PRIVATE VK_USE_PLATFORM_WIN32_KHR)
        target_link_libraries(imgui PRIVATE Vulkan::Vulkan)
    endif()
    target_link_libraries(imgui PUBLIC glfw3::glfw3)
endif()

source_group("vulkan" FILES ${VULKAN_SOURCES})
source_group("d3d12" FILES ${D3D12_FILES})
add_library(GpuApi STATIC
    ${INCLUDE_FILES}
    ${D3D12_FILES}
    ${VULKAN_SOURCES}
    ${COMMON_SOURCES}
    ${SPIRV_REFLECT_SOURCES}
)
target_compile_features(GpuApi PUBLIC cxx_std_17)
target_include_directories(GpuApi PUBLIC inc)
target_include_directories(GpuApi PRIVATE ${DXC_PATH}/inc)
target_include_directories(GpuApi PRIVATE src/third_party_compat)
target_include_directories(GpuApi PRIVATE third_party/glfw/include)
target_include_directories(GpuApi PRIVATE third_party/imgui)
if(GPU_API_BUILD_VULKAN)
    target_include_directories(GpuApi PRIVATE third_party/SPIRV-Reflect)
    target_compile_definitions(GpuApi PRIVATE VK_USE_PLATFORM_WIN32_KHR GPU_API_HAS_VULKAN=1)
else()
    target_compile_definitions(GpuApi PRIVATE GPU_API_HAS_VULKAN=0)
endif()
target_link_libraries(GpuApi PUBLIC imgui)
target_link_libraries(GpuApi PRIVATE d3d12.lib dxgi.lib dxcompiler)
if(GPU_API_BUILD_VULKAN)
    target_link_libraries(GpuApi PRIVATE Vulkan::Vulkan)
endif()

if(GPU_API_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
