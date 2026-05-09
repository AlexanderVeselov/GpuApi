#pragma once

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include "gpu_image.hpp"

#include <cstdint>
#include <vector>

namespace gpu
{
class D3D12Buffer;
class D3D12Device;
class D3D12Image;
class D3D12Pipeline;
struct D3D12Binding;

class D3D12DescriptorSet
{
public:
    D3D12DescriptorSet(D3D12Device& device, D3D12Pipeline const& pipeline);
    ~D3D12DescriptorSet();

    D3D12DescriptorSet(D3D12DescriptorSet const&) = delete;
    D3D12DescriptorSet& operator=(D3D12DescriptorSet const&) = delete;

    D3D12DescriptorSet(D3D12DescriptorSet&& other) noexcept;
    D3D12DescriptorSet& operator=(D3D12DescriptorSet&& other) noexcept;

    void BindBuffer(D3D12Buffer& buffer, uint32_t binding, uint32_t space);
    void BindImage(D3D12Image& image, uint32_t binding, uint32_t space);
    void BindImage(D3D12Image& image, ImageView const& view, uint32_t binding, uint32_t space);

    bool HasRootTable(uint32_t root_parameter_index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetRootTable(uint32_t root_parameter_index) const;

    void Clear();

private:
    struct BoundDescriptor
    {
        uint32_t binding = 0;
        uint32_t space = 0;
        uint32_t root_parameter_index = 0;

        D3D12Descriptor cpu_descriptor;
        D3D12Descriptor gpu_descriptor;
    };

    D3D12Binding const& FindBinding(uint32_t binding, uint32_t space) const;
    BoundDescriptor& FindOrCreateBoundDescriptor(D3D12Binding const& binding);
    void BindDescriptor(D3D12Binding const& binding, D3D12Descriptor cpu_descriptor);

private:
    D3D12DescriptorManager& descriptor_manager_;
    D3D12Pipeline const& pipeline_;
    std::vector<BoundDescriptor> descriptors_;
};

} // namespace gpu
