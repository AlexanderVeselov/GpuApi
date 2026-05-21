#include "vulkan_command_buffer.hpp"

#include "vulkan_acceleration_structure.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptor_set.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_pipeline.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace gpu
{
static bool IsDepthImage(Image const& image)
{
    return HasFlag(image.GetFlags(), ImageFlags::kDepthStencil) || image.GetFormat() == ImageFormat::kD32_Float
        || image.GetFormat() == ImageFormat::kR32_Typeless;
}

static VkImageLayout ToVkImageLayout(ImageLayout layout, bool is_depth)
{
    switch (layout)
    {
    case ImageLayout::kUndefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout::kPresent:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case ImageLayout::kCopySrc:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout::kCopyDst:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout::kRenderTarget:
        return is_depth ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::kShaderRead:
        return is_depth ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::kShaderReadWrite:
        return VK_IMAGE_LAYOUT_GENERAL;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

static VkImageAspectFlags GetImageAspect(Image const& image)
{
    return IsDepthImage(image) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
}

static VkAccelerationStructureGeometryKHR ToVkGeometryDesc(AccelerationStructureGeometryDesc const& geometry)
{
    if (!geometry.vertex_buffer || !geometry.index_buffer)
    {
        throw std::runtime_error(
            "VulkanCommandBuffer::BuildBottomLevelAccelerationStructure: geometry buffers must not be null");
    }

    VkAccelerationStructureGeometryKHR desc{};
    desc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    desc.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    desc.flags = geometry.opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;
    desc.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    desc.geometry.triangles.vertexFormat = ToVkFormat(geometry.vertex_format);
    desc.geometry.triangles.vertexData.deviceAddress = geometry.vertex_buffer->GetGpuAddress() + geometry.vertex_offset;
    desc.geometry.triangles.vertexStride = geometry.vertex_stride;
    desc.geometry.triangles.maxVertex = geometry.vertex_count - 1;
    desc.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    desc.geometry.triangles.indexData.deviceAddress = geometry.index_buffer->GetGpuAddress() + geometry.index_offset;
    return desc;
}

static VkAccelerationStructureGeometryKHR CreateTopLevelGeometryDesc(Buffer& instance_buffer)
{
    VkAccelerationStructureGeometryKHR desc{};
    desc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    desc.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    desc.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    desc.geometry.instances.arrayOfPointers = VK_FALSE;
    desc.geometry.instances.data.deviceAddress = instance_buffer.GetGpuAddress();
    return desc;
}

static VkAccelerationStructureInstanceKHR ToVkInstanceDesc(AccelerationStructureInstanceDesc const& instance)
{
    auto* bottom_level = dynamic_cast<VulkanAccelerationStructure*>(instance.bottom_level);
    if (!bottom_level || bottom_level->GetType() != AccelerationStructureType::kBottomLevel)
    {
        throw std::runtime_error("VulkanCommandBuffer::BuildTopLevelAccelerationStructure: instance BLAS is invalid");
    }

    VkAccelerationStructureInstanceKHR desc{};
    std::memcpy(desc.transform.matrix, instance.transform, sizeof(desc.transform.matrix));
    desc.instanceCustomIndex = instance.instance_id;
    desc.mask = instance.instance_mask;
    desc.instanceShaderBindingTableRecordOffset = 0;
    desc.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    desc.accelerationStructureReference = bottom_level->GetDeviceAddress();
    return desc;
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device, VkCommandPool command_pool)
    : device_(device), command_pool_(command_pool)
{
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool_;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkResult status = vkAllocateCommandBuffers(device_.GetDevice(), &allocate_info, &command_buffer_);
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

    EndRendering();

    VkResult status = vkEndCommandBuffer(command_buffer_);
    VK_THROW_IF_FAILED(status, "Failed to end Vulkan command buffer");

    closed_ = true;
}

void VulkanCommandBuffer::EndRendering()
{
    if (!rendering_active_)
    {
        return;
    }

    vkCmdEndRendering(command_buffer_);
    rendering_active_ = false;
}

void VulkanCommandBuffer::SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride)
{
    auto* vulkan_buffer = dynamic_cast<VulkanBuffer*>(buffer.get());
    THROW_IF(!vulkan_buffer, "Vertex buffer does not belong to the Vulkan backend");
    current_vertex_stride_ = static_cast<uint32_t>(vertex_stride);
    THROW_IF(current_graphics_pipeline_ && current_graphics_pipeline_->GetVertexStride() != 0
            && current_graphics_pipeline_->GetVertexStride() != current_vertex_stride_,
        "Vertex buffer stride does not match the current Vulkan graphics pipeline input layout");

    VkBuffer vertex_buffers[] = {vulkan_buffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
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
    EndRendering();

    vkCmdDispatch(command_buffer_, x, y, z);
}

void VulkanCommandBuffer::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
    uint32_t first_instance)
{
    THROW_IF(!rendering_active_, "Draw requires an active Vulkan render target");

    vkCmdDraw(command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index,
    int32_t vertex_offset, uint32_t first_instance)
{
    THROW_IF(!rendering_active_, "DrawIndexed requires an active Vulkan render target");

    vkCmdDrawIndexed(command_buffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void VulkanCommandBuffer::BindPipeline(GraphicsPipelinePtr const& pipeline)
{
    auto* vulkan_pipeline = dynamic_cast<VulkanGraphicsPipeline*>(pipeline.get());
    THROW_IF(!vulkan_pipeline, "Graphics pipeline does not belong to the Vulkan backend");
    THROW_IF(current_vertex_stride_ != 0 && vulkan_pipeline->GetVertexStride() != 0
            && vulkan_pipeline->GetVertexStride() != current_vertex_stride_,
        "Current vertex buffer stride does not match the Vulkan graphics pipeline input layout");

    current_graphics_pipeline_ = vulkan_pipeline;
    current_pipeline_bind_point_ = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_pipeline->GetPipeline());
}

void VulkanCommandBuffer::BindPipeline(ComputePipelinePtr const& pipeline)
{
    auto* vulkan_pipeline = dynamic_cast<VulkanComputePipeline*>(pipeline.get());
    THROW_IF(!vulkan_pipeline, "Compute pipeline does not belong to the Vulkan backend");

    current_compute_pipeline_ = vulkan_pipeline;
    current_pipeline_bind_point_ = VK_PIPELINE_BIND_POINT_COMPUTE;
    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan_pipeline->GetPipeline());
}

void VulkanCommandBuffer::BindDescriptorSet(DescriptorSetPtr const& descriptor_set)
{
    auto* vulkan_descriptor_set = dynamic_cast<VulkanDescriptorSet*>(descriptor_set.get());
    THROW_IF(!vulkan_descriptor_set, "Descriptor set does not belong to the Vulkan backend");

    std::vector<VkDescriptorSet> const& descriptor_sets = vulkan_descriptor_set->GetDescriptorSets();
    if (descriptor_sets.empty())
    {
        return;
    }

    if (current_pipeline_bind_point_ == VK_PIPELINE_BIND_POINT_GRAPHICS)
    {
        THROW_IF(!current_graphics_pipeline_, "No Vulkan graphics pipeline is currently bound");
        THROW_IF(&vulkan_descriptor_set->GetLayout() != &current_graphics_pipeline_->GetLayout(),
            "Descriptor set layout does not match the current Vulkan graphics pipeline layout");

        vkCmdBindDescriptorSets(command_buffer_,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            current_graphics_pipeline_->GetPipelineLayout(),
            0,
            static_cast<uint32_t>(descriptor_sets.size()),
            descriptor_sets.data(),
            0,
            nullptr);
    }

    if (current_pipeline_bind_point_ == VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        THROW_IF(!current_compute_pipeline_, "No Vulkan compute pipeline is currently bound");
        THROW_IF(&vulkan_descriptor_set->GetLayout() != &current_compute_pipeline_->GetLayout(),
            "Descriptor set layout does not match the current Vulkan compute pipeline layout");

        vkCmdBindDescriptorSets(command_buffer_,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            current_compute_pipeline_->GetPipelineLayout(),
            0,
            static_cast<uint32_t>(descriptor_sets.size()),
            descriptor_sets.data(),
            0,
            nullptr);
    }

    THROW_IF(current_pipeline_bind_point_ == VK_PIPELINE_BIND_POINT_MAX_ENUM,
        "A Vulkan pipeline must be bound before binding descriptor sets");
}

void VulkanCommandBuffer::SetRenderTarget(ImagePtr color_attachment, ImagePtr depth_attachment)
{
    std::vector<ImagePtr> color_attachments;
    if (color_attachment)
    {
        color_attachments.push_back(color_attachment);
    }

    SetRenderTargets(color_attachments, depth_attachment);
}

void VulkanCommandBuffer::SetRenderTargets(std::vector<ImagePtr> const& color_attachments, ImagePtr depth_attachment)
{
    EndRendering();

    THROW_IF(color_attachments.empty() && !depth_attachment, "SetRenderTargets requires at least one attachment");

    uint32_t render_width = 0;
    uint32_t render_height = 0;
    std::vector<VkRenderingAttachmentInfo> color_attachment_infos;
    color_attachment_infos.reserve(color_attachments.size());

    for (ImagePtr const& attachment : color_attachments)
    {
        auto* vulkan_image = dynamic_cast<VulkanImage*>(attachment.get());
        THROW_IF(!vulkan_image, "Color attachment does not belong to the Vulkan backend");
        THROW_IF(IsDepthImage(*attachment), "Color attachment must not be a depth image");

        if (render_width == 0 && render_height == 0)
        {
            render_width = attachment->GetWidth();
            render_height = attachment->GetHeight();
        }
        else
        {
            THROW_IF(attachment->GetWidth() != render_width || attachment->GetHeight() != render_height,
                "All Vulkan render targets must have the same size");
        }

        VkRenderingAttachmentInfo attachment_info = {};
        attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment_info.imageView = vulkan_image->GetImageView();
        attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_infos.push_back(attachment_info);
    }

    VkRenderingAttachmentInfo depth_attachment_info = {};
    if (depth_attachment)
    {
        auto* vulkan_depth = dynamic_cast<VulkanImage*>(depth_attachment.get());
        THROW_IF(!vulkan_depth, "Depth attachment does not belong to the Vulkan backend");
        THROW_IF(!IsDepthImage(*depth_attachment), "Depth attachment must be a depth image");

        if (render_width == 0 && render_height == 0)
        {
            render_width = depth_attachment->GetWidth();
            render_height = depth_attachment->GetHeight();
        }
        else
        {
            THROW_IF(depth_attachment->GetWidth() != render_width || depth_attachment->GetHeight() != render_height,
                "All Vulkan render targets must have the same size");
        }

        depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment_info.imageView = vulkan_depth->GetImageView();
        depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.offset = {0, 0};
    rendering_info.renderArea.extent = {render_width, render_height};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_attachment_infos.size());
    rendering_info.pColorAttachments = color_attachment_infos.empty() ? nullptr : color_attachment_infos.data();
    rendering_info.pDepthAttachment = depth_attachment ? &depth_attachment_info : nullptr;

    vkCmdBeginRendering(command_buffer_, &rendering_info);
    rendering_active_ = true;
}

void VulkanCommandBuffer::SetViewport(const Viewport& viewport)
{
    VkViewport vk_viewport{};
    vk_viewport.x = viewport.x;
    vk_viewport.y = viewport.y + viewport.height;
    vk_viewport.width = viewport.width;
    vk_viewport.height = -viewport.height;
    vk_viewport.minDepth = viewport.min_depth;
    vk_viewport.maxDepth = viewport.max_depth;

    vkCmdSetViewport(command_buffer_, 0, 1, &vk_viewport);
}

void VulkanCommandBuffer::SetScissor(const Rect& rect)
{
    VkRect2D vk_rect{};
    vk_rect.offset = {rect.x, rect.y};
    vk_rect.extent = {static_cast<uint32_t>(rect.width), static_cast<uint32_t>(rect.height)};

    vkCmdSetScissor(command_buffer_, 0, 1, &vk_rect);
}

void VulkanCommandBuffer::ClearImage(ImagePtr image, float r, float g, float b, float a)
{
    EndRendering();

    auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");
    THROW_IF(IsDepthImage(*image), "ClearImage only supports color images in the Vulkan backend");

    VkClearColorValue color = {{r, g, b, a}};
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = vulkan_image->GetMipCount();
    range.baseArrayLayer = 0;
    range.layerCount = vulkan_image->GetArraySize();

    vkCmdClearColorImage(command_buffer_,
        vulkan_image->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &color,
        1,
        &range);
}

void VulkanCommandBuffer::ClearDepthImage(ImagePtr image, float depth)
{
    EndRendering();

    auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");
    THROW_IF(!IsDepthImage(*image), "ClearDepthImage only supports depth images in the Vulkan backend");

    VkClearDepthStencilValue clear_value = {};
    clear_value.depth = depth;
    clear_value.stencil = 0;

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    range.baseMipLevel = 0;
    range.levelCount = vulkan_image->GetMipCount();
    range.baseArrayLayer = 0;
    range.layerCount = vulkan_image->GetArraySize();

    vkCmdClearDepthStencilImage(command_buffer_,
        vulkan_image->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &clear_value,
        1,
        &range);
}

void VulkanCommandBuffer::TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after)
{
    TransitionBarrier(std::vector<ImagePtr>{image}, layout_before, layout_after);
}

void VulkanCommandBuffer::TransitionBarrier(std::vector<ImagePtr> const& images, ImageLayout layout_before,
    ImageLayout layout_after)
{
    EndRendering();

    std::vector<VkImageMemoryBarrier> barriers;
    barriers.reserve(images.size());

    for (ImagePtr const& image : images)
    {
        auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
        THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bool is_depth = IsDepthImage(*image);
        barrier.oldLayout = ToVkImageLayout(layout_before, is_depth);
        barrier.newLayout = ToVkImageLayout(layout_after, is_depth);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkan_image->GetImage();
        barrier.subresourceRange.aspectMask = GetImageAspect(*image);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = vulkan_image->GetMipCount();
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = vulkan_image->GetArraySize();

        barriers.push_back(barrier);
    }

    if (!barriers.empty())
    {
        vkCmdPipelineBarrier(command_buffer_,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            static_cast<uint32_t>(barriers.size()),
            barriers.data());
    }
}

void VulkanCommandBuffer::StorageBarrier(ImagePtr image)
{
    EndRendering();

    auto* vulkan_image = dynamic_cast<VulkanImage*>(image.get());
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vulkan_image->GetImage();
    barrier.subresourceRange.aspectMask = GetImageAspect(*image);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = vulkan_image->GetMipCount();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = vulkan_image->GetArraySize();

    vkCmdPipelineBarrier(command_buffer_,
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

void VulkanCommandBuffer::StorageBarrier(BufferPtr buffer)
{
    EndRendering();

    auto* vulkan_buffer = dynamic_cast<VulkanBuffer*>(buffer.get());
    THROW_IF(!vulkan_buffer, "Buffer does not belong to the Vulkan backend");

    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT
        | VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = vulkan_buffer->GetBuffer();
    barrier.offset = 0;
    barrier.size = buffer->GetSize();

    VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    if (HasFlag(vulkan_buffer->GetFlags(), BufferFlags::kAccelerationStructureBuildInput))
    {
        barrier.dstAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        dst_stage_mask |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    }

    vkCmdPipelineBarrier(command_buffer_, src_stage_mask, dst_stage_mask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void VulkanCommandBuffer::AccelerationStructureBarrier(VulkanAccelerationStructure const& acceleration_structure)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    auto* storage_buffer = dynamic_cast<VulkanBuffer*>(acceleration_structure.GetStorageBuffer().get());
    THROW_IF(!storage_buffer, "acceleration structure storage buffer does not belong to the Vulkan backend");
    barrier.buffer = storage_buffer->GetBuffer();
    barrier.offset = 0;
    barrier.size = acceleration_structure.GetStorageBuffer()->GetSize();

    vkCmdPipelineBarrier(command_buffer_,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0,
        nullptr,
        1,
        &barrier,
        0,
        nullptr);
}

void VulkanCommandBuffer::BuildBottomLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
    std::vector<AccelerationStructureGeometryDesc> const& geometries)
{
    EndRendering();

    auto* bottom_level = dynamic_cast<VulkanAccelerationStructure*>(&acceleration_structure);
    THROW_IF(!bottom_level || bottom_level->GetType() != AccelerationStructureType::kBottomLevel,
        "acceleration structure is not a Vulkan BLAS");
    THROW_IF(geometries.empty(), "geometry list is empty");

    BufferPtr scratch_buffer = device_.CreateBuffer(bottom_level->GetBuildScratchSize(),
        1,
        BufferFlags::kStorage | BufferFlags::kAccelerationStructureBuildInput);
    auto* scratch = dynamic_cast<VulkanBuffer*>(scratch_buffer.get());
    staging_buffers_.push_back(scratch_buffer);

    std::vector<VkAccelerationStructureGeometryKHR> vk_geometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR const*> range_ptrs;
    vk_geometries.reserve(geometries.size());
    ranges.reserve(geometries.size());
    range_ptrs.reserve(geometries.size());

    for (AccelerationStructureGeometryDesc const& geometry : geometries)
    {
        vk_geometries.push_back(ToVkGeometryDesc(geometry));

        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = geometry.index_count / 3;
        range.primitiveOffset = 0;
        range.firstVertex = 0;
        range.transformOffset = 0;
        ranges.push_back(range);
    }

    for (VkAccelerationStructureBuildRangeInfoKHR const& range : ranges)
    {
        range_ptrs.push_back(&range);
    }

    for (AccelerationStructureGeometryDesc const& geometry : geometries)
    {
        StorageBarrier(geometry.vertex_buffer);
        StorageBarrier(geometry.index_buffer);
    }

    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure = bottom_level->GetHandle();
    build_info.geometryCount = static_cast<uint32_t>(vk_geometries.size());
    build_info.pGeometries = vk_geometries.data();
    build_info.scratchData.deviceAddress = scratch->GetGpuAddress();

    auto vk_cmd_build = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device_
                                                                                                          .GetDevice(),
        "vkCmdBuildAccelerationStructuresKHR"));
    THROW_IF(!vk_cmd_build, "vkCmdBuildAccelerationStructuresKHR is unavailable");

    vk_cmd_build(command_buffer_, 1, &build_info, range_ptrs.data());
    AccelerationStructureBarrier(*bottom_level);
}

void VulkanCommandBuffer::BuildTopLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
    std::vector<AccelerationStructureInstanceDesc> const& instances)
{
    EndRendering();

    auto* top_level = dynamic_cast<VulkanAccelerationStructure*>(&acceleration_structure);
    THROW_IF(!top_level || top_level->GetType() != AccelerationStructureType::kTopLevel,
        "acceleration structure is not a Vulkan TLAS");
    THROW_IF(instances.empty(), "instance list is empty");

    BufferPtr scratch_buffer = device_.CreateBuffer(top_level->GetBuildScratchSize(),
        1,
        BufferFlags::kStorage | BufferFlags::kAccelerationStructureBuildInput);
    auto* scratch = dynamic_cast<VulkanBuffer*>(scratch_buffer.get());
    staging_buffers_.push_back(scratch_buffer);

    std::vector<VkAccelerationStructureInstanceKHR> vk_instances;
    vk_instances.reserve(instances.size());
    for (AccelerationStructureInstanceDesc const& instance : instances)
    {
        vk_instances.push_back(ToVkInstanceDesc(instance));
    }

    uint64_t instance_data_size = vk_instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    BufferPtr upload_buffer = device_.CreateBuffer(instance_data_size, 1, BufferFlags::kCpuAccess);
    std::memcpy(upload_buffer->Map(), vk_instances.data(), instance_data_size);
    upload_buffer->Unmap();

    BufferPtr instance_buffer = device_.CreateBuffer(instance_data_size,
        1,
        BufferFlags::kAccelerationStructureBuildInput);
    CopyBuffer(upload_buffer, 0, instance_buffer, 0, instance_data_size);
    StorageBarrier(instance_buffer);
    staging_buffers_.push_back(upload_buffer);
    staging_buffers_.push_back(instance_buffer);

    VkAccelerationStructureGeometryKHR geometry = CreateTopLevelGeometryDesc(*instance_buffer);
    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = static_cast<uint32_t>(vk_instances.size());
    range.primitiveOffset = 0;
    range.firstVertex = 0;
    range.transformOffset = 0;
    VkAccelerationStructureBuildRangeInfoKHR const* range_ptr = &range;

    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure = top_level->GetHandle();
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;
    build_info.scratchData.deviceAddress = scratch->GetGpuAddress();

    auto vk_cmd_build = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device_
                                                                                                          .GetDevice(),
        "vkCmdBuildAccelerationStructuresKHR"));
    THROW_IF(!vk_cmd_build, "vkCmdBuildAccelerationStructuresKHR is unavailable");

    vk_cmd_build(command_buffer_, 1, &build_info, &range_ptr);
    AccelerationStructureBarrier(*top_level);
}

