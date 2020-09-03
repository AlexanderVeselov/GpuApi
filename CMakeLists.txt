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

source_group("vulkan" FILES ${VULKAN_SOURCES})
add_library(GpuApi STATIC ${INCLUDE_FILES} ${VULKAN_SOURCES})
target_include_directories(GpuApi PUBLIC inc)
target_link_libraries(GpuApi Vulkan::Vulkan spirv-cross-core)
#target_compile_features(VulkanLabs PUBLIC cxx_std_17)
