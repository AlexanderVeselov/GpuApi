#pragma once

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include "gpu_buffer.hpp"

#include <cstdint>
#include <vector>

namespace gpu
{
class D3D12Device;
class D3D12DescriptorSet;

class D3D12Buffer final : public Buffer
{
public:
    D3D12Buffer(D3D12Device& device, uint64_t size, uint32_t stride);
    D3D12Buffer(D3D12Device& device, uint64_t size, uint32_t stride, BufferFlags flags);
    ~D3D12Buffer() override;

    ID3D12Resource* GetResource() const { return resource_.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return resource_->GetGPUVirtualAddress(); }
    uint64_t GetGpuAddress() const override { return GetGPUVirtualAddress(); }

    D3D12Descriptor const& GetCBV();
    D3D12Descriptor const& GetSRV();
    D3D12Descriptor const& GetUAV();
    uint32_t GetStride() const { return stride_; }

    D3D12_RESOURCE_STATES GetCurrentState() const { return current_state_; }

    void SetCurrentState(D3D12_RESOURCE_STATES state) { current_state_ = state; }

    void Resize(uint64_t new_size) override;
    void* Map() override;
    void Unmap() override;

    void RegisterDescriptorSet(D3D12DescriptorSet& descriptor_set);
    void UnregisterDescriptorSet(D3D12DescriptorSet& descriptor_set);

private:
    void CreateResource();
    void WriteCBV(D3D12Descriptor descriptor);
    void WriteSRV(D3D12Descriptor descriptor);
    void WriteUAV(D3D12Descriptor descriptor);
    D3D12Descriptor CreateCBV();
    D3D12Descriptor CreateSRV();
    D3D12Descriptor CreateUAV();

private:
    D3D12Device& device_;
    ComPtr<ID3D12Resource> resource_;

    BufferFlags flags_ = BufferFlags::kNone;
    D3D12_RESOURCE_STATES current_state_ = D3D12_RESOURCE_STATE_COMMON;
    uint32_t stride_ = 0;
    bool mapped_ = false;

    D3D12Descriptor cbv_;
    D3D12Descriptor srv_;
    D3D12Descriptor uav_;
    std::vector<D3D12DescriptorSet*> descriptor_sets_;
};

}  // namespace gpu
