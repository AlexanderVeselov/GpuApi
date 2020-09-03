#ifndef VULKAN_API_HPP_
#define VULKAN_API_HPP_

#include "gpu_api.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

namespace gpu
{
    class VulkanApi : public Api
    {
    public:
        VulkanApi(std::vector<char const*> const& enabled_extensions, bool enable_validation);
        ~VulkanApi();

        VulkanSharedObject<VkInstance> GetInstance() const;
        // Create device with presentation support
        std::shared_ptr<VulkanDevice> CreateDevice(std::vector<char const*> const& enabled_extensions, std::uint32_t physical_device_index, std::function<VkSurfaceKHR(VkInstance)> surface_creation_callback);

    private:
        void CreateInstance(std::vector<char const*> enabled_extensions);
        void CreateDebugMessenger();

    private:
        bool validation_enabled_;
        VulkanSharedObject<VkInstance> instance_;
        VkDebugUtilsMessengerEXT debug_messenger_ = nullptr;

    };

} // namespace gpu

#endif // VULKAN_API_HPP_
