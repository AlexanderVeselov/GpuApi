#include "d3d12_device.hpp"
#include "d3d12_acceleration_structure.hpp"
#include "d3d12_buffer.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include "d3d12_imgui_renderer.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_sampler.hpp"
#include "d3d12_swapchain.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace gpu
{
namespace
{
uint64_t AlignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}
}  // namespace

D3D12Device::D3D12Device(D3D12Api& gpu_api, IDXGIAdapter1* dxgi_adapter) : api_(gpu_api)
{
    ThrowIfFailed(D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device_)));
    CheckRayQuerySupport();

    descriptor_manager_ = std::make_unique<D3D12DescriptorManager>(*this);

    graphics_queue_ = std::make_unique<D3D12Queue>(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
    compute_queue_ = std::make_unique<D3D12Queue>(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

D3D12Device::~D3D12Device()
{
    WaitIdle();
    ClearSamplerCache();
    compute_queue_.reset();
    graphics_queue_.reset();
    descriptor_manager_.reset();
}

void D3D12Device::CheckRayQuerySupport()
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    HRESULT hr = d3d12_device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
    ray_query_supported_ = SUCCEEDED(hr) && options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
}

void D3D12Device::WaitIdle()
{
    if (graphics_queue_)
    {
        graphics_queue_->WaitIdle();
    }

    if (compute_queue_)
    {
        compute_queue_->WaitIdle();
    }
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

BufferPtr D3D12Device::CreateBuffer(size_t size, uint32_t stride, BufferFlags flags)
{
    return std::make_shared<D3D12Buffer>(*this, size, stride, flags);
}

AccelerationStructurePtr D3D12Device::CreateAccelerationStructure(AccelerationStructureType type, uint64_t size)
{
    if (!SupportsRayQuery())
    {
        throw std::runtime_error("D3D12Device::CreateAccelerationStructure: ray query is not supported");
    }
    if (size == 0)
    {
        throw std::runtime_error("D3D12Device::CreateAccelerationStructure: size must be greater than zero");
    }

    size = AlignUp(size, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
    BufferPtr storage_buffer = CreateBuffer(size, 1, BufferFlags::kAccelerationStructureStorage);
    uint64_t gpu_address = storage_buffer->GetGpuAddress();
    return std::make_shared<D3D12AccelerationStructure>(*this, type, std::move(storage_buffer), gpu_address);
}

ImagePtr D3D12Device::CreateImage(uint32_t width, uint32_t height, ImageFormat format, ImageFlags flags,
    uint32_t mip_count, uint32_t array_size)
{
    return std::make_shared<D3D12Image>(*this, width, height, format, mip_count, array_size, flags);
}

SamplerPtr D3D12Device::CreateSampler(SamplerDesc const& desc)
{
    return std::make_shared<D3D12Sampler>(*this, desc);
}

GraphicsPipelinePtr D3D12Device::CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc)
{
    auto pipeline = std::make_unique<D3D12GraphicsPipeline>(*this, pipeline_desc);
    RegisterPipeline(pipeline.get());
    return pipeline;
}

ComputePipelinePtr D3D12Device::CreateComputePipeline(char const* cs_filename)
{
    auto pipeline = std::make_unique<D3D12ComputePipeline>(*this, cs_filename);
    RegisterPipeline(pipeline.get());
    return pipeline;
}

SwapchainPtr D3D12Device::CreateSwapchain(void* window_native_handle, uint32_t width, uint32_t height,
    uint32_t image_count)
{
    return std::make_unique<D3D12Swapchain>(*this, window_native_handle, width, height, image_count);
}

ImGuiRendererPtr D3D12Device::CreateImGuiRenderer(void* glfw_window, Swapchain& swapchain)
{
    return std::make_unique<D3D12ImGuiRenderer>(*this, glfw_window, swapchain);
}

}  // namespace gpu
