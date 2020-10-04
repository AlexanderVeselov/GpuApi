#include "d3d12_descriptor_manager.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

#include <cassert>

namespace gpu
{
    namespace
    {
        constexpr std::uint32_t kMaxDescriptors = 1024u;
    }

    D3D12DescriptorManager::D3D12DescriptorManager(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE desc_type)
    {
        auto d3d12_device = device.GetD3D12Device();

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.Type = desc_type;
        heap_desc.NumDescriptors = kMaxDescriptors;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(d3d12_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&descriptor_heap_)));

        // Get descriptor handle size
        desc_size_ = d3d12_device->GetDescriptorHandleIncrementSize(desc_type);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorManager::AllocateDescriptor()
    {
        // TODO: this is the simplest way
        assert(num_allocated_descriptors_ < kMaxDescriptors);
        D3D12_CPU_DESCRIPTOR_HANDLE result = descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
        result.ptr += desc_size_ * (num_allocated_descriptors_++);
        return result;
    }

    void D3D12DescriptorManager::FreeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE /*handle*/)
    {
        // Do nothing for now
    }

}