cmake_minimum_required(VERSION 3.22)

project(GpuApi)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(VULKAN_SOURCES
    src/vulkan/vulkan_api.cpp
    src/vulkan/vulkan_api.hpp
    src/vulkan/vulkan_buffer.cpp
    src/vulkan/vulkan_buffer.hpp
    src/vulkan/vulkan_command_buffer.cpp
    src/vulkan/vulkan_command_buffer.hpp
    src/vulkan/vulkan_device.cpp
    src/vulkan/vulkan_device.hpp
    src/vulkan/vulkan_exception.hpp
    src/vulkan/vulkan_graphics_pipeline.cpp
    src/vulkan/vulkan_graphics_pipeline.hpp
    src/vulkan/vulkan_image.cpp
    src/vulkan/vulkan_image.hpp
    src/vulkan/vulkan_memory_manager.cpp
    src/vulkan/vulkan_memory_manager.hpp
    src/vulkan/vulkan_shader.cpp
    src/vulkan/vulkan_shader.hpp
    src/vulkan/vulkan_shared_object.hpp
    src/vulkan/vulkan_swapchain.cpp
    src/vulkan/vulkan_swapchain.hpp
    src/vulkan/vulkan_validation.hpp
)

set(INCLUDE_FILES
    inc/gpu_api.hpp
    inc/gpu_buffer.hpp
    inc/gpu_command_buffer.hpp
    inc/gpu_device.hpp
    inc/gpu_image.hpp
    inc/gpu_pipeline.hpp
    inc/gpu_queue.hpp
    inc/gpu_swapchain.hpp
    inc/gpu_sync.hpp
    inc/gpu_types.hpp
)

set(D3D12_FILES
    src/d3d12/d3d12_api.cpp
    src/d3d12/d3d12_api.hpp
    src/d3d12/d3d12_buffer.cpp
    src/d3d12/d3d12_buffer.hpp
    src/d3d12/d3d12_command_buffer.cpp
    src/d3d12/d3d12_command_buffer.hpp
    src/d3d12/d3d12_common.hpp
    src/d3d12/d3d12_descriptor_manager.cpp
    src/d3d12/d3d12_descriptor_manager.hpp
    src/d3d12/d3d12_device.cpp
    src/d3d12/d3d12_device.hpp
    src/d3d12/d3d12_exception.hpp
    src/d3d12/d3d12_image.cpp
    src/d3d12/d3d12_image.hpp
    src/d3d12/d3d12_pipeline.cpp
    src/d3d12/d3d12_pipeline.hpp
    src/d3d12/d3d12_queue.cpp
    src/d3d12/d3d12_queue.hpp
    src/d3d12/d3d12_shader_manager.cpp
    src/d3d12/d3d12_shader_manager.hpp
    src/d3d12/d3d12_swapchain.cpp
    src/d3d12/d3d12_swapchain.hpp
    src/d3d12/d3d12_sync.cpp
    src/d3d12/d3d12_sync.hpp
)

set(COMMON_SOURCES
    src/common/gpu_api.cpp
    src/common/utils.hpp
)

set(DXC_PATH ${CMAKE_SOURCE_DIR}/third_party/dxc_2022_12_16)

add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES
    IMPORTED_LOCATION ${DXC_PATH}/bin/x64/dxcompiler.dll
    IMPORTED_IMPLIB ${DXC_PATH}/lib/x64/dxcompiler.lib
)

#source_group("vulkan" FILES ${VULKAN_SOURCES})
source_group("d3d12" FILES ${D3D12_FILES})
add_library(GpuApi STATIC ${INCLUDE_FILES} ${D3D12_FILES} ${COMMON_SOURCES})
target_include_directories(GpuApi PUBLIC inc)
target_include_directories(GpuApi PRIVATE ${DXC_PATH}/inc)
target_link_libraries(GpuApi d3d12.lib dxgi.lib dxcompiler)
add_subdirectory(samples)
#target_link_libraries(GpuApi Vulkan::Vulkan)
#target_compile_features(VulkanLabs PUBLIC cxx_std_17)
