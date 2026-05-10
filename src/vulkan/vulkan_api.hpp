#pragma once

#include "gpu_api.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanApi final : public Api
{
public:
    VulkanApi();
    ~VulkanApi() override;

    DevicePtr CreateDevice() override;

    VkInstance GetInstance() const { return instance_; }

private:
    void CreateInstance();
    VkPhysicalDevice ChoosePhysicalDevice() const;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
};

} // namespace gpu
