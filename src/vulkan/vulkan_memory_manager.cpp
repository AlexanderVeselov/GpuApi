#include "vulkan_memory_manager.hpp"
#include "vulkan_device.hpp"

VulkanMemoryManager::VulkanMemoryManager(VulkanDevice & device)
    : device_(device)
{

}

uint32_t VulkanMemoryManager::FindMemoryTypeIndex(uint32_t memory_type_bits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device_.GetPhysicalDevice(), &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((memory_type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkDeviceMemory VulkanMemoryManager::AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags flags)
{
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = requirements.size;
    memory_allocate_info.memoryTypeIndex = FindMemoryTypeIndex(requirements.memoryTypeBits, flags);

    VkDeviceMemory memory;
    vkAllocateMemory(device_.GetDevice(), &memory_allocate_info, nullptr, &memory);

    return memory;
}
