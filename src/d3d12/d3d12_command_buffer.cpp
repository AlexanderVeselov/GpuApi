#include "d3d12_command_buffer.hpp"
#include "d3d12_device.hpp"
#include "d3d12_common.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"

namespace gpu
{
    D3D12CommandBuffer::D3D12CommandBuffer(D3D12Device& device,
        D3D12Queue& queue, D3D12_COMMAND_LIST_TYPE command_list_type)
        //: device_(device)
        //, queue_(queue)
        //, command_list_type_(command_list_type)
    {
        auto d3d12_device = device.GetD3D12Device();
        ThrowIfFailed(d3d12_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            queue.GetCommandAllocator(), nullptr, IID_PPV_ARGS(&cmd_list_)));
    }

    void D3D12CommandBuffer::Draw(std::uint32_t vertex_count, std::uint32_t start_vertex_location)
    {
        DrawInstanced(vertex_count, 1u, start_vertex_location, 0u);
    }

    void D3D12CommandBuffer::DrawInstanced(std::uint32_t vertex_count, std::uint32_t instance_count,
        std::uint32_t start_vertex_location, std::uint32_t start_instance_location)
    {
        cmd_list_->DrawInstanced(vertex_count, instance_count, start_vertex_location, start_instance_location);
    }

    void D3D12CommandBuffer::BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline)
    {
    }

    void D3D12CommandBuffer::ClearImage(ImagePtr image, float r, float g, float b, float a)
    {
        D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());

        D3D12_RECT rect = { 0, 0, image->GetWidth(), image->GetHeight() };
        float color[4] = { r, g, b, a };

        cmd_list_->ClearRenderTargetView(d3d12_image->GetRTVHandle(), color, 1u, &rect);
    }

    void D3D12CommandBuffer::End()
    {
        ThrowIfFailed(cmd_list_->Close());
    }

} // namespace gpu
