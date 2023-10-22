#ifndef D3D12_COMMAND_BUFFER_HPP_
#define D3D12_COMMAND_BUFFER_HPP_

#include "gpu_command_buffer.hpp"
#include "d3d12_queue.hpp"

namespace gpu
{
class D3D12GraphicsPipeline;
class D3D12ComputePipeline;

class D3D12CommandBuffer : public CommandBuffer
{
public:
    D3D12CommandBuffer(D3D12Device& device, D3D12Queue& queue, D3D12_COMMAND_LIST_TYPE command_list_type);

    void Dispatch(ComputePipelinePtr const& pipeline, std::uint32_t num_groups_x,
        std::uint32_t num_groups_y, std::uint32_t num_groups_z) override;
    void Draw(std::uint32_t vertex_count, std::uint32_t start_vertex_location) override;
    void DrawIndexed(std::uint32_t index_count, std::uint32_t start_index_location,
        std::uint32_t start_vertex_location) override;

    void DrawInstanced(std::uint32_t vertex_count, std::uint32_t instance_count,
        std::uint32_t start_vertex_location, std::uint32_t start_instance_location) override;
    void DrawIndexedInstanced(std::uint32_t index_count, std::uint32_t instance_count,
        std::uint32_t start_index_location, std::uint32_t start_vertex_location,
        std::uint32_t start_instance_location) override;

    void BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline) override;
    //void BindComputePipeline(ComputePipelinePtr const& pipeline) override;

    void SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride) override;

    void ClearImage(ImagePtr image, float r, float g, float b, float a) override;

    void TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after) override;

    void StorageBarrier(ImagePtr image) override;

    void Reset() override;
    void End() override;

    ID3D12GraphicsCommandList* GetCommandList() const { return cmd_list_.Get(); }
    ID3D12CommandAllocator* GetCommandAllocator() const { return command_allocator_.Get(); }

private:
    void BindDescriptorsGraphics();
    void BindDescriptorsCompute(D3D12ComputePipeline* d3d12_pipeline);
    D3D12Queue& queue_;

    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> cmd_list_;
    D3D12GraphicsPipeline* current_graphics_pipeline_ = nullptr; // TODO: Fix it

};

} // namespace gpu

#endif // D3D12_COMMAND_BUFFER_HPP_
