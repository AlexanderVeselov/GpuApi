#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace gpu
{
class VulkanDevice;

class VulkanMemoryManager
{
public:
    explicit VulkanMemoryManager(VulkanDevice& device);

    VkDeviceMemory AllocateMemory(
        VkMemoryRequirements requirements,
        VkMemoryPropertyFlags properties);

private:
    uint32_t FindMemoryTypeIndex(
        uint32_t memory_type_bits,
        VkMemoryPropertyFlags properties) const;

private:
    VulkanDevice& device_;
};

} // namespace gpu
