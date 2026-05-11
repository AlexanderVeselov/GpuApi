#include "vulkan_memory_manager.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

namespace gpu
{
VulkanMemoryManager::VulkanMemoryManager(VulkanDevice& device) : device_(device)
{
}

uint32_t VulkanMemoryManager::FindMemoryTypeIndex(
    uint32_t memory_type_bits, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(device_.GetPhysicalDevice(), &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        bool supported = (memory_type_bits & (1u << i)) != 0;
        bool has_properties =
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties;

        if (supported && has_properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable Vulkan memory type");
}

VkDeviceMemory VulkanMemoryManager::AllocateMemory(
    VkMemoryRequirements requirements, VkMemoryPropertyFlags properties)
{
    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = requirements.size;
    memory_allocate_info.memoryTypeIndex =
        FindMemoryTypeIndex(requirements.memoryTypeBits, properties);

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkResult status =
        vkAllocateMemory(device_.GetDevice(), &memory_allocate_info, nullptr, &memory);
    VK_THROW_IF_FAILED(status, "Failed to allocate Vulkan memory");

    return memory;
}

} // namespace gpu
