#include "d3d12_command_buffer.hpp"
#include "d3d12_buffer.hpp"
#include "d3d12_descriptor_set.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_queue.hpp"

#include <cassert>
#include <stdexcept>

namespace gpu
{
namespace
{
D3D12_RESOURCE_STATES LayoutToD3D12ResourceState(ImageLayout layout, bool is_depth)
{
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
        return is_depth ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ImageLayout::kShaderRead:
        return is_depth ? D3D12_RESOURCE_STATE_DEPTH_READ :
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case ImageLayout::kShaderReadWrite:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    assert(false && "LayoutToD3D12ResourceState: unsupported image layout");
    return D3D12_RESOURCE_STATE_COMMON;
}

bool IsDepthFormat(ImageFormat format)
{
    return format == ImageFormat::kD32_Float || format == ImageFormat::kR32_Typeless;
}

D3D12Descriptor CopyDescriptorToGPU(
    D3D12DescriptorManager& descriptor_manager,
    D3D12Descriptor descriptor)
{
    assert(descriptor.IsValid() && "CopyDescriptorToGPU: source descriptor is invalid");

    switch (descriptor.heap)
    {
    case D3D12DescriptorHeapId::CPU_CBV_SRV_UAV:
        return descriptor_manager.CopyToGPUCBVSRVUAV(descriptor);
    case D3D12DescriptorHeapId::CPU_SAMPLER:
        return descriptor_manager.CopyToGPUSampler(descriptor);
    default:
        assert(false && "CopyDescriptorToGPU: source descriptor heap is not shader-visible-copyable");
        return {};
    }
}
}

D3D12CommandBuffer::D3D12CommandBuffer(
    D3D12Device& device,
    D3D12Queue& queue,
    D3D12_COMMAND_LIST_TYPE command_list_type)
    : device_(device)
    , queue_(queue)
    , command_list_type_(command_list_type)
{
    auto d3d12_device = device_.GetD3D12Device();
    ThrowIfFailed(d3d12_device->CreateCommandAllocator(command_list_type_,
        IID_PPV_ARGS(&command_allocator_)));
    ThrowIfFailed(d3d12_device->CreateCommandList(
        0,
        command_list_type_,
        command_allocator_.Get(),
        nullptr,
        IID_PPV_ARGS(&cmd_list_)));
}

D3D12CommandBuffer::~D3D12CommandBuffer()
{
    FreeCommittedDescriptors();
}

void D3D12CommandBuffer::SetVertexBuffer(BufferPtr buffer, std::size_t vertex_stride)
{
    D3D12Buffer* d3d12_buffer = static_cast<D3D12Buffer*>(buffer.get());
    assert(d3d12_buffer && "D3D12CommandBuffer::SetVertexBuffer: buffer is not a D3D12Buffer");

    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = d3d12_buffer->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(buffer->GetSize());
    view.StrideInBytes = static_cast<UINT>(vertex_stride);

    cmd_list_->IASetVertexBuffers(0, 1, &view);
}

void D3D12CommandBuffer::SetIndexBuffer(BufferPtr buffer)
{
    D3D12Buffer* d3d12_buffer = static_cast<D3D12Buffer*>(buffer.get());
    assert(d3d12_buffer && "D3D12CommandBuffer::SetIndexBuffer: buffer is not a D3D12Buffer");

    D3D12_INDEX_BUFFER_VIEW view = {};
    view.BufferLocation = d3d12_buffer->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(buffer->GetSize());
    view.Format = d3d12_buffer->GetStride() == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    cmd_list_->IASetIndexBuffer(&view);
}

void D3D12CommandBuffer::BindPipeline(GraphicsPipelinePtr const& pipeline)
{
    D3D12GraphicsPipeline* d3d12_pipeline = static_cast<D3D12GraphicsPipeline*>(pipeline.get());
    assert(d3d12_pipeline && "D3D12CommandBuffer::BindPipeline: pipeline is not a D3D12GraphicsPipeline");

    current_graphics_pipeline_ = d3d12_pipeline;
    cmd_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd_list_->SetPipelineState(d3d12_pipeline->GetPipelineState());
    cmd_list_->SetGraphicsRootSignature(d3d12_pipeline->GetRootSignature());
}

void D3D12CommandBuffer::BindPipeline(ComputePipelinePtr const& pipeline)
{
    D3D12ComputePipeline* d3d12_pipeline = static_cast<D3D12ComputePipeline*>(pipeline.get());
    assert(d3d12_pipeline && "D3D12CommandBuffer::BindPipeline: pipeline is not a D3D12ComputePipeline");

    current_compute_pipeline_ = d3d12_pipeline;
    cmd_list_->SetPipelineState(d3d12_pipeline->GetPipelineState());
    cmd_list_->SetComputeRootSignature(d3d12_pipeline->GetRootSignature());
}

void D3D12CommandBuffer::BindDescriptorSet(DescriptorSetPtr const& descriptor_set)
{
    D3D12DescriptorSet* d3d12_descriptor_set =
        dynamic_cast<D3D12DescriptorSet*>(descriptor_set.get());
    if (!d3d12_descriptor_set)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::BindDescriptorSet: descriptor set was not created by the D3D12 backend");
    }

