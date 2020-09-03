#ifndef VULKAN_BUFFER_HPP_
#define VULKAN_BUFFER_HPP_

#include "vulkan_shared_object.hpp"
#include "vulkan_device.hpp"

class VulkanBuffer
{
public:
    VulkanBuffer(VulkanDevice & device, VkDeviceSize size, VkBufferUsageFlags usage);
    void Write(void const* data);
    VkBuffer GetBuffer() const { return buffer_; }

private:
    VulkanDevice & device_;
    VkDeviceSize buffer_size_;
    VulkanScopedObject<VkBuffer, vkCreateBuffer, vkDestroyBuffer> buffer_;
    VulkanScopedObject<VkDeviceMemory, nullptr, vkFreeMemory> memory_;

};

#endif // VULKAN_BUFFER_HPP_
