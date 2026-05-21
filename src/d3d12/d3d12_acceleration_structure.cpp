#include "d3d12_acceleration_structure.hpp"

namespace gpu
{

D3D12AccelerationStructure::D3D12AccelerationStructure(AccelerationStructureType type, BufferPtr storage_buffer,
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address)
    : AccelerationStructure(type, std::move(storage_buffer)), gpu_address_(gpu_address)
{
}

}  // namespace gpu