    current_descriptor_set_ = d3d12_descriptor_set;
}

void D3D12CommandBuffer::Dispatch(std::uint32_t num_groups_x, std::uint32_t num_groups_y,
    std::uint32_t num_groups_z)
{
    assert(num_groups_x > 0 && num_groups_y > 0 && num_groups_z > 0 &&
        "D3D12CommandBuffer::Dispatch: dispatch group counts must be greater than zero");
    assert(current_compute_pipeline_ &&
        "D3D12CommandBuffer::Dispatch: compute pipeline must be bound before dispatch");

    BindDescriptorsCompute();
    cmd_list_->Dispatch(num_groups_x, num_groups_y, num_groups_z);
}

void D3D12CommandBuffer::Draw(uint32_t vertex_count, uint32_t instance_count,
    uint32_t first_vertex, uint32_t first_instance)
{
    BindDescriptorsGraphics();
    cmd_list_->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

void D3D12CommandBuffer::DrawIndexed(uint32_t index_count, uint32_t instance_count,
    uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
    BindDescriptorsGraphics();
    cmd_list_->DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void D3D12CommandBuffer::SetRenderTarget(ImagePtr color_attachment, ImagePtr depth_attachment)
{
    SetRenderTargets({ color_attachment }, depth_attachment);
}

void D3D12CommandBuffer::SetRenderTargets(std::vector<ImagePtr> const& color_attachments, ImagePtr depth_attachment)
{
    assert(color_attachments.size() <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        "D3D12CommandBuffer::SetRenderTargets: too many color attachments");

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles(color_attachments.size());
    for (std::size_t i = 0; i < color_attachments.size(); ++i)
    {
        D3D12Image* d3d12_image = static_cast<D3D12Image*>(color_attachments[i].get());
        assert(d3d12_image && "D3D12CommandBuffer::SetRenderTargets: color attachment is not a D3D12Image");
        rtv_handles[i] = d3d12_image->GetRTVHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = {};
    if (depth_attachment)
    {
        D3D12Image* d3d12_image = static_cast<D3D12Image*>(depth_attachment.get());
        assert(d3d12_image && "D3D12CommandBuffer::SetRenderTargets: depth attachment is not a D3D12Image");
        dsv_handle = d3d12_image->GetDSVHandle();
    }

    cmd_list_->OMSetRenderTargets(
        static_cast<UINT>(rtv_handles.size()),
        rtv_handles.empty() ? nullptr : rtv_handles.data(),
        FALSE,
        depth_attachment ? &dsv_handle : nullptr);
}

void D3D12CommandBuffer::SetViewport(const Viewport& viewport)
{
    D3D12_VIEWPORT d3d12_viewport = {};
    d3d12_viewport.TopLeftX = viewport.x;
    d3d12_viewport.TopLeftY = viewport.y;
    d3d12_viewport.Width = viewport.width;
    d3d12_viewport.Height = viewport.height;
    d3d12_viewport.MinDepth = viewport.min_depth;
    d3d12_viewport.MaxDepth = viewport.max_depth;

    cmd_list_->RSSetViewports(1, &d3d12_viewport);
}

void D3D12CommandBuffer::SetScissor(const Rect& rect)
{
    D3D12_RECT d3d12_rect = {};
    d3d12_rect.left = rect.x;
    d3d12_rect.top = rect.y;
    d3d12_rect.right = rect.x + rect.width;
    d3d12_rect.bottom = rect.y + rect.height;

    cmd_list_->RSSetScissorRects(1, &d3d12_rect);
}

void D3D12CommandBuffer::ClearImage(ImagePtr image, float r, float g, float b, float a)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());
    assert(d3d12_image && "D3D12CommandBuffer::ClearImage: image is not a D3D12Image");

    D3D12_RECT rect = {
        0,
        0,
        static_cast<LONG>(image->GetWidth()),
        static_cast<LONG>(image->GetHeight())
    };
    float color[4] = { r, g, b, a };
    cmd_list_->ClearRenderTargetView(d3d12_image->GetRTVHandle(), color, 1, &rect);
}

void D3D12CommandBuffer::TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());
    assert(d3d12_image && "D3D12CommandBuffer::TransitionBarrier: image is not a D3D12Image");

    const bool is_depth = IsDepthFormat(image->GetFormat());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = d3d12_image->GetResource();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = LayoutToD3D12ResourceState(layout_before, is_depth);
    barrier.Transition.StateAfter = LayoutToD3D12ResourceState(layout_after, is_depth);
    cmd_list_->ResourceBarrier(1, &barrier);
}

