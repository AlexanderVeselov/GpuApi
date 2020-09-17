#ifndef D3D12_COMMAND_BUFFER_HPP_
#define D3D12_COMMAND_BUFFER_HPP_

#include "gpu_command_buffer.hpp"
#include "d3d12_queue.hpp"

namespace gpu
{
    class D3D12CommandBuffer : public CommandBuffer
    {
    public:
        D3D12CommandBuffer(D3D12Device& device, D3D12Queue& queue, D3D12_COMMAND_LIST_TYPE command_list_type);

        //void Dispatch(std::uint32_t num_groups_x, std::uint32_t num_groups_y, std::uint32_t num_groups_z) override;
        //void Draw() override;

        //void BindComputePipeline(ComputePipelinePtr const& pipeline) override;
        //void BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline) override;

    private:
        //D3D12Device& device_;
        //D3D12Queue& queue_;
        //D3D12_COMMAND_LIST_TYPE command_list_type_;

        ComPtr<ID3D12GraphicsCommandList> cmd_list_;

    };

} // namespace gpu

#endif // D3D12_COMMAND_BUFFER_HPP_
