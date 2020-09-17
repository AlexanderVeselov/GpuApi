#ifndef GPU_COMMAND_BUFFER_HPP_
#define GPU_COMMAND_BUFFER_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class CommandBuffer
    {
    public:
        //virtual void Dispatch(std::uint32_t num_groups_x, std::uint32_t num_groups_y, std::uint32_t num_groups_z) = 0;
        //virtual void Draw() = 0;

        //virtual void BindComputePipeline(ComputePipelinePtr const& pipeline) = 0;
        //virtual void BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline) = 0;

    };

} // namespace gpu

#endif // GPU_COMMAND_BUFFER_HPP_
