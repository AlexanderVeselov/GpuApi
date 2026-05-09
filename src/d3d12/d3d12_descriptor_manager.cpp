#include "d3d12_descriptor_manager.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

#include <cassert>

namespace gpu
{
D3D12DescriptorHeapAllocator::D3D12DescriptorHeapAllocator(
    D3D12Device& device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t capacity,
    bool shader_visible)
    : capacity_(capacity)
    , shader_visible_(shader_visible)
{
    auto d3d12_device = device.GetD3D12Device();

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = type;
    heap_desc.NumDescriptors = capacity;
    heap_desc.Flags = shader_visible_ ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(d3d12_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_)));

    cpu_start_ = heap_->GetCPUDescriptorHandleForHeapStart();
    if (shader_visible_)
    {
        gpu_start_ = heap_->GetGPUDescriptorHandleForHeapStart();
    }

    descriptor_size_ = d3d12_device->GetDescriptorHandleIncrementSize(type);

    free_list_.reserve(capacity_);
    for (uint32_t index = capacity_; index > 0; --index)
    {
        free_list_.push_back(index - 1);
    }

#ifndef NDEBUG
    allocated_.resize(capacity_);
#endif
}

uint32_t D3D12DescriptorHeapAllocator::Allocate()
{
    assert(!free_list_.empty() && "D3D12DescriptorHeapAllocator::Allocate: descriptor heap is full");

    const uint32_t index = free_list_.back();
    free_list_.pop_back();

#ifndef NDEBUG
    assert(!allocated_[index] && "D3D12DescriptorHeapAllocator::Allocate: descriptor is already allocated");
    allocated_[index] = true;
#endif
    return index;
}

