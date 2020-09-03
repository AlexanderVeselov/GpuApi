#ifndef VULKAN_COMMAND_BUFFER_HPP_
#define VULKAN_COMMAND_BUFFER_HPP_

#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>

class VulkanGraphicsPipeline;
class VulkanBuffer;

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer(VulkanDevice & device, VkCommandPool command_pool);
    void Begin();
    void BeginGraphics(std::shared_ptr<VulkanGraphicsPipeline> pipeline);
    void EndGraphics();
    void Draw(std::uint32_t vertex_count, std::uint32_t first_vertex = 0, std::uint32_t instance_count = 1, std::uint32_t first_instance = 0);
    void End();
    void BindVertexBuffer(std::shared_ptr<VulkanBuffer> buffer);
    VkCommandBuffer GetCommandBuffer() const { return command_buffer_; }
    
private:
    VulkanDevice & device_;

    VkCommandPool command_pool_;
    VkCommandBuffer command_buffer_;

    std::shared_ptr<VulkanGraphicsPipeline> current_pipeline_;

};

#endif // VULKAN_COMMAND_BUFFER_HPP_
