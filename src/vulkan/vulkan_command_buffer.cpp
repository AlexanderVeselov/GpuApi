#include "vulkan_command_buffer.hpp"

#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_pipeline.hpp"

namespace gpu
{
static VkImageLayout ToVkImageLayout(ImageLayout layout)
{
    switch (layout)
    {
    case ImageLayout::kUndefined: return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout::kPresent: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case ImageLayout::kCopySrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout::kCopyDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout::kRenderTarget: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::kShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::kShaderReadWrite: return VK_IMAGE_LAYOUT_GENERAL;
    default: return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device, VkCommandPool command_pool)
    : device_(device)
    , command_pool_(command_pool)
{
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool_;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkResult status =
        vkAllocateCommandBuffers(device_.GetDevice(), &allocate_info, &command_buffer_);
    VK_THROW_IF_FAILED(status, "Failed to allocate Vulkan command buffer");

    Begin();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    if (command_buffer_ != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device_.GetDevice(), command_pool_, 1, &command_buffer_);
    }
}

void VulkanCommandBuffer::Begin()
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult status = vkBeginCommandBuffer(command_buffer_, &begin_info);
    VK_THROW_IF_FAILED(status, "Failed to begin Vulkan command buffer");
}

void VulkanCommandBuffer::Close()
{
    if (closed_)
    {
        return;
    }

    VkResult status = vkEndCommandBuffer(command_buffer_);
    VK_THROW_IF_FAILED(status, "Failed to end Vulkan command buffer");

    closed_ = true;
}

void VulkanCommandBuffer::SetVertexBuffer(BufferPtr buffer, std::size_t)
{
    auto* vulkan_buffer = dynamic_cast<VulkanBuffer*>(buffer.get());
    THROW_IF(!vulkan_buffer, "Vertex buffer does not belong to the Vulkan backend");

    VkBuffer vertex_buffers[] = { vulkan_buffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, vertex_buffers, offsets);
}

