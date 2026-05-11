#pragma once

#include "d3d12_common.hpp"
#include <cstdint>
#include <vector>

namespace gpu
{
class D3D12Device;

enum class D3D12DescriptorHeapId
{
    CPU_CBV_SRV_UAV,
    CPU_RTV,
    CPU_DSV,
    CPU_SAMPLER,

    GPU_CBV_SRV_UAV,
    GPU_SAMPLER
};

struct D3D12Descriptor
{
    D3D12DescriptorHeapId heap = D3D12DescriptorHeapId::CPU_CBV_SRV_UAV;
    uint32_t index = UINT32_MAX;

    bool IsValid() const
    {
        return index != UINT32_MAX;
    }
};

class D3D12DescriptorHeapAllocator
{
  public:
    D3D12DescriptorHeapAllocator(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t capacity, bool shader_visible);

    uint32_t Allocate();
    void Free(uint32_t index);

    D3D12_CPU_DESCRIPTOR_HANDLE CPU(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GPU(uint32_t index) const;

    bool IsShaderVisible() const
    {
        return shader_visible_;
    }
    ID3D12DescriptorHeap* Heap() const
    {
        return heap_.Get();
    }

  private:
    ComPtr<ID3D12DescriptorHeap> heap_;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_start_{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_start_{};

    uint32_t descriptor_size_ = 0;
    uint32_t capacity_ = 0;
    bool shader_visible_ = false;

    std::vector<uint32_t> free_list_;

#ifndef NDEBUG
    std::vector<bool> allocated_;
#endif
};

class D3D12DescriptorManager
{
  public:
    explicit D3D12DescriptorManager(D3D12Device& device);

    D3D12Descriptor AllocateCPUCBVSRVUAV();
    D3D12Descriptor AllocateCPURTV();
    D3D12Descriptor AllocateCPUDSV();
    D3D12Descriptor AllocateCPUSampler();

    D3D12Descriptor AllocateGPUCBVSRVUAV();
    D3D12Descriptor AllocateGPUSampler();

    void Free(D3D12Descriptor descriptor);

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(D3D12Descriptor descriptor) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(D3D12Descriptor descriptor) const;

    D3D12Descriptor CopyToGPUCBVSRVUAV(D3D12Descriptor src);
    D3D12Descriptor CopyToGPUSampler(D3D12Descriptor src);

    ID3D12DescriptorHeap* GetGPUCBVSRVUAVHeap() const;
    ID3D12DescriptorHeap* GetGPUSamplerHeap() const;

  private:
    D3D12DescriptorHeapAllocator& GetAllocator(D3D12DescriptorHeapId heap);
    D3D12DescriptorHeapAllocator const& GetAllocator(D3D12DescriptorHeapId heap) const;

  private:
    D3D12Device& device_;

    D3D12DescriptorHeapAllocator cpu_cbv_srv_uav_;
    D3D12DescriptorHeapAllocator cpu_rtv_;
    D3D12DescriptorHeapAllocator cpu_dsv_;
    D3D12DescriptorHeapAllocator cpu_sampler_;

    D3D12DescriptorHeapAllocator gpu_cbv_srv_uav_;
    D3D12DescriptorHeapAllocator gpu_sampler_;
};
} // namespace gpu
