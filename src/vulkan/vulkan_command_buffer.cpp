#include "vulkan_exception.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_buffer.hpp"

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice & device, VkCommandPool command_pool)
    : device_(device)
    , command_pool_(command_pool)
{
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool_;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkDevice logical_device = device_.GetDevice();
    VkResult status = vkAllocateCommandBuffers(logical_device, &allocate_info, &command_buffer_);
    VK_THROW_IF_FAILED(status, "Failed to allocate command buffer!");

}

void VulkanCommandBuffer::Begin()
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    VkResult status = vkBeginCommandBuffer(command_buffer_, &begin_info);
    VK_THROW_IF_FAILED(status, "Failed to begin command buffer!");
}

void VulkanCommandBuffer::Draw(std::uint32_t vertex_count, std::uint32_t first_vertex, std::uint32_t instance_count, std::uint32_t first_instance)
{
    vkCmdDraw(command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
}

void VulkanCommandBuffer::End()
{
    VkResult status = vkEndCommandBuffer(command_buffer_);
    VK_THROW_IF_FAILED(status, "Failed to end command buffer!");
}

void VulkanCommandBuffer::BeginGraphics(std::shared_ptr<VulkanGraphicsPipeline> pipeline)
{
    THROW_IF(current_pipeline_, "Graphics is already started!");

    current_pipeline_ = pipeline;

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = current_pipeline_->GetRenderPass();
    render_pass_begin_info.framebuffer = current_pipeline_->GetFramebuffer();
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = pipeline->GetExtent();

    VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

    auto const& descriptor_sets = pipeline->GetDescriptorSets();
    std::vector<VkDescriptorSet> vk_descriptor_sets;
    for (auto const& ds : descriptor_sets)
    {
        vk_descriptor_sets.push_back(ds.second.vk_descriptor_set);
    }

    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(),
        0, static_cast<std::uint32_t>(vk_descriptor_sets.size()), vk_descriptor_sets.data(), 0, nullptr);
}

void VulkanCommandBuffer::EndGraphics()
{
    THROW_IF(!current_pipeline_, "Graphics is not started!");
    vkCmdEndRenderPass(command_buffer_);
}

void VulkanCommandBuffer::BindVertexBuffer(std::shared_ptr<VulkanBuffer> buffer)
{
    VkBuffer vertex_buffers[] = { buffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, vertex_buffers, offsets);
}