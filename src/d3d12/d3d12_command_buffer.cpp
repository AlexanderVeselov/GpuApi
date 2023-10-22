#include "d3d12_command_buffer.hpp"
#include "d3d12_device.hpp"
#include "d3d12_common.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_buffer.hpp"
#include "d3d12_image.hpp"
#include "d3d12_pipeline.hpp"

#include <cassert>

namespace gpu
{
namespace
{
D3D12_RESOURCE_STATES LayoutToD3D12ResourceState(ImageLayout layout, bool is_depth)
{
    // Depth is not implemented
    assert(!is_depth);

    switch (layout)
    {
    case ImageLayout::kUndefined:
        return D3D12_RESOURCE_STATE_COMMON;
    case ImageLayout::kPresent:
        return D3D12_RESOURCE_STATE_PRESENT;
    case ImageLayout::kCopySrc:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ImageLayout::kCopyDst:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case ImageLayout::kRenderTarget:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ImageLayout::kShaderRead:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case ImageLayout::kShaderReadWrite:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    default:
        assert(!"Unknown resource state");
        return D3D12_RESOURCE_STATE_COMMON;
    }
}
}

D3D12CommandBuffer::D3D12CommandBuffer(D3D12Device& device,
    D3D12Queue& queue, D3D12_COMMAND_LIST_TYPE command_list_type)
    : queue_(queue)
{
    auto d3d12_device = device.GetD3D12Device();
    ThrowIfFailed(d3d12_device->CreateCommandAllocator(command_list_type,
        IID_PPV_ARGS(&command_allocator_)));
    ThrowIfFailed(d3d12_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        command_allocator_.Get(), nullptr, IID_PPV_ARGS(&cmd_list_)));
}

void D3D12CommandBuffer::BindDescriptorsGraphics()
{
    assert(current_graphics_pipeline_ != nullptr);

    auto const& bindings = current_graphics_pipeline_->GetBindings();

    for (D3D12Binding const& binding : bindings)
    {
        switch (binding.descriptor_type)
        {
        case D3D12Binding::DescriptorType::kRootConstant:
            assert(!"D3D12Binding::DescriptorType::kRootConstant: Not implemented");
            //cmd_list_->SetGraphicsRoot32BitConstants(binding.root_parameter_index,
                               //binding.num_32bit_values_to_set, binding.data, 0);
            break;
        case D3D12Binding::DescriptorType::kDescriptorTable:
            assert(!"D3D12Binding::DescriptorType::kDescriptorTable: Not implemented");
            //cmd_list_->SetGraphicsRootDescriptorTable(binding.root_parameter_index,
                           //binding.gpu_handle);
            break;
        }
    }
}

void D3D12CommandBuffer::BindDescriptorsCompute(D3D12ComputePipeline* d3d12_pipeline)
{
    assert(d3d12_pipeline != nullptr);

    //current_pipeline_
    //cmd_list_->SetGraphicsRoot32BitConstants
    //cmd_list_->SetGraphicsRootConstantBufferView
    //cmd_list_->SetGraphics
}

void D3D12CommandBuffer::Dispatch(ComputePipelinePtr const& pipeline, std::uint32_t num_groups_x,
    std::uint32_t num_groups_y, std::uint32_t num_groups_z)
{
    assert(num_groups_x > 0 && num_groups_y > 0 && num_groups_z > 0);

    D3D12ComputePipeline* d3d12_pipeline = static_cast<D3D12ComputePipeline*>(pipeline.get());
    //cmd_list_->SetComputeRootDescriptorTable()
    cmd_list_->SetPipelineState(d3d12_pipeline->GetPipelineState());
    cmd_list_->SetComputeRootSignature(d3d12_pipeline->GetRootSignature());
    BindDescriptorsCompute(d3d12_pipeline);
    cmd_list_->Dispatch(num_groups_x, num_groups_y, num_groups_z);
}

void D3D12CommandBuffer::Draw(std::uint32_t vertex_count, std::uint32_t start_vertex_location)
{
    DrawInstanced(vertex_count, 1u, start_vertex_location, 0u);
}

