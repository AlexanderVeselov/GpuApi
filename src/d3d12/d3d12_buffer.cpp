#include "d3d12_buffer.hpp"
#include "d3d12_descriptor_set.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

#include <algorithm>
#include <cassert>

namespace gpu
{
namespace
{
uint64_t AlignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

D3D12_HEAP_TYPE ToHeapType(BufferFlags flags)
{
    if (HasFlag(flags, BufferFlags::kCpuAccess))
    {
        return D3D12_HEAP_TYPE_UPLOAD;
    }

    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(BufferFlags flags)
{
    D3D12_RESOURCE_FLAGS d3d12_flags = D3D12_RESOURCE_FLAG_NONE;

    if (HasFlag(flags, BufferFlags::kStorage) || HasFlag(flags, BufferFlags::kAccelerationStructureStorage))
    {
        d3d12_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return d3d12_flags;
}

D3D12_RESOURCE_STATES GetInitialState(BufferFlags flags)
{
    if (HasFlag(flags, BufferFlags::kCpuAccess))
    {
        return D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    if (HasFlag(flags, BufferFlags::kAccelerationStructureStorage))
    {
        return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    }

    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_RESOURCE_DESC CreateBufferDesc(uint64_t size, BufferFlags flags)
{
    if (HasFlag(flags, BufferFlags::kConstant))
    {
        size = AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = {1, 0};
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = ToD3D12ResourceFlags(flags);
    return desc;
}
}  // namespace

D3D12Buffer::D3D12Buffer(D3D12Device& device, uint64_t size, uint32_t stride)
    : D3D12Buffer(device, size, stride, BufferFlags::kCpuAccess)
{
}

D3D12Buffer::D3D12Buffer(D3D12Device& device, uint64_t size, uint32_t stride, BufferFlags flags)
    : Buffer(size), device_(device), flags_(flags), current_state_(GetInitialState(flags)), stride_(stride)
{
    assert(size_ > 0 && "D3D12Buffer: size must be greater than zero");
    assert(stride_ > 0 && "D3D12Buffer: stride must be greater than zero");
    assert(size_ % stride_ == 0 && "D3D12Buffer: size must be divisible by stride");
    CreateResource();
}

D3D12Buffer::~D3D12Buffer()
{
    device_.WaitIdle();

    for (D3D12DescriptorSet* descriptor_set : descriptor_sets_)
    {
        if (descriptor_set)
        {
            descriptor_set->OnBufferDestroyed(*this);
        }
    }
    descriptor_sets_.clear();

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    descriptor_manager.Free(cbv_);
    descriptor_manager.Free(srv_);
    descriptor_manager.Free(uav_);
}

D3D12Descriptor const& D3D12Buffer::GetCBV()
{
    if (!cbv_.IsValid())
    {
        cbv_ = CreateCBV();
    }

    return cbv_;
}

D3D12Descriptor const& D3D12Buffer::GetSRV()
{
    if (!srv_.IsValid())
    {
        srv_ = CreateSRV();
    }

    return srv_;
}

D3D12Descriptor const& D3D12Buffer::GetUAV()
{
    if (!uav_.IsValid())
    {
        uav_ = CreateUAV();
    }

    return uav_;
}

void D3D12Buffer::Resize(uint64_t new_size)
{
    assert(new_size > 0 && "D3D12Buffer::Resize: size must be greater than zero");
    assert(new_size % stride_ == 0 && "D3D12Buffer::Resize: size must be divisible by stride");
    assert(!mapped_ && "D3D12Buffer::Resize: buffer must not be resized while mapped");
    if (new_size == size_)
    {
        return;
    }

    device_.WaitIdle();
    size_ = new_size;
    current_state_ = GetInitialState(flags_);
    resource_.Reset();
    CreateResource();

    if (cbv_.IsValid())
    {
        WriteCBV(cbv_);
    }
    if (srv_.IsValid())
    {
        WriteSRV(srv_);
    }
    if (uav_.IsValid())
    {
        WriteUAV(uav_);
    }

    for (D3D12DescriptorSet* descriptor_set : descriptor_sets_)
    {
        if (descriptor_set)
        {
            descriptor_set->OnBufferResized(*this);
        }
    }
}

void* D3D12Buffer::Map()
{
    assert(HasFlag(flags_, BufferFlags::kCpuAccess) && "D3D12Buffer::Map: buffer was not created with CPU access");
    assert(!mapped_ && "D3D12Buffer::Map: buffer is already mapped");

    D3D12_RANGE read_range = {};
    void* data = nullptr;
    ThrowIfFailed(resource_->Map(0, &read_range, &data));
    mapped_ = true;
    return data;
}

void D3D12Buffer::Unmap()
{
    assert(mapped_ && "D3D12Buffer::Unmap: buffer is not mapped");

    resource_->Unmap(0, nullptr);
    mapped_ = false;
}

D3D12Descriptor D3D12Buffer::CreateCBV()
{
    assert(HasFlag(flags_, BufferFlags::kConstant)
        && "D3D12Buffer::CreateCBV: buffer was not created with constant-buffer support");

    D3D12Descriptor descriptor = device_.GetDescriptorManager().AllocateCPUCBVSRVUAV();
    WriteCBV(descriptor);
    return descriptor;
}

D3D12Descriptor D3D12Buffer::CreateSRV()
{
    assert(HasFlag(flags_, BufferFlags::kShaderResource)
        && "D3D12Buffer::CreateSRV: buffer was not created with shader-resource support");

    D3D12Descriptor descriptor = device_.GetDescriptorManager().AllocateCPUCBVSRVUAV();
    WriteSRV(descriptor);
    return descriptor;
}

D3D12Descriptor D3D12Buffer::CreateUAV()
{
    assert(HasFlag(flags_, BufferFlags::kStorage)
        && "D3D12Buffer::CreateUAV: buffer was not created with storage/UAV support");

    D3D12Descriptor descriptor = device_.GetDescriptorManager().AllocateCPUCBVSRVUAV();
    WriteUAV(descriptor);
    return descriptor;
}

void D3D12Buffer::RegisterDescriptorSet(D3D12DescriptorSet& descriptor_set)
{
    if (std::find(descriptor_sets_.begin(), descriptor_sets_.end(), &descriptor_set) == descriptor_sets_.end())
    {
        descriptor_sets_.push_back(&descriptor_set);
    }
}

void D3D12Buffer::UnregisterDescriptorSet(D3D12DescriptorSet& descriptor_set)
{
    descriptor_sets_.erase(std::remove(descriptor_sets_.begin(), descriptor_sets_.end(), &descriptor_set), descriptor_sets_.end());
}

void D3D12Buffer::CreateResource()
{
    auto d3d12_device = device_.GetD3D12Device();

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = ToHeapType(flags_);
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resource_desc = CreateBufferDesc(size_, flags_);

    ThrowIfFailed(d3d12_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
        current_state_, nullptr, IID_PPV_ARGS(&resource_)));
}

void D3D12Buffer::WriteCBV(D3D12Descriptor descriptor)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
    cbv_desc.BufferLocation = resource_->GetGPUVirtualAddress();
    cbv_desc.SizeInBytes = static_cast<UINT>(AlignUp(size_, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    device_.GetD3D12Device()->CreateConstantBufferView(&cbv_desc, device_.GetDescriptorManager().GetCPU(descriptor));
}

void D3D12Buffer::WriteSRV(D3D12Descriptor descriptor)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = static_cast<UINT>(size_ / stride_);
    srv_desc.Buffer.StructureByteStride = stride_;
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    device_.GetD3D12Device()->CreateShaderResourceView(resource_.Get(), &srv_desc,
        device_.GetDescriptorManager().GetCPU(descriptor));
}

void D3D12Buffer::WriteUAV(D3D12Descriptor descriptor)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = static_cast<UINT>(size_ / stride_);
    uav_desc.Buffer.StructureByteStride = stride_;
    uav_desc.Buffer.CounterOffsetInBytes = 0;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    device_.GetD3D12Device()->CreateUnorderedAccessView(resource_.Get(), nullptr, &uav_desc,
        device_.GetDescriptorManager().GetCPU(descriptor));
}

}  // namespace gpu
