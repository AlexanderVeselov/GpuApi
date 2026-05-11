#pragma once

#include "gpu_descriptor_set.hpp"
#include "vulkan_pipeline_layout.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace gpu
{
class VulkanDevice;

class VulkanDescriptorSet final : public DescriptorSet
{
  public:
    explicit VulkanDescriptorSet(VulkanDevice& device, VulkanPipelineLayout const& layout);
    ~VulkanDescriptorSet() override;

    VulkanDescriptorSet(VulkanDescriptorSet const&) = delete;
    VulkanDescriptorSet& operator=(VulkanDescriptorSet const&) = delete;

    void BindBuffer(Buffer& buffer, uint32_t binding, uint32_t space) override;
    void BindImage(Image& image, uint32_t binding, uint32_t space) override;
    void BindImage(Image& image, ImageView const& view, uint32_t binding, uint32_t space) override;

    VulkanPipelineLayout const& GetLayout() const
    {
        return layout_;
    }

    std::vector<VkDescriptorSet> const& GetDescriptorSets() const
    {
        return descriptor_sets_;
    }

    void Clear() override;

  private:
    VulkanBinding const& FindBinding(uint32_t binding, uint32_t space) const;
    void CreateDescriptorPool();
    void AllocateDescriptorSets();

  private:
    VulkanDevice& device_;
    VulkanPipelineLayout const& layout_;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;
};

} // namespace gpu
