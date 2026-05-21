#include "d3d12_command_buffer.hpp"
#include "d3d12_acceleration_structure.hpp"
#include "d3d12_buffer.hpp"
#include "d3d12_descriptor_set.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_queue.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <vector>

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
        return is_depth ? D3D12_RESOURCE_STATE_DEPTH_READ
                        : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
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

D3D12_RAYTRACING_GEOMETRY_DESC ToD3D12GeometryDesc(AccelerationStructureGeometryDesc const& geometry)
{
    auto* vertex_buffer = dynamic_cast<D3D12Buffer*>(geometry.vertex_buffer.get());
    auto* index_buffer = dynamic_cast<D3D12Buffer*>(geometry.index_buffer.get());
    if (!vertex_buffer || !index_buffer)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::BuildBottomLevelAccelerationStructure: geometry buffers must be D3D12 buffers");
    }

    D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
    desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    desc.Flags = geometry.opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    desc.Triangles.VertexBuffer.StartAddress = vertex_buffer->GetGPUVirtualAddress() + geometry.vertex_offset;
    desc.Triangles.VertexBuffer.StrideInBytes = geometry.vertex_stride;
    desc.Triangles.VertexCount = geometry.vertex_count;
    desc.Triangles.VertexFormat = ImageToDXGIFormat(geometry.vertex_format);
    desc.Triangles.IndexBuffer = index_buffer->GetGPUVirtualAddress() + geometry.index_offset;
    desc.Triangles.IndexCount = geometry.index_count;
    desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    return desc;
}

D3D12_RAYTRACING_INSTANCE_DESC ToD3D12InstanceDesc(AccelerationStructureInstanceDesc const& instance)
{
    auto* bottom_level = dynamic_cast<D3D12AccelerationStructure*>(instance.bottom_level);
    if (!bottom_level || bottom_level->GetType() != AccelerationStructureType::kBottomLevel)
    {
        throw std::runtime_error("D3D12CommandBuffer::BuildTopLevelAccelerationStructure: instance BLAS is invalid");
    }

    D3D12_RAYTRACING_INSTANCE_DESC desc = {};
    std::memcpy(desc.Transform, instance.transform, sizeof(desc.Transform));
    desc.InstanceID = instance.instance_id;
    desc.InstanceMask = instance.instance_mask;
    desc.InstanceContributionToHitGroupIndex = 0;
    desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    desc.AccelerationStructure = bottom_level->GetGpuAddress();
    return desc;
}

}  // namespace

D3D12CommandBuffer::D3D12CommandBuffer(D3D12Device& device, D3D12Queue& queue,
    D3D12_COMMAND_LIST_TYPE command_list_type)
    : device_(device), queue_(queue), command_list_type_(command_list_type)
{
    auto d3d12_device = device_.GetD3D12Device();
    ThrowIfFailed(d3d12_device->CreateCommandAllocator(command_list_type_, IID_PPV_ARGS(&command_allocator_)));
    ThrowIfFailed(d3d12_device->CreateCommandList(0, command_list_type_, command_allocator_.Get(), nullptr,
        IID_PPV_ARGS(&cmd_list_)));
}

void D3D12CommandBuffer::SetVertexBuffer(BufferPtr buffer, size_t vertex_stride)
{
    D3D12Buffer* d3d12_buffer = static_cast<D3D12Buffer*>(buffer.get());
    assert(d3d12_buffer && "D3D12CommandBuffer::SetVertexBuffer: buffer is not a D3D12Buffer");
    current_vertex_stride_ = static_cast<uint32_t>(vertex_stride);
    if (current_graphics_pipeline_ && current_graphics_pipeline_->GetVertexStride() != 0
        && current_graphics_pipeline_->GetVertexStride() != current_vertex_stride_)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::SetVertexBuffer: vertex buffer stride does "
            "not match the current graphics pipeline input layout");
    }

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
    if (current_vertex_stride_ != 0 && d3d12_pipeline->GetVertexStride() != 0
        && d3d12_pipeline->GetVertexStride() != current_vertex_stride_)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::BindPipeline: current vertex buffer stride "
            "does not match the graphics pipeline input layout");
    }

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
    D3D12DescriptorSet* d3d12_descriptor_set = dynamic_cast<D3D12DescriptorSet*>(descriptor_set.get());
    if (!d3d12_descriptor_set)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::BindDescriptorSet: descriptor set was not "
            "created by the D3D12 backend");
    }

    current_descriptor_set_ = d3d12_descriptor_set;
}

