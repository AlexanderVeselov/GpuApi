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

D3D12_RAYTRACING_GEOMETRY_DESC ToD3D12GeometryDesc(AccelerationStructureGeometryDesc const& geometry)
{
    auto* vertex_buffer = dynamic_cast<D3D12Buffer*>(geometry.vertex_buffer.get());
    auto* index_buffer = dynamic_cast<D3D12Buffer*>(geometry.index_buffer.get());
    if (!vertex_buffer || !index_buffer)
    {
        throw std::runtime_error("D3D12Device: acceleration-structure geometry buffers must be D3D12 buffers");
    }
    if (geometry.vertex_count == 0 || geometry.index_count == 0 || geometry.index_count % 3 != 0)
    {
        throw std::runtime_error("D3D12Device: acceleration-structure geometry must contain indexed triangles");
    }

    D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
    desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    desc.Flags = geometry.opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    desc.Triangles.VertexBuffer.StartAddress = vertex_buffer->GetGPUVirtualAddress() + geometry.vertex_offset;
    desc.Triangles.VertexBuffer.StrideInBytes = geometry.vertex_stride;
    desc.Triangles.VertexCount = geometry.vertex_count;
    desc.Triangles.VertexFormat = ImageToDXGIFormat(geometry.vertex_format);
    desc.Triangles.IndexBuffer = index_buffer->GetGPUVirtualAddress() + geometry.index_offset;
    desc.Triangles.IndexCount = geometry.index_count;
    desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    return desc;
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

AccelerationStructurePtr D3D12Device::CreateAccelerationStructure(AccelerationStructureType type, uint64_t size,
    uint64_t build_scratch_size)
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
    return std::make_shared<D3D12AccelerationStructure>(*this,
        type,
        std::move(storage_buffer),
        build_scratch_size,
        gpu_address);
}

AccelerationStructurePtr
D3D12Device::CreateBottomLevelAccelerationStructure(std::vector<AccelerationStructureGeometryDesc> const& geometries)
{
    if (geometries.empty())
    {
        throw std::runtime_error("D3D12Device::CreateBottomLevelAccelerationStructure: geometry list is empty");
    }

    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> d3d12_geometries;
    d3d12_geometries.reserve(geometries.size());
    for (AccelerationStructureGeometryDesc const& geometry : geometries)
    {
        d3d12_geometries.push_back(ToD3D12GeometryDesc(geometry));
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.NumDescs = static_cast<UINT>(d3d12_geometries.size());
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.pGeometryDescs = d3d12_geometries.data();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    ComPtr<ID3D12Device5> rt_device;
    ThrowIfFailed(d3d12_device_.As(&rt_device));
    rt_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
    return CreateAccelerationStructure(AccelerationStructureType::kBottomLevel,
        info.ResultDataMaxSizeInBytes,
        info.ScratchDataSizeInBytes);
}

AccelerationStructurePtr D3D12Device::CreateTopLevelAccelerationStructure(uint32_t instance_count)
{
    if (instance_count == 0)
    {
        throw std::
            runtime_error("D3D12Device::CreateTopLevelAccelerationStructure: instance count must be greater than zero");
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.NumDescs = instance_count;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    ComPtr<ID3D12Device5> rt_device;
    ThrowIfFailed(d3d12_device_.As(&rt_device));
    rt_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
    return CreateAccelerationStructure(AccelerationStructureType::kTopLevel,
        info.ResultDataMaxSizeInBytes,
        info.ScratchDataSizeInBytes);
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