void VulkanCommandBuffer::SetIndexBuffer(BufferPtr buffer)
{
    auto* vulkan_buffer = dynamic_cast<VulkanBuffer*>(buffer.get());
    THROW_IF(!vulkan_buffer, "Index buffer does not belong to the Vulkan backend");

    vkCmdBindIndexBuffer(command_buffer_, vulkan_buffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::Dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    vkCmdDispatch(command_buffer_, x, y, z);
}

void VulkanCommandBuffer::Draw(
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance)
{
    vkCmdDraw(command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
}

void VulkanCommandBuffer::DrawIndexed(
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance)
{
    vkCmdDrawIndexed(
        command_buffer_,
        index_count,
        instance_count,
        first_index,
        vertex_offset,
        first_instance);
}

void VulkanCommandBuffer::BindPipeline(GraphicsPipelinePtr const& pipeline)
{
    auto* vulkan_pipeline = dynamic_cast<VulkanGraphicsPipeline*>(pipeline.get());
    THROW_IF(!vulkan_pipeline, "Graphics pipeline does not belong to the Vulkan backend");

    vkCmdBindPipeline(
        command_buffer_,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan_pipeline->GetPipeline());
}

void VulkanCommandBuffer::BindPipeline(ComputePipelinePtr const& pipeline)
{
    auto* vulkan_pipeline = dynamic_cast<VulkanComputePipeline*>(pipeline.get());
    THROW_IF(!vulkan_pipeline, "Compute pipeline does not belong to the Vulkan backend");

    vkCmdBindPipeline(
        command_buffer_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        vulkan_pipeline->GetPipeline());
}

void VulkanCommandBuffer::BindDescriptorSet(DescriptorSetPtr const&)
{
    throw std::runtime_error("Vulkan descriptor sets are not implemented yet");
}

void VulkanCommandBuffer::SetRenderTarget(ImagePtr, ImagePtr)
{
    throw std::runtime_error("Vulkan render targets are not implemented yet");
}

void VulkanCommandBuffer::SetRenderTargets(std::vector<ImagePtr> const&, ImagePtr)
{
    throw std::runtime_error("Vulkan render targets are not implemented yet");
}

void VulkanCommandBuffer::SetViewport(const Viewport& viewport)
{
    VkViewport vk_viewport{};
    vk_viewport.x = viewport.x;
    vk_viewport.y = viewport.y;
    vk_viewport.width = viewport.width;
    vk_viewport.height = viewport.height;
    vk_viewport.minDepth = viewport.min_depth;
    vk_viewport.maxDepth = viewport.max_depth;

    vkCmdSetViewport(command_buffer_, 0, 1, &vk_viewport);
}

void VulkanCommandBuffer::SetScissor(const Rect& rect)
{
    VkRect2D vk_rect{};
    vk_rect.offset = { rect.x, rect.y };
    vk_rect.extent = { static_cast<uint32_t>(rect.width), static_cast<uint32_t>(rect.height) };

    vkCmdSetScissor(command_buffer_, 0, 1, &vk_rect);
}

void VulkanCommandBuffer::ClearImage(ImagePtr image, float r, float g, float b, float a)
{
    auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");

    VkClearColorValue color = { { r, g, b, a } };
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = vulkan_image->GetMipCount();
    range.baseArrayLayer = 0;
    range.layerCount = vulkan_image->GetArraySize();

    vkCmdClearColorImage(
        command_buffer_,
        vulkan_image->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &color,
        1,
        &range);
}

void VulkanCommandBuffer::TransitionBarrier(
    ImagePtr image,
    ImageLayout layout_before,
    ImageLayout layout_after)
{
    auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = ToVkImageLayout(layout_before);
    barrier.newLayout = ToVkImageLayout(layout_after);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vulkan_image->GetImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = vulkan_image->GetMipCount();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = vulkan_image->GetArraySize();

    vkCmdPipelineBarrier(
        command_buffer_,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);
}

void VulkanCommandBuffer::StorageBarrier(ImagePtr image)
{
    auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vulkan_image->GetImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = vulkan_image->GetMipCount();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = vulkan_image->GetArraySize();

    vkCmdPipelineBarrier(
        command_buffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);
}

void VulkanCommandBuffer::CopyBuffer(
    Buffer* src,
    uint64_t src_offset,
    Buffer* dst,
    uint64_t dst_offset,
    uint64_t size)
{
    auto* vulkan_src = dynamic_cast<VulkanBuffer*>(src);
    auto* vulkan_dst = dynamic_cast<VulkanBuffer*>(dst);
    THROW_IF(!vulkan_src || !vulkan_dst, "Buffer does not belong to the Vulkan backend");

    VkBufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer_, vulkan_src->GetBuffer(), vulkan_dst->GetBuffer(), 1, &copy_region);
}

void VulkanCommandBuffer::CopyBufferToImage(Image* dst, Buffer* src)
{
    auto* vulkan_dst = dynamic_cast<VulkanImage*>(dst);
    auto* vulkan_src = dynamic_cast<VulkanBuffer*>(src);
    THROW_IF(!vulkan_dst || !vulkan_src, "Resource does not belong to the Vulkan backend");

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { vulkan_dst->GetWidth(), vulkan_dst->GetHeight(), 1 };

    vkCmdCopyBufferToImage(
        command_buffer_,
        vulkan_src->GetBuffer(),
        vulkan_dst->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void VulkanCommandBuffer::CopyImage(Image* dst, Image* src)
{
    auto* vulkan_dst = dynamic_cast<VulkanImage*>(dst);
    auto* vulkan_src = dynamic_cast<VulkanImage*>(src);
    THROW_IF(!vulkan_dst || !vulkan_src, "Image does not belong to the Vulkan backend");

    VkImageCopy region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount = 1;
    region.extent = { vulkan_dst->GetWidth(), vulkan_dst->GetHeight(), 1 };

    vkCmdCopyImage(
        command_buffer_,
        vulkan_src->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vulkan_dst->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

} // namespace gpu
