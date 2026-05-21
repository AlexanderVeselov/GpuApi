#pragma once

#include "gpu_acceleration_structure.hpp"

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"

namespace gpu
{

class D3D12Device;

class D3D12AccelerationStructure final : public AccelerationStructure
{
public:
    D3D12AccelerationStructure(D3D12Device& device, AccelerationStructureType type, BufferPtr storage_buffer,
        uint64_t build_scratch_size, D3D12_GPU_VIRTUAL_ADDRESS gpu_address);
    ~D3D12AccelerationStructure() override;

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return gpu_address_; }
    D3D12Descriptor const& GetSRV();

private:
    D3D12Descriptor CreateSRV();

private:
    D3D12Device& device_;
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address_ = 0;
    D3D12Descriptor srv_;
};

}  // namespace gpu
