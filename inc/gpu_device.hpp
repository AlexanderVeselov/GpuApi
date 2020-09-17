#ifndef GPU_DEVICE_HPP_
#define GPU_DEVICE_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class Device
    {
    public:
        /*
        // Resources
        virtual BufferPtr CreateBuffer() = 0;
        virtual ImagePtr CreateImage() = 0;

        // Synchronization primitives
        virtual SemaphorePtr CreateSemaphore() = 0;
        virtual FencePtr CreateFence() = 0;
        */
        // Queues
        virtual Queue& GetQueue(QueueType queue_type) = 0;

        // Pipelines
        virtual GraphicsPipelinePtr CreateGraphicsPipeline(char const* vs_filename, char const* ps_filename) = 0;
        //virtual ComputePipelinePtr CreateComputePipeline() = 0;

    };

} // namespace gpu

#endif // GPU_DEVICE_HPP_
