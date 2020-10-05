#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_swapchain.hpp"
#include "d3d12_sync.hpp"

#include <cassert>

namespace gpu
{
    D3D12Device::D3D12Device(D3D12Api& gpu_api, IDXGIAdapter1* dxgi_adapter)
        : api_(gpu_api)
    {
        ThrowIfFailed(D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device_)));

        graphics_queue_ = std::make_unique<D3D12Queue>(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
        compute_queue_ = std::make_unique<D3D12Queue>(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);

        rtv_desc_manager_ = std::make_unique<D3D12DescriptorManager>(*this,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        cbv_srv_uav_desc_manager_ = std::make_unique<D3D12DescriptorManager>(*this,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    Queue& D3D12Device::GetQueue(QueueType queue_type)
    {
        switch (queue_type)
        {
        case QueueType::kGraphics:
            return *graphics_queue_;
        case QueueType::kCompute:
            return *compute_queue_;
        default:
            assert(!"D3D12Device::GetQueue: unimplemented");
            return *graphics_queue_;
        }
    }

    ImagePtr D3D12Device::CreateImage(std::uint32_t width, std::uint32_t height, ImageFormat format)
    {
        return std::make_shared<D3D12Image>(*this, width, height, format);
    }

    GraphicsPipelinePtr D3D12Device::CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc)
    {
        return std::make_unique<D3D12GraphicsPipeline>(*this, pipeline_desc);
    }

    //ComputePipelinePtr D3D12Device::CreateComputePipeline()
    //{
    //
    //}

    SwapchainPtr D3D12Device::CreateSwapchain(HWND hwnd, std::uint32_t width, std::uint32_t height)
    {
        return std::make_unique<D3D12Swapchain>(*this, hwnd, width, height);
    }

    FencePtr D3D12Device::CreateFence()
    {
        return std::make_shared<D3D12Fence>(*this);
    }


}
