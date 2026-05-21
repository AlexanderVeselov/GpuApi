#pragma once

#include "gpu_command_buffer.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanDevice;
class VulkanGraphicsPipeline;
class VulkanComputePipeline;

class VulkanCommandBuffer final : public CommandBuffer
{
public:
    VulkanCommandBuffer(VulkanDevice& device, VkCommandPool command_pool);
    ~VulkanCommandBuffer() override;

    VkCommandBuffer GetCommandBuffer() const { return command_buffer_; }
    void Close();

    void SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride) override;
    void SetIndexBuffer(BufferPtr buffer) override;

    void Dispatch(std::uint32_t num_groups_x, std::uint32_t num_groups_y, std::uint32_t num_groups_z) override;

    void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override;

    void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset,
        uint32_t first_instance) override;

    void BindPipeline(GraphicsPipelinePtr const& pipeline) override;
    void BindPipeline(ComputePipelinePtr const& pipeline) override;
    void BindDescriptorSet(DescriptorSetPtr const& descriptor_set) override;

    void SetRenderTarget(ImagePtr color_attachment, ImagePtr depth_attachment) override;
    void SetRenderTargets(std::vector<ImagePtr> const& color_attachments, ImagePtr depth_attachment) override;

    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Rect& rect) override;

    void ClearImage(ImagePtr image, float r, float g, float b, float a) override;
    void ClearDepthImage(ImagePtr image, float depth) override;

    void TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after) override;
    void TransitionBarrier(std::vector<ImagePtr> const& images, ImageLayout layout_before,
        ImageLayout layout_after) override;

    void StorageBarrier(ImagePtr image) override;
    void StorageBarrier(BufferPtr buffer) override;

    void BuildBottomLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
        std::vector<AccelerationStructureGeometryDesc> const& geometries) override;
    void BuildTopLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
        std::vector<AccelerationStructureInstanceDesc> const& instances) override;

    void CopyBuffer(BufferPtr src, uint64_t src_offset, BufferPtr dst, uint64_t dst_offset, uint64_t size) override;

    void CopyBufferToImage(ImagePtr dst, BufferPtr src) override;
    void UploadImage(ImagePtr dst, void const* data, size_t data_size) override;
    void CopyImage(ImagePtr dst, ImagePtr src) override;

private:
    void Begin();
    void EndRendering();

private:
    VulkanDevice& device_;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    bool closed_ = false;
    bool rendering_active_ = false;
    uint32_t current_vertex_stride_ = 0;
    VulkanGraphicsPipeline* current_graphics_pipeline_ = nullptr;
    VulkanComputePipeline* current_compute_pipeline_ = nullptr;
    VkPipelineBindPoint current_pipeline_bind_point_ = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    std::vector<BufferPtr> staging_buffers_;
};

}  // namespace gpu
