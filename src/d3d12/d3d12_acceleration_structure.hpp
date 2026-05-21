#pragma once

#include "gpu_acceleration_structure.hpp"

#include "d3d12_common.hpp"

namespace gpu
{

class D3D12AccelerationStructure final : public AccelerationStructure
{
public:
    D3D12AccelerationStructure(AccelerationStructureType type, BufferPtr storage_buffer,
        D3D12_GPU_VIRTUAL_ADDRESS gpu_address);

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return gpu_address_; }

private:
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address_ = 0;
};

}  // namespace gpu
