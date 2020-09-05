#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_pipeline.hpp"

#include <cassert>

namespace gpu
{
    D3D12Device::D3D12Device(Api& gpu_api, IDXGIAdapter1* dxgi_adapter)
        : api_(gpu_api)
    {
        ThrowIfFailed(D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device_)));

        graphics_queue_ = std::make_unique<D3D12Queue>(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
        compute_queue_ = std::make_unique<D3D12Queue>(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
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

    //GraphicsPipelinePtr D3D12Device::CreateGraphicsPipeline()
    //{
    //    //return std::make_shared<D3D12GraphicsPipeline>();
    //}
    //
    //ComputePipelinePtr D3D12Device::CreateComputePipeline()
    //{
    //
    //}



}