void D3D12DescriptorHeapAllocator::Free(uint32_t index)
{
    assert(index < capacity_ && "D3D12DescriptorHeapAllocator::Free: descriptor index is out of range");
#ifndef NDEBUG
    assert(allocated_[index] && "D3D12DescriptorHeapAllocator::Free: descriptor was not allocated or was already freed");
    allocated_[index] = false;
#endif
    free_list_.push_back(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapAllocator::CPU(uint32_t index) const
{
    assert(index < capacity_ && "D3D12DescriptorHeapAllocator::CPU: descriptor index is out of range");
    D3D12_CPU_DESCRIPTOR_HANDLE handle = cpu_start_;
    handle.ptr += descriptor_size_ * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapAllocator::GPU(uint32_t index) const
{
    assert(shader_visible_ && "D3D12DescriptorHeapAllocator::GPU: heap is not shader visible");
    assert(index < capacity_ && "D3D12DescriptorHeapAllocator::GPU: descriptor index is out of range");
    D3D12_GPU_DESCRIPTOR_HANDLE handle = gpu_start_;
    handle.ptr += descriptor_size_ * index;
    return handle;
}

D3D12DescriptorManager::D3D12DescriptorManager(D3D12Device& device)
    : device_(device)
    , cpu_cbv_srv_uav_(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536, false)
    , cpu_rtv_(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 8192, false)
    , cpu_dsv_(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 4096, false)
    , cpu_sampler_(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048, false)
    , gpu_cbv_srv_uav_(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536, true)
    , gpu_sampler_(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048, true)
{
}

D3D12Descriptor D3D12DescriptorManager::AllocateCPUCBVSRVUAV()
{
    return { D3D12DescriptorHeapId::CPU_CBV_SRV_UAV, cpu_cbv_srv_uav_.Allocate() };
}

D3D12Descriptor D3D12DescriptorManager::AllocateCPURTV()
{
    return { D3D12DescriptorHeapId::CPU_RTV, cpu_rtv_.Allocate() };
}

D3D12Descriptor D3D12DescriptorManager::AllocateCPUDSV()
{
    return { D3D12DescriptorHeapId::CPU_DSV, cpu_dsv_.Allocate() };
}

D3D12Descriptor D3D12DescriptorManager::AllocateCPUSampler()
{
    return { D3D12DescriptorHeapId::CPU_SAMPLER, cpu_sampler_.Allocate() };
}

D3D12Descriptor D3D12DescriptorManager::AllocateGPUCBVSRVUAV()
{
    return { D3D12DescriptorHeapId::GPU_CBV_SRV_UAV, gpu_cbv_srv_uav_.Allocate() };
}

D3D12Descriptor D3D12DescriptorManager::AllocateGPUSampler()
{
    return { D3D12DescriptorHeapId::GPU_SAMPLER, gpu_sampler_.Allocate() };
}

void D3D12DescriptorManager::Free(D3D12Descriptor descriptor)
{
    if (!descriptor.IsValid())
    {
        return;
    }

    GetAllocator(descriptor.heap).Free(descriptor.index);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorManager::GetCPU(D3D12Descriptor descriptor) const
{
    assert(descriptor.IsValid() && "D3D12DescriptorManager::GetCPU: descriptor is invalid");
    return GetAllocator(descriptor.heap).CPU(descriptor.index);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorManager::GetGPU(D3D12Descriptor descriptor) const
{
    assert(descriptor.IsValid() && "D3D12DescriptorManager::GetGPU: descriptor is invalid");
    assert((descriptor.heap == D3D12DescriptorHeapId::GPU_CBV_SRV_UAV ||
        descriptor.heap == D3D12DescriptorHeapId::GPU_SAMPLER) &&
        "D3D12DescriptorManager::GetGPU: descriptor must come from a GPU-visible heap");

    return GetAllocator(descriptor.heap).GPU(descriptor.index);
}

D3D12Descriptor D3D12DescriptorManager::CopyToGPUCBVSRVUAV(D3D12Descriptor src)
{
    assert(src.IsValid() && "D3D12DescriptorManager::CopyToGPUCBVSRVUAV: source descriptor is invalid");
    assert(src.heap == D3D12DescriptorHeapId::CPU_CBV_SRV_UAV &&
        "D3D12DescriptorManager::CopyToGPUCBVSRVUAV: source descriptor must come from CPU_CBV_SRV_UAV heap");

    D3D12Descriptor dst = AllocateGPUCBVSRVUAV();

    device_.GetD3D12Device()->CopyDescriptorsSimple(
        1,
        GetCPU(dst),
        GetCPU(src),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return dst;
}

D3D12Descriptor D3D12DescriptorManager::CopyToGPUSampler(D3D12Descriptor src)
{
    assert(src.IsValid() && "D3D12DescriptorManager::CopyToGPUSampler: source descriptor is invalid");
    assert(src.heap == D3D12DescriptorHeapId::CPU_SAMPLER &&
        "D3D12DescriptorManager::CopyToGPUSampler: source descriptor must come from CPU_SAMPLER heap");

    D3D12Descriptor dst = AllocateGPUSampler();

    device_.GetD3D12Device()->CopyDescriptorsSimple(
        1,
        GetCPU(dst),
        GetCPU(src),
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    return dst;
}

ID3D12DescriptorHeap* D3D12DescriptorManager::GetGPUCBVSRVUAVHeap() const
{
    return gpu_cbv_srv_uav_.Heap();
}

ID3D12DescriptorHeap* D3D12DescriptorManager::GetGPUSamplerHeap() const
{
    return gpu_sampler_.Heap();
}

D3D12DescriptorHeapAllocator& D3D12DescriptorManager::GetAllocator(D3D12DescriptorHeapId heap)
{
    switch (heap)
    {
    case D3D12DescriptorHeapId::CPU_CBV_SRV_UAV:
        return cpu_cbv_srv_uav_;
    case D3D12DescriptorHeapId::CPU_RTV:
        return cpu_rtv_;
    case D3D12DescriptorHeapId::CPU_DSV:
        return cpu_dsv_;
    case D3D12DescriptorHeapId::CPU_SAMPLER:
        return cpu_sampler_;
    case D3D12DescriptorHeapId::GPU_CBV_SRV_UAV:
        return gpu_cbv_srv_uav_;
    case D3D12DescriptorHeapId::GPU_SAMPLER:
        return gpu_sampler_;
    }

    assert(false && "D3D12DescriptorManager::GetAllocator: unknown descriptor heap id");
    return cpu_cbv_srv_uav_;
}

D3D12DescriptorHeapAllocator const& D3D12DescriptorManager::GetAllocator(D3D12DescriptorHeapId heap) const
{
    switch (heap)
    {
    case D3D12DescriptorHeapId::CPU_CBV_SRV_UAV:
        return cpu_cbv_srv_uav_;
    case D3D12DescriptorHeapId::CPU_RTV:
        return cpu_rtv_;
    case D3D12DescriptorHeapId::CPU_DSV:
        return cpu_dsv_;
    case D3D12DescriptorHeapId::CPU_SAMPLER:
        return cpu_sampler_;
    case D3D12DescriptorHeapId::GPU_CBV_SRV_UAV:
        return gpu_cbv_srv_uav_;
    case D3D12DescriptorHeapId::GPU_SAMPLER:
        return gpu_sampler_;
    }

    assert(false && "D3D12DescriptorManager::GetAllocator const: unknown descriptor heap id");
    return cpu_cbv_srv_uav_;
}

}
