#include "d3d12_acceleration_structure.hpp"

#include "d3d12_device.hpp"

#include <utility>

namespace gpu
{

D3D12AccelerationStructure::D3D12AccelerationStructure(D3D12Device& device, AccelerationStructureType type,
    BufferPtr storage_buffer, uint64_t build_scratch_size, D3D12_GPU_VIRTUAL_ADDRESS gpu_address)
    : AccelerationStructure(type, std::move(storage_buffer), build_scratch_size)
    , device_(device)
    , gpu_address_(gpu_address)
{
}

D3D12AccelerationStructure::~D3D12AccelerationStructure()
{
    device_.WaitIdle();
    device_.GetDescriptorManager().Free(srv_);
}

D3D12Descriptor const& D3D12AccelerationStructure::GetSRV()
{
    if (!srv_.IsValid())
    {
        srv_ = CreateSRV();
    }

    return srv_;
}

D3D12Descriptor D3D12AccelerationStructure::CreateSRV()
{
    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    D3D12Descriptor descriptor = descriptor_manager.AllocateCPUCBVSRVUAV();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.RaytracingAccelerationStructure.Location = gpu_address_;

    device_.GetD3D12Device()->CreateShaderResourceView(nullptr, &srv_desc, descriptor_manager.GetCPU(descriptor));

    return descriptor;
}

}  // namespace gpu
