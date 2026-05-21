#pragma once

#include "gpu_device.hpp"

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include "d3d12_queue.hpp"

namespace gpu
{
class D3D12Api;

class D3D12Device : public Device
{
public:
    D3D12Device(D3D12Api& gpu_api, IDXGIAdapter1* dxgi_adapter);
    ~D3D12Device() override;

    // Resources
    BufferPtr CreateBuffer(size_t size, uint32_t stride, BufferFlags flags) override;
    AccelerationStructurePtr CreateAccelerationStructure(AccelerationStructureType type, uint64_t size) override;
    ImagePtr CreateImage(uint32_t width, uint32_t height, ImageFormat format, ImageFlags flags, uint32_t mip_count = 1,
        uint32_t array_size = 1) override;

    Queue& GetQueue(QueueType queue_type) override;

    // Pipelines
    GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) override;
    ComputePipelinePtr CreateComputePipeline(char const* cs_filename) override;

    SwapchainPtr CreateSwapchain(void* window_native_handle, uint32_t width, uint32_t height,
        uint32_t image_count) override;
    ImGuiRendererPtr CreateImGuiRenderer(void* glfw_window, Swapchain& swapchain) override;

    ID3D12Device* GetD3D12Device() const { return d3d12_device_.Get(); }

    D3D12Api& GetD3D12Api() { return api_; }

    D3D12DescriptorManager& GetDescriptorManager() const { return *descriptor_manager_; }
    void WaitIdle() override;
    bool SupportsRayQuery() const override { return ray_query_supported_; }

private:
    void CheckRayQuerySupport();
    SamplerPtr CreateSampler(SamplerDesc const& desc) override;

private:
    D3D12Api& api_;
    ComPtr<ID3D12Device> d3d12_device_;
    std::unique_ptr<D3D12DescriptorManager> descriptor_manager_;
    std::unique_ptr<Queue> graphics_queue_;
    std::unique_ptr<Queue> compute_queue_;
    bool ray_query_supported_ = false;
};

}  // namespace gpu
