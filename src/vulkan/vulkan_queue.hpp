#pragma once

#include "gpu_queue.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace gpu
{
class VulkanDevice;
class VulkanCommandBuffer;

class VulkanQueue final : public Queue
{
  public:
    VulkanQueue(VulkanDevice& device, uint32_t queue_family_index);
    ~VulkanQueue() override;

    CommandBufferPtr CreateCommandBuffer() override;
    void Submit(CommandBufferPtr cmd_buffer) override;
    void WaitIdle() override;

    VkQueue GetQueue() const
    {
        return queue_;
    }
    VkCommandPool GetCommandPool() const
    {
        return command_pool_;
    }

  private:
    VulkanDevice& device_;
    uint32_t queue_family_index_ = 0;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<CommandBufferPtr> in_flight_command_buffers_;
};

} // namespace gpu
