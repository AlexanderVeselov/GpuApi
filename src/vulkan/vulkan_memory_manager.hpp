#ifndef VULKAN_MEMORY_MANAGER_HPP_
#define VULKAN_MEMORY_MANAGER_HPP_

#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanMemoryManager
{
public:
    VulkanMemoryManager(VulkanDevice & device_);
    VkDeviceMemory AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags flags);

private:
    uint32_t FindMemoryTypeIndex(uint32_t memory_type_bits, VkMemoryPropertyFlags properties);

    VulkanDevice & device_;

};

#endif // VULKAN_MEMORY_MANAGER_HPP_
