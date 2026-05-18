#pragma once

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include "gpu_sampler.hpp"

namespace gpu
{
class D3D12Device;

class D3D12Sampler final : public Sampler
{
public:
    D3D12Sampler(D3D12Device& device, SamplerDesc const& desc);
    ~D3D12Sampler() override;

    D3D12Descriptor GetDescriptor() const { return descriptor_; }

private:
    D3D12Device& device_;
    D3D12Descriptor descriptor_;
};

}  // namespace gpu
