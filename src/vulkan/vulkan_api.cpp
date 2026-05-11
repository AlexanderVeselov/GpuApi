#include "vulkan_api.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

#include <vector>

namespace gpu
{
VulkanApi::VulkanApi()
    : shader_manager_("")
{
    CreateInstance();
}

VulkanApi::~VulkanApi()
{
    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
    }
}

void VulkanApi::CreateInstance()
{
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "GpuApi";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "GpuApi";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    std::vector<char const*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkResult status = vkCreateInstance(&create_info, nullptr, &instance_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan instance");
}

VkPhysicalDevice VulkanApi::ChoosePhysicalDevice() const
{
    uint32_t physical_device_count = 0;
    VkResult status = vkEnumeratePhysicalDevices(instance_, &physical_device_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan physical devices");
    THROW_IF(physical_device_count == 0, "No Vulkan physical devices are available");

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    status = vkEnumeratePhysicalDevices(instance_, &physical_device_count, physical_devices.data());
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan physical devices");

    VkPhysicalDevice best_device = physical_devices[0];
    uint64_t best_score = 0;

    for (VkPhysicalDevice physical_device : physical_devices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical_device, &properties);

        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        uint64_t dedicated_memory = 0;
        for (uint32_t i = 0; i < memory_properties.memoryHeapCount; ++i)
        {
            if ((memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
            {
                dedicated_memory += memory_properties.memoryHeaps[i].size;
            }
        }

        uint64_t score = dedicated_memory;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1ull << 62;
        }

        if (score > best_score)
        {
            best_score = score;
            best_device = physical_device;
        }
    }

    return best_device;
}

DevicePtr VulkanApi::CreateDevice()
{
    return std::make_unique<VulkanDevice>(*this, ChoosePhysicalDevice());
}

} // namespace gpu
