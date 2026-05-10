#pragma once

#include "gpu_buffer.hpp"
#include "gpu_image.hpp"
#include "gpu_types.hpp"

namespace gpu
{
/// Logical GPU device. Creates resources, queues, pipelines, and presentation objects.
class Device
{
public:
    virtual ~Device() = default;

    /// Creates a buffer with a fixed element stride and explicit usage flags.
    virtual BufferPtr CreateBuffer(size_t size, uint32_t stride, BufferFlags flags) = 0;

    /// Creates an image with explicit subresource counts and usage flags.
    virtual ImagePtr CreateImage(
        uint32_t width,
        uint32_t height,
        ImageFormat format,
        uint32_t mip_count,
        uint32_t array_size,
        ImageFlags flags) = 0;

    /// Returns a queue supporting the requested command class.
    virtual Queue& GetQueue(QueueType queue_type) = 0;

    /// Creates a graphics pipeline from shader paths and render target formats.
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(
        GraphicsPipelineDesc const& pipeline_desc) = 0;

    /// Creates a compute pipeline from a compute shader path.
    virtual ComputePipelinePtr CreateComputePipeline(char const* cs_filename) = 0;

    /// Creates a swapchain for a native platform window handle.
    virtual SwapchainPtr CreateSwapchain(
        void* window_native_handle,
        uint32_t width,
        uint32_t height,
        uint32_t image_count) = 0;
};

} // namespace gpu
