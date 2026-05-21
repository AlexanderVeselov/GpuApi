#pragma once

#include "gpu_api.hpp"
#include "vulkan_shader_manager.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanApi final : public Api
{
public:
    VulkanApi();
    ~VulkanApi() override;

    DevicePtr CreateDevice() override;
    void SetShaderPath(char const* shader_path) override;

    VkInstance GetInstance() const { return instance_; }
    VulkanShaderManager& GetShaderManager() { return shader_manager_; }

private:
    void CreateInstance();
    void SetupDebugMessenger();
    VkPhysicalDevice ChoosePhysicalDevice() const;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VulkanShaderManager shader_manager_;
};

}  // namespace gpu
