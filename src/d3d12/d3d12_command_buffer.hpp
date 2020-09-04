#ifndef D3D12_COMMAND_BUFFER_HPP_
#define D3D12_COMMAND_BUFFER_HPP_

#include "gpu_command_buffer.hpp"

namespace gpu
{
    class D3D12CommandBuffer : public CommandBuffer
    {
    public:
        void Dispatch(std::uint32_t num_groups_x, std::uint32_t num_groups_y, std::uint32_t num_groups_z) override;
        void Draw() override;

        void BindComputePipeline(ComputePipelinePtr const& pipeline) override;
        void BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline) override;

    };

} // namespace gpu

#endif // D3D12_COMMAND_BUFFER_HPP_
