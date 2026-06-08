#pragma once

#include "gpu_acceleration_structure.hpp"
#include "gpu_types.hpp"

#include <cstddef>

namespace gpu
{
/// Records rendering, compute, copy, and resource barrier commands for a Queue.
class CommandBuffer
{
public:
    virtual ~CommandBuffer() = default;

    /// Binds a vertex buffer. The stride must match the vertex format expected by the pipeline.
    virtual void SetVertexBuffer(BufferPtr buffer, size_t vertex_stride) = 0;

    /// Binds an index buffer.
    virtual void SetIndexBuffer(BufferPtr buffer) = 0;

    /// Dispatches work using the currently bound compute pipeline and descriptor set.
    virtual void Dispatch(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) = 0;

    /// Draws non-indexed geometry using the currently bound graphics pipeline and descriptor set.
    virtual void Draw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0,
        uint32_t first_instance = 0) = 0;

    /// Draws indexed geometry using the currently bound graphics pipeline and descriptor set.
    virtual void DrawIndexed(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0,
        int32_t vertex_offset = 0, uint32_t first_instance = 0) = 0;

    /// Binds graphics pipeline state for subsequent draw calls.
    virtual void BindPipeline(GraphicsPipelinePtr const& pipeline) = 0;

    /// Binds compute pipeline state for subsequent dispatch calls.
    virtual void BindPipeline(ComputePipelinePtr const& pipeline) = 0;

    /// Binds resource descriptors for subsequent draw or dispatch calls.
    virtual void BindDescriptorSet(DescriptorSetPtr const& descriptor_set) = 0;

    /// Writes root constants / push constants to the currently bound pipeline layout.
    virtual void SetRootConstants(void const* data, size_t data_size, size_t dst_offset = 0) = 0;

    /// Sets one color attachment and an optional depth attachment.
    virtual void SetRenderTarget(ImagePtr color_attachment, ImagePtr depth_attachment) = 0;

    /// Sets multiple color attachments and an optional depth attachment.
    virtual void SetRenderTargets(std::vector<ImagePtr> const& color_attachments, ImagePtr depth_attachment) = 0;

    /// Sets the raster viewport.
    virtual void SetViewport(const Viewport& viewport) = 0;

    /// Sets the raster scissor rectangle.
    virtual void SetScissor(const Rect& rect) = 0;

    /// Clears an image with a constant RGBA value.
    virtual void ClearImage(ImagePtr image, float r, float g, float b, float a) = 0;

    /// Clears a depth image.
    virtual void ClearDepthImage(ImagePtr image, float depth) = 0;

    /// Inserts an image layout transition barrier.
    virtual void TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after) = 0;

    /// Inserts a transition barrier for multiple images.
    virtual void TransitionBarrier(std::vector<ImagePtr> const& images, ImageLayout layout_before,
        ImageLayout layout_after) = 0;

    /// Inserts an unordered-access/storage synchronization barrier for an image.
    virtual void StorageBarrier(ImagePtr image) = 0;

    /// Inserts an unordered-access/storage synchronization barrier for a buffer.
    virtual void StorageBarrier(BufferPtr buffer) = 0;

    /// Builds a bottom-level acceleration structure from triangle geometry.
    virtual void BuildBottomLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
        std::vector<AccelerationStructureGeometryDesc> const& geometries) = 0;

    /// Builds a top-level acceleration structure. The command buffer uploads the backend instance descriptors.
    virtual void BuildTopLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
        std::vector<AccelerationStructureInstanceDesc> const& instances) = 0;

    /// Copies a byte range between buffers.
    virtual void CopyBuffer(BufferPtr src, uint64_t src_offset, BufferPtr dst, uint64_t dst_offset, uint64_t size) = 0;

    /// Copies a linear buffer into an image.
    virtual void CopyBufferToImage(ImagePtr dst, BufferPtr src) = 0;

    /// Uploads tightly packed CPU image data into one mip level of an image.
    virtual void UploadImage(ImagePtr dst, void const* data, size_t data_size, uint32_t mip_level = 0) = 0;

    /// Copies the contents of one image into another compatible image.
    virtual void CopyImage(ImagePtr dst, ImagePtr src) = 0;
};

}  // namespace gpu
