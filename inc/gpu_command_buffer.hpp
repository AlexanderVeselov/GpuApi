#ifndef GPU_COMMAND_BUFFER_HPP_
#define GPU_COMMAND_BUFFER_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class CommandBuffer
    {
    public:
        virtual void Dispatch(ComputePipelinePtr const& pipeline, std::uint32_t num_groups_x,
            std::uint32_t num_groups_y, std::uint32_t num_groups_z) = 0;
        virtual void Draw(std::uint32_t vertex_count, std::uint32_t start_vertex_location) = 0;
        virtual void DrawIndexed(std::uint32_t index_count, std::uint32_t start_index_location,
            std::uint32_t start_vertex_location) = 0;

        virtual void DrawInstanced(std::uint32_t vertex_count, std::uint32_t instance_count,
            std::uint32_t start_vertex_location, std::uint32_t start_instance_location) = 0;
        virtual void DrawIndexedInstanced(std::uint32_t index_count, std::uint32_t instance_count,
            std::uint32_t start_index_location, std::uint32_t start_vertex_location,
            std::uint32_t start_instance_location) = 0;

        virtual void BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline) = 0;
        //virtual void BindComputePipeline(ComputePipelinePtr const& pipeline) = 0;

        virtual void SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride) = 0;

        virtual void ClearImage(ImagePtr image, float r, float g, float b, float a) = 0;

        virtual void TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after) = 0;

        virtual void StorageBarrier(ImagePtr image) = 0;

        virtual void Reset() = 0;
        virtual void End() = 0;
        virtual ~CommandBuffer() = default;
    };

} // namespace gpu

#endif // GPU_COMMAND_BUFFER_HPP_
