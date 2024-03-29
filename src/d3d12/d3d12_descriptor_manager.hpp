#ifndef D3D12_DESCRIPTOR_MANAGER_HPP_
#define D3D12_DESCRIPTOR_MANAGER_HPP_

#include "d3d12_common.hpp"
#include <cstdint>

namespace gpu
{
    class D3D12Device;

    class D3D12DescriptorManager
    {
    public:
        D3D12DescriptorManager(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE desc_type);

        std::uint32_t AllocateDescriptor();
        void FreeDescriptor(std::uint32_t descriptor);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(std::uint32_t descriptor);
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(std::uint32_t descriptor);
        ID3D12DescriptorHeap* GetDescriptorHeap() const { return descriptor_heap_.Get(); }

    private:
        ComPtr<ID3D12DescriptorHeap> descriptor_heap_;
        std::uint32_t desc_size_;
        std::uint32_t num_allocated_descriptors_ = 0;

    };
}

#endif // D3D12_DESCRIPTOR_MANAGER_HPP_
