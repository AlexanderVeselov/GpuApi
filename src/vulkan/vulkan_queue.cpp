#include "vulkan_queue.hpp"

#include "vulkan_command_buffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

namespace gpu
{
VulkanQueue::VulkanQueue(VulkanDevice& device, uint32_t queue_family_index)
    : device_(device)
    , queue_family_index_(queue_family_index)
{
    vkGetDeviceQueue(device_.GetDevice(), queue_family_index_, 0, &queue_);

    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index_;

    VkResult status =
        vkCreateCommandPool(device_.GetDevice(), &command_pool_info, nullptr, &command_pool_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan command pool");
}

VulkanQueue::~VulkanQueue()
{
    WaitIdle();

    if (command_pool_ != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device_.GetDevice(), command_pool_, nullptr);
    }
}

CommandBufferPtr VulkanQueue::CreateCommandBuffer()
{
    return std::make_unique<VulkanCommandBuffer>(device_, command_pool_);
}

void VulkanQueue::Submit(CommandBufferPtr cmd_buffer)
{
    auto* vulkan_command_buffer = dynamic_cast<VulkanCommandBuffer*>(cmd_buffer.get());
    THROW_IF(!vulkan_command_buffer, "Command buffer does not belong to the Vulkan backend");

    vulkan_command_buffer->Close();

    VkCommandBuffer vk_command_buffer = vulkan_command_buffer->GetCommandBuffer();

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_command_buffer;

    VkResult status = vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);
    VK_THROW_IF_FAILED(status, "Failed to submit Vulkan command buffer");

    in_flight_command_buffers_.push_back(std::move(cmd_buffer));
}

void VulkanQueue::WaitIdle()
{
    if (queue_ != VK_NULL_HANDLE)
    {
        VkResult status = vkQueueWaitIdle(queue_);
        VK_THROW_IF_FAILED(status, "Failed to wait for Vulkan queue idle");
    }

    in_flight_command_buffers_.clear();
}

} // namespace gpu
