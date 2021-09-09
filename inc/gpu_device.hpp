#ifndef GPU_DEVICE_HPP_
#define GPU_DEVICE_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class Device
    {
    public:
        // Resources
        virtual BufferPtr CreateBuffer(std::size_t size) = 0;
        virtual ImagePtr CreateImage(std::uint32_t width, std::uint32_t height, ImageFormat format) = 0;

        // Synchronization primitives
        //virtual SemaphorePtr CreateSemaphore() = 0;
        virtual FencePtr CreateFence() = 0;

        // Queues
        virtual Queue& GetQueue(QueueType queue_type) = 0;

        // Pipelines
        virtual GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) = 0;
        virtual ComputePipelinePtr CreateComputePipeline(char const* cs_filename) = 0;

        // Swapchain. Requires HWND in the case of WIN32 platform
        virtual SwapchainPtr CreateSwapchain(void* window_native_handle, std::uint32_t width, std::uint32_t height) = 0;
    };

} // namespace gpu

#endif // GPU_DEVICE_HPP_