void VulkanCommandBuffer::CopyBuffer(BufferPtr src, uint64_t src_offset, BufferPtr dst, uint64_t dst_offset,
    uint64_t size)
{
    EndRendering();

    auto* vulkan_src = dynamic_cast<VulkanBuffer*>(src.get());
    auto* vulkan_dst = dynamic_cast<VulkanBuffer*>(dst.get());
    THROW_IF(!vulkan_src || !vulkan_dst, "Buffer does not belong to the Vulkan backend");

    VkBufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer_, vulkan_src->GetBuffer(), vulkan_dst->GetBuffer(), 1, &copy_region);
}

void VulkanCommandBuffer::CopyBufferToImage(ImagePtr dst, BufferPtr src)
{
    EndRendering();

    auto* vulkan_dst = dynamic_cast<VulkanImage*>(dst.get());
    auto* vulkan_src = dynamic_cast<VulkanBuffer*>(src.get());
    THROW_IF(!vulkan_dst || !vulkan_src, "Resource does not belong to the Vulkan backend");

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = GetImageAspect(*dst);
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {vulkan_dst->GetWidth(), vulkan_dst->GetHeight(), 1};

    vkCmdCopyBufferToImage(command_buffer_,
        vulkan_src->GetBuffer(),
        vulkan_dst->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void VulkanCommandBuffer::UploadImage(ImagePtr dst, void const* data, size_t data_size)
{
    EndRendering();

    auto* vulkan_dst = dynamic_cast<VulkanImage*>(dst.get());
    THROW_IF(!vulkan_dst, "Image does not belong to the Vulkan backend");
    THROW_IF(!data || data_size == 0, "Image upload data is empty");

    BufferPtr staging_buffer = device_.CreateBuffer(data_size, 1, BufferFlags::kCpuAccess);
    void* mapped_data = staging_buffer->Map();
    std::memcpy(mapped_data, data, data_size);
    staging_buffer->Unmap();

    auto* vulkan_src = static_cast<VulkanBuffer*>(staging_buffer.get());

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = GetImageAspect(*dst);
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {vulkan_dst->GetWidth(), vulkan_dst->GetHeight(), 1};

    vkCmdCopyBufferToImage(command_buffer_,
        vulkan_src->GetBuffer(),
        vulkan_dst->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
    staging_buffers_.push_back(std::move(staging_buffer));
}

void VulkanCommandBuffer::CopyImage(ImagePtr dst, ImagePtr src)
{
    EndRendering();

    auto* vulkan_dst = dynamic_cast<VulkanImage*>(dst.get());
    auto* vulkan_src = dynamic_cast<VulkanImage*>(src.get());
    THROW_IF(!vulkan_dst || !vulkan_src, "Image does not belong to the Vulkan backend");

    VkImageCopy region{};
    region.srcSubresource.aspectMask = GetImageAspect(*src);
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = GetImageAspect(*dst);
    region.dstSubresource.layerCount = 1;
    region.extent = {vulkan_dst->GetWidth(), vulkan_dst->GetHeight(), 1};

    vkCmdCopyImage(command_buffer_,
        vulkan_src->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vulkan_dst->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

}  // namespace gpu