void D3D12CommandBuffer::StorageBarrier(ImagePtr image)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());
    assert(d3d12_image && "D3D12CommandBuffer::StorageBarrier: image is not a D3D12Image");

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = d3d12_image->GetResource();

    cmd_list_->ResourceBarrier(1, &barrier);
}

void D3D12CommandBuffer::CopyBuffer(Buffer* src, uint64_t src_offset, Buffer* dst, uint64_t dst_offset, uint64_t size)
{
    D3D12Buffer* d3d12_src = static_cast<D3D12Buffer*>(src);
    D3D12Buffer* d3d12_dst = static_cast<D3D12Buffer*>(dst);
    assert(d3d12_src && d3d12_dst && "D3D12CommandBuffer::CopyBuffer: src/dst must be D3D12Buffer");

    cmd_list_->CopyBufferRegion(
        d3d12_dst->GetResource(),
        dst_offset,
        d3d12_src->GetResource(),
        src_offset,
        size);
}

void D3D12CommandBuffer::CopyBufferToImage(Image* dst, Buffer* src)
{
    D3D12Image* d3d12_dst = static_cast<D3D12Image*>(dst);
    D3D12Buffer* d3d12_src = static_cast<D3D12Buffer*>(src);
    assert(d3d12_dst && d3d12_src && "D3D12CommandBuffer::CopyBufferToImage: dst must be D3D12Image and src must be D3D12Buffer");

    D3D12_RESOURCE_DESC image_desc = d3d12_dst->GetResource()->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT num_rows = 0;
    UINT64 row_size = 0;
    UINT64 total_size = 0;
    device_.GetD3D12Device()->GetCopyableFootprints(
        &image_desc,
        0,
        1,
        0,
        &footprint,
        &num_rows,
        &row_size,
        &total_size);

    D3D12_TEXTURE_COPY_LOCATION dst_location = {};
    dst_location.pResource = d3d12_dst->GetResource();
    dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst_location.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src_location = {};
    src_location.pResource = d3d12_src->GetResource();
    src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src_location.PlacedFootprint = footprint;

    cmd_list_->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
}

