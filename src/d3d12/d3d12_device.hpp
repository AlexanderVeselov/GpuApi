#ifndef D3D12_DEVICE_HPP_
#define D3D12_DEVICE_HPP_

#include "gpu_device.hpp"

#include "d3d12_common.hpp"
#include "d3d12_queue.hpp"

namespace gpu
{
    class D3D12Api;

    class D3D12Device : public Device
    {
    public:
        D3D12Device(D3D12Api& gpu_api, IDXGIAdapter1* dxgi_adapter);

        /*
        // Resources
        BufferPtr CreateBuffer() override;
        ImagePtr CreateImage() override;

        // Synchronization primitives
        SemaphorePtr CreateSemaphore() override;
        FencePtr CreateFence() override;

        */
        Queue& GetQueue(QueueType queue_type) override;

        // Pipelines
        GraphicsPipelinePtr CreateGraphicsPipeline(char const* vs_filename, char const* ps_filename) override;
        //ComputePipelinePtr CreateComputePipeline() override;

        SwapchainPtr CreateSwapchain(HWND hwnd, std::uint32_t width, std::uint32_t height) override;

        ID3D12Device* GetD3D12Device() const { return d3d12_device_.Get(); }

        D3D12Api& GetD3D12Api() { return api_; }

    private:
        D3D12Api& api_;
        ComPtr<ID3D12Device> d3d12_device_;
        std::unique_ptr<Queue> graphics_queue_;
        std::unique_ptr<Queue> compute_queue_;
    };

} // namespace gpu

#endif // D3D12_DEVICE_HPP_
