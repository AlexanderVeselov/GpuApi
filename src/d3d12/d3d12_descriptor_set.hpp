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
class D3D12Sampler;

class D3D12DescriptorSet : public DescriptorSet
{
public:
    struct BoundDescriptor
    {
        uint32_t binding = 0;
        uint32_t space = 0;
        uint32_t root_parameter_index = 0;

        D3D12Descriptor cpu_descriptor;
    };

    explicit D3D12DescriptorSet(D3D12PipelineLayout const& layout);
    ~D3D12DescriptorSet();

    D3D12DescriptorSet(D3D12DescriptorSet const&) = delete;
    D3D12DescriptorSet& operator=(D3D12DescriptorSet const&) = delete;

    D3D12DescriptorSet(D3D12DescriptorSet&& other) noexcept;
    D3D12DescriptorSet& operator=(D3D12DescriptorSet&& other) noexcept;

    void BindBuffer(Buffer& buffer, uint32_t binding, uint32_t space) override;
    void BindImage(Image& image, uint32_t binding, uint32_t space) override;
    void BindImage(Image& image, ImageView const& view, uint32_t binding, uint32_t space) override;
    void BindSampler(Sampler& sampler, uint32_t binding, uint32_t space) override;

    void BindBuffer(D3D12Buffer& buffer, uint32_t binding, uint32_t space);
    void BindImage(D3D12Image& image, uint32_t binding, uint32_t space);
    void BindImage(D3D12Image& image, ImageView const& view, uint32_t binding, uint32_t space);
    void BindSampler(D3D12Sampler& sampler, uint32_t binding, uint32_t space);

    D3D12PipelineLayout const& GetLayout() const { return layout_; }
    std::vector<BoundDescriptor> const& GetBoundDescriptors() const { return descriptors_; }

    void Clear() override;

private:
    D3D12Binding const& FindBinding(uint32_t binding, uint32_t space) const;
    BoundDescriptor& FindOrCreateBoundDescriptor(D3D12Binding const& binding);
    void BindDescriptor(D3D12Binding const& binding, D3D12Descriptor cpu_descriptor);

private:
    D3D12PipelineLayout const& layout_;
    std::vector<BoundDescriptor> descriptors_;
};

}  // namespace gpu