void D3D12CommandBuffer::CopyImage(Image* dst, Image* src)
{
    D3D12Image* d3d12_dst = static_cast<D3D12Image*>(dst);
    D3D12Image* d3d12_src = static_cast<D3D12Image*>(src);
    assert(d3d12_dst && d3d12_src && "D3D12CommandBuffer::CopyImage: dst/src must be D3D12Image");
    assert(dst->GetWidth() == src->GetWidth() && "D3D12CommandBuffer::CopyImage: source and destination widths must match");
    assert(dst->GetHeight() == src->GetHeight() && "D3D12CommandBuffer::CopyImage: source and destination heights must match");
    assert(dst->GetFormat() == src->GetFormat() && "D3D12CommandBuffer::CopyImage: source and destination formats must match");

    D3D12_TEXTURE_COPY_LOCATION dst_location = {};
    dst_location.pResource = d3d12_dst->GetResource();
    dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst_location.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src_location = {};
    src_location.pResource = d3d12_src->GetResource();
    src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src_location.SubresourceIndex = 0;

    cmd_list_->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
}

void D3D12CommandBuffer::Close()
{
    if (closed_)
    {
        return;
    }

    ThrowIfFailed(cmd_list_->Close());
    closed_ = true;
}

void D3D12CommandBuffer::BindDescriptorsGraphics()
{
    if (!current_graphics_pipeline_)
    {
        return;
    }

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    ID3D12DescriptorHeap* heaps[] = {
        descriptor_manager.GetGPUCBVSRVUAVHeap(),
        descriptor_manager.GetGPUSamplerHeap()
    };
    cmd_list_->SetDescriptorHeaps(2, heaps);

    if (!current_descriptor_set_)
    {
        return;
    }

    assert(&current_descriptor_set_->GetLayout() == &current_graphics_pipeline_->GetLayout() &&
        "D3D12CommandBuffer::BindDescriptorsGraphics: descriptor set layout must match the current graphics pipeline layout");

    for (D3D12DescriptorSet::BoundDescriptor const& descriptor :
        current_descriptor_set_->GetBoundDescriptors())
    {
        D3D12Descriptor gpu_descriptor = CopyDescriptorToGPU(descriptor_manager, descriptor.cpu_descriptor);
        committed_descriptors_.push_back(gpu_descriptor);

        cmd_list_->SetGraphicsRootDescriptorTable(
            descriptor.root_parameter_index,
            descriptor_manager.GetGPU(gpu_descriptor));
    }
}

void D3D12CommandBuffer::BindDescriptorsCompute()
{
    if (!current_compute_pipeline_)
    {
        return;
    }

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    ID3D12DescriptorHeap* heaps[] = {
        descriptor_manager.GetGPUCBVSRVUAVHeap(),
        descriptor_manager.GetGPUSamplerHeap()
    };
    cmd_list_->SetDescriptorHeaps(2, heaps);

    if (!current_descriptor_set_)
    {
        return;
    }

    assert(&current_descriptor_set_->GetLayout() == &current_compute_pipeline_->GetLayout() &&
        "D3D12CommandBuffer::BindDescriptorsCompute: descriptor set layout must match the current compute pipeline layout");

    for (D3D12DescriptorSet::BoundDescriptor const& descriptor :
        current_descriptor_set_->GetBoundDescriptors())
    {
        D3D12Descriptor gpu_descriptor = CopyDescriptorToGPU(descriptor_manager, descriptor.cpu_descriptor);
        committed_descriptors_.push_back(gpu_descriptor);

        cmd_list_->SetComputeRootDescriptorTable(
            descriptor.root_parameter_index,
            descriptor_manager.GetGPU(gpu_descriptor));
    }
}

void D3D12CommandBuffer::FreeCommittedDescriptors()
{
    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    for (D3D12Descriptor descriptor : committed_descriptors_)
    {
        descriptor_manager.Free(descriptor);
    }

    committed_descriptors_.clear();
}

} // namespace gpu
