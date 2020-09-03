#include "vulkan_buffer.hpp"
#include "vulkan_exception.hpp"

VulkanBuffer::VulkanBuffer(VulkanDevice & device, VkDeviceSize size, VkBufferUsageFlags usage)
    : device_(device)
    , buffer_size_(size)
{
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice logical_device = device_.GetDevice();
    VkResult status = buffer_.Create(logical_device, buffer_create_info);
    VK_THROW_IF_FAILED(status, "Failed to create buffer!");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(logical_device, buffer_, &memory_requirements);

    // TODO: add other buffer types support
    memory_.Attach(logical_device, device_.GetMemoryManager().AllocateMemory(memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
    status = vkBindBufferMemory(logical_device, buffer_, memory_, 0);
    VK_THROW_IF_FAILED(status, "Find to bind buffer memory!");

}

void VulkanBuffer::Write(void const* data)
{
    void* mapped_data;
    vkMapMemory(device_.GetDevice(), memory_, 0, buffer_size_, 0, &mapped_data);
    memcpy(mapped_data, data, buffer_size_);
    vkUnmapMemory(device_.GetDevice(), memory_);
}
