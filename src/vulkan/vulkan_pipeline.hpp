#pragma once

#include "gpu_pipeline.hpp"
#include "vulkan_pipeline_layout.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanDevice;

class VulkanPipeline : virtual public Pipeline
{
public:
    explicit VulkanPipeline(VulkanDevice& device);
    ~VulkanPipeline() override;
    DescriptorSetPtr CreateDescriptorSet() override;
    VkPipeline GetPipeline() const { return pipeline_; }
    VkPipelineLayout GetPipelineLayout() const { return layout_.GetPipelineLayout(); }
    VulkanPipelineLayout const& GetLayout() const { return layout_; }
    std::vector<VulkanBinding> const& GetBindings() const { return layout_.GetBindings(); }

protected:
    void DestroyPipeline();

protected:
    VulkanDevice& device_;
    VulkanPipelineLayout layout_;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

class VulkanGraphicsPipeline final : public GraphicsPipeline, public VulkanPipeline
{
public:
    VulkanGraphicsPipeline(VulkanDevice& device, GraphicsPipelineDesc const& pipeline_desc);
    uint32_t GetVertexStride() const { return vertex_stride_; }
    void Reload() override;

private:
    uint32_t vertex_stride_ = 0;
};

class VulkanComputePipeline final : public ComputePipeline, public VulkanPipeline
{
public:
    VulkanComputePipeline(VulkanDevice& device, char const* cs_filename);
    void Reload() override;
};

} // namespace gpu
