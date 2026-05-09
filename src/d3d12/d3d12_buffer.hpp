#pragma once

#include "gpu_buffer.hpp"
#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"

#include <cstdint>

namespace gpu
{
class D3D12Device;

class D3D12Buffer final : public Buffer
{
public:
    D3D12Buffer(D3D12Device& device, uint64_t size, uint32_t stride);
    D3D12Buffer(
        D3D12Device& device,
        uint64_t size,
        uint32_t stride,
        BufferFlags flags);
    ~D3D12Buffer() override;

    ID3D12Resource* GetResource() const
    {
        return resource_.Get();
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return resource_->GetGPUVirtualAddress();
    }

    D3D12Descriptor const& GetCBV();
    D3D12Descriptor const& GetSRV();
    D3D12Descriptor const& GetUAV();
    uint32_t GetStride() const { return stride_; }

    D3D12_RESOURCE_STATES GetCurrentState() const
    {
        return current_state_;
    }

    void SetCurrentState(D3D12_RESOURCE_STATES state)
    {
        current_state_ = state;
    }

    void* Map() override;
    void Unmap() override;

private:
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
};

} // namespace gpu
