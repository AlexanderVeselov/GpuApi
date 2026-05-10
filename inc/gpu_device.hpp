#pragma once

#include "gpu_buffer.hpp"
#include "gpu_image.hpp"
#include "gpu_types.hpp"

namespace gpu
{
class Device
{
public:
    // Resources
    virtual BufferPtr CreateBuffer(std::size_t size, std::uint32_t stride, BufferFlags flags) = 0;
    virtual ImagePtr CreateImage(
        uint32_t width,
        uint32_t height,
        ImageFormat format,
        uint32_t mip_count,
        uint32_t array_size,
        ImageFlags flags) = 0;

    // Queues
    virtual Queue& GetQueue(QueueType queue_type) = 0;

    // Pipelines
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) = 0;
    virtual ComputePipelinePtr CreateComputePipeline(char const* cs_filename) = 0;

    // Swapchain. Requires HWND in the case of WIN32 platform
    virtual SwapchainPtr CreateSwapchain(void* window_native_handle,
        std::uint32_t width, std::uint32_t height, std::uint32_t image_count) = 0;
};

} // namespace gpu
