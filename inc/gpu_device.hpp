#ifndef GPU_DEVICE_HPP_
#define GPU_DEVICE_HPP_

#include "gpu_types.hpp"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace gpu
{
    class Device
    {
    public:
        // Resources
        //virtual BufferPtr CreateBuffer() = 0;
        virtual ImagePtr CreateImage(std::uint32_t width, std::uint32_t height, ImageFormat format) = 0;

        // Synchronization primitives
        //virtual SemaphorePtr CreateSemaphore() = 0;
        virtual FencePtr CreateFence() = 0;

        // Queues
        virtual Queue& GetQueue(QueueType queue_type) = 0;

        // Pipelines
        virtual GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) = 0;
        //virtual ComputePipelinePtr CreateComputePipeline() = 0;

        // Swapchain
#ifdef WIN32
        virtual SwapchainPtr CreateSwapchain(HWND hwnd, std::uint32_t width, std::uint32_t height) = 0;
#endif

    };

} // namespace gpu

#endif // GPU_DEVICE_HPP_
