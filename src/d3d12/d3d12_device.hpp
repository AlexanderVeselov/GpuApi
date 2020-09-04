#ifndef D3D12_DEVICE_HPP_
#define D3D12_DEVICE_HPP_

#include "gpu_device.hpp"
#include "gpu_api.hpp"

#include "d3d12_common.hpp"
#include "d3d12_queue.hpp"

namespace gpu
{
    class D3D12Device : public Device
    {
    public:
        D3D12Device(Api& gpi_api, IDXGIAdapter1* dxgi_adapter);

        // Resources
        BufferPtr CreateBuffer() override;
        ImagePtr CreateImage() override;

        // Synchronization primitives
        SemaphorePtr CreateSemaphore() override;
        FencePtr CreateFence() override;

        Queue& GetQueue(QueueType queue_type) override;

        // Pipelines
        GraphicsPipelinePtr CreateGraphicsPipeline() override;
        ComputePipelinePtr CreateComputePipeline() override;

        ID3D12Device* GetD3D12Device() const { return d3d12_device_.Get(); }

    private:
        Api& api_;
        ComPtr<ID3D12Device> d3d12_device_;
        D3D12Queue graphics_queue_;
        D3D12Queue compute_queue_;
    };

} // namespace gpu

#endif // D3D12_DEVICE_HPP_
