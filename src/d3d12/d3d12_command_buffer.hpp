#pragma once

#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include "gpu_command_buffer.hpp"

#include <vector>

namespace gpu
{
class D3D12Device;
class D3D12DescriptorSet;
class D3D12ComputePipeline;
class D3D12Queue;
class D3D12GraphicsPipeline;
class D3D12Buffer;

class D3D12CommandBuffer final : public CommandBuffer
{
public:
    D3D12CommandBuffer(D3D12Device& device, D3D12Queue& queue, D3D12_COMMAND_LIST_TYPE command_list_type);
    ~D3D12CommandBuffer() = default;

    ID3D12GraphicsCommandList* GetCommandList() const { return cmd_list_.Get(); }

    void SetVertexBuffer(BufferPtr buffer, size_t vertex_stride) override;
    void SetIndexBuffer(BufferPtr buffer) override;

    void BindPipeline(GraphicsPipelinePtr const& pipeline) override;
    void BindPipeline(ComputePipelinePtr const& pipeline) override;
    void BindDescriptorSet(DescriptorSetPtr const& descriptor_set) override;
    void SetRootConstants(void const* data, size_t data_size, size_t dst_offset = 0) override;
    void Dispatch(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) override;
    void Draw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0,
        uint32_t first_instance = 0) override;
    void DrawIndexed(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0,
        int32_t vertex_offset = 0, uint32_t first_instance = 0) override;

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

    void Close();

private:
    void BindDescriptorHeaps();
    void BindDescriptorsGraphics();
    void BindDescriptorsCompute();
    void TransitionBuffer(D3D12Buffer& buffer, D3D12_RESOURCE_STATES state_after);

private:
    D3D12Device& device_;
    D3D12Queue& queue_;
    D3D12_COMMAND_LIST_TYPE command_list_type_;
    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> cmd_list_;
    bool closed_ = false;
    uint32_t current_vertex_stride_ = 0;
    D3D12GraphicsPipeline* current_graphics_pipeline_ = nullptr;
    D3D12ComputePipeline* current_compute_pipeline_ = nullptr;
    D3D12DescriptorSet* current_descriptor_set_ = nullptr;
    bool descriptor_heaps_bound_ = false;
    std::vector<BufferPtr> staging_buffers_;
};

}  // namespace gpu