void D3D12CommandBuffer::Dispatch(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z)
{
    assert(num_groups_x > 0 && num_groups_y > 0 && num_groups_z > 0
        && "D3D12CommandBuffer::Dispatch: dispatch group counts must be greater than zero");
    assert(current_compute_pipeline_ && "D3D12CommandBuffer::Dispatch: compute pipeline must be bound before dispatch");

    BindDescriptorsCompute();
    cmd_list_->Dispatch(num_groups_x, num_groups_y, num_groups_z);
}

void D3D12CommandBuffer::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
    uint32_t first_instance)
{
    BindDescriptorsGraphics();
    cmd_list_->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

void D3D12CommandBuffer::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index,
    int32_t vertex_offset, uint32_t first_instance)
{
    BindDescriptorsGraphics();
    cmd_list_->DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void D3D12CommandBuffer::SetRenderTarget(ImagePtr color_attachment, ImagePtr depth_attachment)
{
    SetRenderTargets({color_attachment}, depth_attachment);
}

void D3D12CommandBuffer::SetRenderTargets(std::vector<ImagePtr> const& color_attachments, ImagePtr depth_attachment)
{
    assert(color_attachments.size() <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT
        && "D3D12CommandBuffer::SetRenderTargets: too many color attachments");

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles(color_attachments.size());
    for (size_t i = 0; i < color_attachments.size(); ++i)
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

    cmd_list_->OMSetRenderTargets(static_cast<UINT>(rtv_handles.size()),
        rtv_handles.empty() ? nullptr : rtv_handles.data(), FALSE, depth_attachment ? &dsv_handle : nullptr);
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

    D3D12_RECT rect = {0, 0, static_cast<LONG>(image->GetWidth()), static_cast<LONG>(image->GetHeight())};
    float color[4] = {r, g, b, a};
    cmd_list_->ClearRenderTargetView(d3d12_image->GetRTVHandle(), color, 1, &rect);
}

void D3D12CommandBuffer::ClearDepthImage(ImagePtr image, float depth)
{
    D3D12Image* d3d12_image = static_cast<D3D12Image*>(image.get());
    assert(d3d12_image && "D3D12CommandBuffer::ClearDepthImage: image is not a D3D12Image");

    D3D12_RECT rect = {0, 0, static_cast<LONG>(image->GetWidth()), static_cast<LONG>(image->GetHeight())};
    cmd_list_->ClearDepthStencilView(d3d12_image->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 1, &rect);
}

void D3D12CommandBuffer::TransitionBarrier(ImagePtr image, ImageLayout layout_before, ImageLayout layout_after)
{
    TransitionBarrier(std::vector<ImagePtr>{image}, layout_before, layout_after);
}

void D3D12CommandBuffer::TransitionBarrier(std::vector<ImagePtr> const& images, ImageLayout layout_before,
    ImageLayout layout_after)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(images.size());
    for (const auto& image : images)
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
        barriers.push_back(barrier);
    }

    if (!barriers.empty())
    {
        cmd_list_->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }
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

void D3D12CommandBuffer::StorageBarrier(BufferPtr buffer)
{
    D3D12Buffer* d3d12_buffer = static_cast<D3D12Buffer*>(buffer.get());
    assert(d3d12_buffer && "D3D12CommandBuffer::StorageBarrier: buffer is not a D3D12Buffer");

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = d3d12_buffer->GetResource();

    cmd_list_->ResourceBarrier(1, &barrier);
}

