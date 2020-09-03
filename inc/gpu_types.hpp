#ifndef GPU_TYPES_HPP_
#define GPU_TYPES_HPP_

#include <memory>

namespace gpu
{
    template <class T>
    using Ptr = std::unique_ptr<T>;

    template <class T>
    using SharedPtr = std::shared_ptr<T>;

    class Device;
    class Buffer;
    class CommandBuffer;
    class Device;
    class Image;
    class GraphicsPipeline;
    class ComputePipeline;
    class Queue;
    class Swapchain;
    class Semaphore;
    class Fence;

    // Unique resources
    using DevicePtr             = Ptr<Device>;
    using CommandBufferPtr      = Ptr<CommandBuffer>;
    using GraphicsPipelinePtr   = Ptr<GraphicsPipeline>;
    using ComputePipelinePtr    = Ptr<ComputePipeline>;
    using SwapchainPtr          = Ptr<Swapchain>;

    // Shared resources
    using BufferPtr             = SharedPtr<Buffer>;
    using ImagePtr              = SharedPtr<Image>;
    using SemaphorePtr          = SharedPtr<Semaphore>;
    using FencePtr              = SharedPtr<Fence>;

    enum class QueueType
    {
        kGraphics = 0,
        kCompute  = 1,
        kTransfer = 2
    };

} // namespace gpu

#endif // GPU_TYPES_HPP_