void D3D12CommandBuffer::DrawIndexed(std::uint32_t index_count, std::uint32_t start_index_location,
    std::uint32_t start_vertex_location)
{
    DrawIndexedInstanced(index_count, 1u, start_index_location, start_vertex_location, 0u);
}

void D3D12CommandBuffer::DrawInstanced(std::uint32_t vertex_count, std::uint32_t instance_count,
    std::uint32_t start_vertex_location, std::uint32_t start_instance_location)
{
    BindDescriptorsGraphics();
    cmd_list_->DrawInstanced(vertex_count, instance_count, start_vertex_location, start_instance_location);
}

void D3D12CommandBuffer::DrawIndexedInstanced(std::uint32_t index_count,
    std::uint32_t instance_count, std::uint32_t start_index_location,
    std::uint32_t start_vertex_location, std::uint32_t start_instance_location)
{
    BindDescriptorsGraphics();
    cmd_list_->DrawIndexedInstanced(index_count, instance_count, start_index_location, start_vertex_location, start_instance_location);
}

void D3D12CommandBuffer::BindGraphicsPipeline(GraphicsPipelinePtr const& pipeline)
{
    D3D12GraphicsPipeline* d3d12_pipeline = static_cast<D3D12GraphicsPipeline*>(pipeline.get());
    current_graphics_pipeline_ = d3d12_pipeline;

    auto& pipeline_desc = d3d12_pipeline->GetDesc();

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
    rtvs.resize(pipeline_desc.color_attachments.size());

    std::uint32_t width = 0;
    std::uint32_t height = 0;

    for (auto i = 0; i < rtvs.size(); ++i)
    {
        D3D12Image* d3d12_image = static_cast<D3D12Image*>(pipeline_desc.color_attachments[i].get());
        rtvs[i] = d3d12_image->GetRTVHandle();

        ///@TODO: check if all images are equal width/height
        width = d3d12_image->GetWidth();
        height = d3d12_image->GetHeight();
    }

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmd_list_->RSSetViewports(1u, &viewport);

    D3D12_RECT scissor = {};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = width;
    scissor.bottom = height;
    cmd_list_->RSSetScissorRects(1u, &scissor);

    ///@TODO: add DSV!!!
    cmd_list_->OMSetRenderTargets(rtvs.size(), rtvs.data(), false, nullptr);
    cmd_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd_list_->SetPipelineState(d3d12_pipeline->GetPipelineState());
    cmd_list_->SetGraphicsRootSignature(d3d12_pipeline->GetRootSignature());

}

void D3D12CommandBuffer::SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride)
{
    D3D12Buffer* d3d12_buffer = static_cast<D3D12Buffer*>(buffer.get());

    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
    vertex_buffer_view.BufferLocation = d3d12_buffer->GetResource()->GetGPUVirtualAddress();
    vertex_buffer_view.SizeInBytes = buffer->GetSize();
    vertex_buffer_view.StrideInBytes = vertex_stride;

    cmd_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view);
}

void D3D12CommandBuffer::ClearImage(ImagePtr image, float r, float g, float b, float a)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());

    D3D12_RECT rect = { 0, 0, (LONG)image->GetWidth(), (LONG)image->GetHeight() };
    float color[4] = { r, g, b, a };

    cmd_list_->ClearRenderTargetView(d3d12_image->GetRTVHandle(), color, 1u, &rect);
}

void D3D12CommandBuffer::TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = d3d12_image->GetResource();
    barrier.Transition.StateBefore = LayoutToD3D12ResourceState(layout_before, false);
    barrier.Transition.StateAfter = LayoutToD3D12ResourceState(layout_after, false);

    cmd_list_->ResourceBarrier(1u, &barrier);
}

void D3D12CommandBuffer::StorageBarrier(ImagePtr image)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = d3d12_image->GetResource();

    cmd_list_->ResourceBarrier(1u, &barrier);
}

void D3D12CommandBuffer::Reset()
{
     assert(!"Not implemented!");
}

void D3D12CommandBuffer::End()
{
    ThrowIfFailed(cmd_list_->Close());
}

} // namespace gpu