void D3D12CommandBuffer::TransitionBuffer(D3D12Buffer& buffer, D3D12_RESOURCE_STATES state_after)
{
    D3D12_RESOURCE_STATES state_before = buffer.GetCurrentState();
    if (state_before == state_after)
    {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = buffer.GetResource();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = state_before;
    barrier.Transition.StateAfter = state_after;
    cmd_list_->ResourceBarrier(1, &barrier);
    buffer.SetCurrentState(state_after);
}

void D3D12CommandBuffer::BuildBottomLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
    std::vector<AccelerationStructureGeometryDesc> const& geometries)
{
    auto* bottom_level = dynamic_cast<D3D12AccelerationStructure*>(&acceleration_structure);
    if (!bottom_level || bottom_level->GetType() != AccelerationStructureType::kBottomLevel)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::BuildBottomLevelAccelerationStructure: acceleration structure is not a D3D12 BLAS");
    }
    if (geometries.empty())
    {
        throw std::runtime_error("D3D12CommandBuffer::BuildBottomLevelAccelerationStructure: geometry list is empty");
    }

    BufferPtr scratch_buffer = device_.CreateBuffer(bottom_level->GetBuildScratchSize(), 1,
        BufferFlags::kStorage | BufferFlags::kAccelerationStructureBuildInput);
    auto* scratch = dynamic_cast<D3D12Buffer*>(scratch_buffer.get());
    staging_buffers_.push_back(scratch_buffer);

    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> d3d12_geometries;
    d3d12_geometries.reserve(geometries.size());
    for (AccelerationStructureGeometryDesc const& geometry : geometries)
    {
        auto* vertex_buffer = dynamic_cast<D3D12Buffer*>(geometry.vertex_buffer.get());
        auto* index_buffer = dynamic_cast<D3D12Buffer*>(geometry.index_buffer.get());
        if (!vertex_buffer || !index_buffer)
        {
            throw std::runtime_error(
                "D3D12CommandBuffer::BuildBottomLevelAccelerationStructure: geometry buffers are invalid");
        }

        TransitionBuffer(*vertex_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionBuffer(*index_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        d3d12_geometries.push_back(ToD3D12GeometryDesc(geometry));
    }
    TransitionBuffer(*scratch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
    build_desc.DestAccelerationStructureData = bottom_level->GetGpuAddress();
    build_desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
    build_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    build_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    build_desc.Inputs.NumDescs = static_cast<UINT>(d3d12_geometries.size());
    build_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    build_desc.Inputs.pGeometryDescs = d3d12_geometries.data();

    ComPtr<ID3D12GraphicsCommandList4> rt_command_list;
    ThrowIfFailed(cmd_list_.As(&rt_command_list));
    rt_command_list->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
    StorageBarrier(bottom_level->GetStorageBuffer());
}

void D3D12CommandBuffer::BuildTopLevelAccelerationStructure(AccelerationStructure& acceleration_structure,
    std::vector<AccelerationStructureInstanceDesc> const& instances)
{
    auto* top_level = dynamic_cast<D3D12AccelerationStructure*>(&acceleration_structure);
    if (!top_level || top_level->GetType() != AccelerationStructureType::kTopLevel)
    {
        throw std::runtime_error(
            "D3D12CommandBuffer::BuildTopLevelAccelerationStructure: acceleration structure is not a D3D12 TLAS");
    }
    if (instances.empty())
    {
        throw std::runtime_error("D3D12CommandBuffer::BuildTopLevelAccelerationStructure: instance list is empty");
    }

    BufferPtr scratch_buffer = device_.CreateBuffer(top_level->GetBuildScratchSize(), 1,
        BufferFlags::kStorage | BufferFlags::kAccelerationStructureBuildInput);
    auto* scratch = dynamic_cast<D3D12Buffer*>(scratch_buffer.get());
    staging_buffers_.push_back(scratch_buffer);

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> d3d12_instances;
    d3d12_instances.reserve(instances.size());
    for (AccelerationStructureInstanceDesc const& instance : instances)
    {
        d3d12_instances.push_back(ToD3D12InstanceDesc(instance));
    }

    uint64_t instance_data_size = d3d12_instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    BufferPtr upload_buffer = device_.CreateBuffer(instance_data_size, 1, BufferFlags::kCpuAccess);
    std::memcpy(upload_buffer->Map(), d3d12_instances.data(), instance_data_size);
    upload_buffer->Unmap();

    BufferPtr instance_buffer = device_.CreateBuffer(instance_data_size, 1,
        BufferFlags::kAccelerationStructureBuildInput);
    CopyBuffer(upload_buffer, 0, instance_buffer, 0, instance_data_size);
    staging_buffers_.push_back(upload_buffer);
    staging_buffers_.push_back(instance_buffer);

    auto* d3d12_instance_buffer = dynamic_cast<D3D12Buffer*>(instance_buffer.get());
    TransitionBuffer(*d3d12_instance_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionBuffer(*scratch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
    build_desc.DestAccelerationStructureData = top_level->GetGpuAddress();
    build_desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
    build_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    build_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    build_desc.Inputs.NumDescs = static_cast<UINT>(d3d12_instances.size());
    build_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    build_desc.Inputs.InstanceDescs = d3d12_instance_buffer->GetGPUVirtualAddress();

    ComPtr<ID3D12GraphicsCommandList4> rt_command_list;
    ThrowIfFailed(cmd_list_.As(&rt_command_list));
    rt_command_list->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
    StorageBarrier(top_level->GetStorageBuffer());
}

void D3D12CommandBuffer::CopyBuffer(BufferPtr src, uint64_t src_offset, BufferPtr dst, uint64_t dst_offset,
    uint64_t size)
{
    D3D12Buffer* d3d12_src = static_cast<D3D12Buffer*>(src.get());
    D3D12Buffer* d3d12_dst = static_cast<D3D12Buffer*>(dst.get());
    assert(d3d12_src && d3d12_dst && "D3D12CommandBuffer::CopyBuffer: src/dst must be D3D12Buffer");

    if (d3d12_src->GetCurrentState() != D3D12_RESOURCE_STATE_GENERIC_READ)
    {
        TransitionBuffer(*d3d12_src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
    TransitionBuffer(*d3d12_dst, D3D12_RESOURCE_STATE_COPY_DEST);
    cmd_list_->CopyBufferRegion(d3d12_dst->GetResource(), dst_offset, d3d12_src->GetResource(), src_offset, size);
    TransitionBuffer(*d3d12_dst, D3D12_RESOURCE_STATE_COMMON);
}

void D3D12CommandBuffer::CopyBufferToImage(ImagePtr dst, BufferPtr src)
{
    D3D12Image* d3d12_dst = static_cast<D3D12Image*>(dst.get());
    D3D12Buffer* d3d12_src = static_cast<D3D12Buffer*>(src.get());
    assert(d3d12_dst && d3d12_src &&
           "D3D12CommandBuffer::CopyBufferToImage: dst must be D3D12Image and src must be "
           "D3D12Buffer");

    D3D12_RESOURCE_DESC image_desc = d3d12_dst->GetResource()->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT num_rows = 0;
    UINT64 row_size = 0;
    UINT64 total_size = 0;
    device_.GetD3D12Device()->GetCopyableFootprints(&image_desc, 0, 1, 0, &footprint, &num_rows, &row_size,
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

void D3D12CommandBuffer::UploadImage(ImagePtr dst, void const* data, size_t data_size)
{
    D3D12Image* d3d12_dst = static_cast<D3D12Image*>(dst.get());
    assert(d3d12_dst && "D3D12CommandBuffer::UploadImage: dst must be D3D12Image");

    D3D12_RESOURCE_DESC image_desc = d3d12_dst->GetResource()->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT num_rows = 0;
    UINT64 row_size = 0;
    UINT64 total_size = 0;
    device_.GetD3D12Device()->GetCopyableFootprints(&image_desc, 0, 1, 0, &footprint, &num_rows, &row_size,
        &total_size);

    const UINT64 required_source_size = row_size * num_rows;
    if (!data || data_size < required_source_size)
    {
        throw std::runtime_error("D3D12CommandBuffer::UploadImage: source image data is too small");
    }

    BufferPtr staging_buffer = device_.CreateBuffer(total_size, 1, BufferFlags::kCpuAccess);
    uint8_t* dst_data = static_cast<uint8_t*>(staging_buffer->Map());
    uint8_t const* src_data = static_cast<uint8_t const*>(data);
    for (UINT row = 0; row < num_rows; ++row)
    {
        std::memcpy(dst_data + footprint.Offset + footprint.Footprint.RowPitch * row, src_data + row_size * row,
            static_cast<size_t>(row_size));
    }
    staging_buffer->Unmap();

    D3D12Buffer* d3d12_src = static_cast<D3D12Buffer*>(staging_buffer.get());

    D3D12_TEXTURE_COPY_LOCATION dst_location = {};
    dst_location.pResource = d3d12_dst->GetResource();
    dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst_location.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src_location = {};
    src_location.pResource = d3d12_src->GetResource();
    src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src_location.PlacedFootprint = footprint;

    cmd_list_->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
    staging_buffers_.push_back(std::move(staging_buffer));
}

void D3D12CommandBuffer::CopyImage(ImagePtr dst, ImagePtr src)
{
    D3D12Image* d3d12_dst = static_cast<D3D12Image*>(dst.get());
    D3D12Image* d3d12_src = static_cast<D3D12Image*>(src.get());
    assert(d3d12_dst && d3d12_src && "D3D12CommandBuffer::CopyImage: dst/src must be D3D12Image");
    assert(dst->GetWidth() == src->GetWidth()
        && "D3D12CommandBuffer::CopyImage: source and destination widths must match");
    assert(dst->GetHeight() == src->GetHeight()
        && "D3D12CommandBuffer::CopyImage: source and destination heights must match");
    assert(dst->GetFormat() == src->GetFormat()
        && "D3D12CommandBuffer::CopyImage: source and destination formats must match");

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

void D3D12CommandBuffer::BindDescriptorHeaps()
{
    if (descriptor_heaps_bound_)
    {
        return;
    }

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    ID3D12DescriptorHeap* heaps[] = {descriptor_manager.GetGPUCBVSRVUAVHeap(), descriptor_manager.GetGPUSamplerHeap()};
    cmd_list_->SetDescriptorHeaps(2, heaps);
    descriptor_heaps_bound_ = true;
}

void D3D12CommandBuffer::BindDescriptorsGraphics()
{
    if (!current_graphics_pipeline_)
    {
        return;
    }

    BindDescriptorHeaps();

    if (!current_descriptor_set_)
    {
        return;
    }

    assert(&current_descriptor_set_->GetLayout() == &current_graphics_pipeline_->GetLayout() &&
           "D3D12CommandBuffer::BindDescriptorsGraphics: descriptor set layout must match the "
           "current graphics pipeline layout");

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    for (D3D12DescriptorSet::BoundDescriptor const& descriptor : current_descriptor_set_->GetBoundDescriptors())
    {
        assert(!descriptor.gpu_descriptors.empty()
            && "D3D12CommandBuffer::BindDescriptorsGraphics: descriptor set has no GPU descriptors");
        cmd_list_->SetGraphicsRootDescriptorTable(descriptor.root_parameter_index,
            descriptor_manager.GetGPU(descriptor.gpu_descriptors.front()));
    }
}

void D3D12CommandBuffer::BindDescriptorsCompute()
{
    if (!current_compute_pipeline_)
    {
        return;
    }

    BindDescriptorHeaps();

    if (!current_descriptor_set_)
    {
        return;
    }

    assert(&current_descriptor_set_->GetLayout() == &current_compute_pipeline_->GetLayout() &&
           "D3D12CommandBuffer::BindDescriptorsCompute: descriptor set layout must match the "
           "current compute pipeline layout");

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    for (D3D12DescriptorSet::BoundDescriptor const& descriptor : current_descriptor_set_->GetBoundDescriptors())
    {
        assert(!descriptor.gpu_descriptors.empty()
            && "D3D12CommandBuffer::BindDescriptorsCompute: descriptor set has no GPU descriptors");
        cmd_list_->SetComputeRootDescriptorTable(descriptor.root_parameter_index,
            descriptor_manager.GetGPU(descriptor.gpu_descriptors.front()));
    }
}

}  // namespace gpu
