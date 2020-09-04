#ifndef D3D12_DEVICE_HPP_
#define D3D12_DEVICE_HPP_

#include "gpu_device.hpp"

namespace gpu
{
    class D3D12Device : public Device
    {
    public:
        // Resources
        BufferPtr CreateBuffer() override;
        ImagePtr CreateImage() override;

        // Synchronization primitives
        SemaphorePtr CreateSemaphore() override;
        FencePtr CreateFence() override;

        // Pipelines
        GraphicsPipelinePtr CreateGraphicsPipeline() override;
        ComputePipelinePtr CreateComputePipeline() override;

    };

} // namespace gpu

#endif // D3D12_DEVICE_HPP_
