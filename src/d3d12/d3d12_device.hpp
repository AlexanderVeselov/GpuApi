#ifndef D3D12_DEVICE_HPP_
#define D3D12_DEVICE_HPP_

#include "gpu_device.hpp"

#include "d3d12_common.hpp"
#include "d3d12_queue.hpp"
#include "d3d12_descriptor_manager.hpp"

namespace gpu
{
    class D3D12Api;

    class D3D12Device : public Device
    {
    public:
        D3D12Device(D3D12Api& gpu_api, IDXGIAdapter1* dxgi_adapter);

        // Resources
        //BufferPtr CreateBuffer() override;
        ImagePtr CreateImage(std::uint32_t width, std::uint32_t height, ImageFormat format) override;

        // Synchronization primitives
        //SemaphorePtr CreateSemaphore() override;
        //FencePtr CreateFence() override;

        Queue& GetQueue(QueueType queue_type) override;

        // Pipelines
        GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) override;
        //ComputePipelinePtr CreateComputePipeline() override;

        SwapchainPtr CreateSwapchain(HWND hwnd, std::uint32_t width, std::uint32_t height) override;

        ID3D12Device* GetD3D12Device() const { return d3d12_device_.Get(); }

        D3D12Api& GetD3D12Api() { return api_; }

        D3D12DescriptorManager& GetRTVDescManager() const { return *rtv_desc_manager_; }
        D3D12DescriptorManager& GetCBVSRVUAVDescManager() const { return *cbv_srv_uav_desc_manager_; }

    private:
        D3D12Api& api_;
        ComPtr<ID3D12Device> d3d12_device_;
        std::unique_ptr<Queue> graphics_queue_;
        std::unique_ptr<Queue> compute_queue_;
        std::unique_ptr<D3D12DescriptorManager> rtv_desc_manager_;
        std::unique_ptr<D3D12DescriptorManager> cbv_srv_uav_desc_manager_;

    };

} // namespace gpu

#endif // D3D12_DEVICE_HPP_
