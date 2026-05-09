#pragma once

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include "d3d12_pipeline_layout.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_image.hpp"

#include <cstdint>
#include <vector>

namespace gpu
{
class D3D12Buffer;
class D3D12Image;

class D3D12DescriptorSet : public DescriptorSet
{
public:
    D3D12DescriptorSet(D3D12DescriptorManager& descriptor_manager, D3D12PipelineLayout const& layout);
    ~D3D12DescriptorSet();

    D3D12DescriptorSet(D3D12DescriptorSet const&) = delete;
    D3D12DescriptorSet& operator=(D3D12DescriptorSet const&) = delete;

    D3D12DescriptorSet(D3D12DescriptorSet&& other) noexcept;
    D3D12DescriptorSet& operator=(D3D12DescriptorSet&& other) noexcept;

    void BindBuffer(Buffer& buffer, std::uint32_t binding, std::uint32_t space) override;
    void BindImage(Image& image, std::uint32_t binding, std::uint32_t space) override;
    void BindImage(Image& image, ImageView const& view, std::uint32_t binding, std::uint32_t space) override;

    void BindBuffer(D3D12Buffer& buffer, std::uint32_t binding, std::uint32_t space);
    void BindImage(D3D12Image& image, std::uint32_t binding, std::uint32_t space);
    void BindImage(D3D12Image& image, ImageView const& view, std::uint32_t binding, std::uint32_t space);

    bool HasRootTable(uint32_t root_parameter_index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetRootTable(uint32_t root_parameter_index) const;

    void Clear() override;

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
    D3D12PipelineLayout const& layout_;
    std::vector<BoundDescriptor> descriptors_;
};

} // namespace gpu
