#pragma once

#include "gpu_sampler.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanDevice;

class VulkanSampler final : public Sampler
{
public:
    VulkanSampler(VulkanDevice& device, SamplerDesc const& desc);
    ~VulkanSampler() override;

    VkSampler GetSampler() const { return sampler_; }

private:
    VulkanDevice& device_;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}  // namespace gpu
