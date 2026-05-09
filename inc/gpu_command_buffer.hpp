#pragma once

#include "gpu_types.hpp"

namespace gpu
{
class CommandBuffer
{
public:
    virtual ~CommandBuffer() = default;

    virtual void SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride) = 0;
    virtual void SetIndexBuffer(BufferPtr buffer) = 0;

    virtual void Dispatch(ComputePipelinePtr const& pipeline, std::uint32_t num_groups_x,
        std::uint32_t num_groups_y, std::uint32_t num_groups_z) = 0;
    virtual void Draw(uint32_t vertex_count, uint32_t instance_count = 1,
        uint32_t first_vertex = 0, uint32_t first_instance = 0) = 0;
    virtual void DrawIndexed(uint32_t index_count, uint32_t instance_count = 1,
        uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0) = 0;

    virtual void BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline) = 0;

    virtual void SetRenderTarget(ImagePtr color_attachment, ImagePtr depth_attachment) = 0;
    virtual void SetRenderTargets(std::vector<ImagePtr> const& color_attachments,
        ImagePtr depth_attachment) = 0;

    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissor(const Rect& rect) = 0;
    void SetViewport(std::uint32_t width, std::uint32_t height)
    {
        SetViewport(Viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f });
    }
    void SetScissorRect(std::uint32_t width, std::uint32_t height)
    {
        SetScissor(Rect{ 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) });
    }

    virtual void ClearImage(ImagePtr image, float r, float g, float b, float a) = 0;

    virtual void TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after) = 0;
    virtual void StorageBarrier(ImagePtr image) = 0;

    virtual void CopyBuffer(Buffer* src, uint64_t src_offset, Buffer* dst, uint64_t dst_offset, uint64_t size) = 0;

    virtual void CopyBufferToImage(Image* dst, Buffer* src) = 0;

    virtual void End() = 0;
};

} // namespace gpu
