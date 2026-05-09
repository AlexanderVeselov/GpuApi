#include "d3d12_image.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

#include <cassert>

namespace gpu
{
namespace
{
D3D12_RESOURCE_DESC CreateTexture2DDesc(
    uint32_t width,
    uint32_t height,
    ImageFormat format,
    uint32_t mip_count,
    uint32_t array_size,
    D3D12_RESOURCE_FLAGS flags)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = static_cast<UINT16>(array_size);
    desc.MipLevels = static_cast<UINT16>(mip_count);
    desc.Format = ImageToDXGIFormat(format);
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = flags;
    return desc;
}

DXGI_FORMAT GetRTVFormat(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_R32_FLOAT : format;
}

DXGI_FORMAT GetDSVFormat(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_D32_FLOAT : format;
}

DXGI_FORMAT GetSRVFormat(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_R32_FLOAT : format;
}

bool IsDepthFormat(ImageFormat format)
{
    return format == ImageFormat::kD32_Float || format == ImageFormat::kR32_Typeless;
}
}

D3D12Image::D3D12Image(D3D12Device& device, uint32_t width, uint32_t height, ImageFormat format)
    : D3D12Image(
        device,
        width,
        height,
        format,
        1,
        1,
        IsDepthFormat(format)
            ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
            : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON)
{
}

D3D12Image::D3D12Image(
    D3D12Device& device,
    ID3D12Resource* resource,
    uint32_t width,
    uint32_t height,
    ImageFormat format)
    : Image(width, height, format)
    , device_(device)
    , resource_(resource)
    , flags_(D3D12_RESOURCE_FLAG_NONE)
    , current_state_(D3D12_RESOURCE_STATE_PRESENT)
{
    assert(resource_ && "D3D12Image: wrapped resource must not be null");

    D3D12_RESOURCE_DESC desc = resource_->GetDesc();
    mip_count_ = desc.MipLevels;
    array_size_ = desc.DepthOrArraySize;
    flags_ = desc.Flags;
}

D3D12Image::D3D12Image(
    D3D12Device& device,
    uint32_t width,
    uint32_t height,
    ImageFormat format,
    uint32_t mip_count,
    uint32_t array_size,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initial_state)
    : Image(width, height, format)
    , device_(device)
    , mip_count_(mip_count)
    , array_size_(array_size)
    , flags_(flags)
    , current_state_(initial_state)
{
    auto d3d12_device = device.GetD3D12Device();

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resource_desc = CreateTexture2DDesc(
        width,
        height,
        format,
        mip_count_,
        array_size_,
        flags_);

    DXGI_FORMAT dxgi_format = ImageToDXGIFormat(format);
    D3D12_CLEAR_VALUE clear_value = {};
    D3D12_CLEAR_VALUE* clear_value_ptr = nullptr;

    if (flags_ & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        clear_value.Format = GetDSVFormat(dxgi_format);
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0;
        clear_value_ptr = &clear_value;
    }
    else if (flags_ & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        clear_value.Format = GetRTVFormat(dxgi_format);
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.0f;
        clear_value.Color[2] = 0.0f;
        clear_value.Color[3] = 0.0f;
        clear_value_ptr = &clear_value;
    }

    ThrowIfFailed(d3d12_device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        initial_state,
        clear_value_ptr,
        IID_PPV_ARGS(&resource_)));
}

D3D12Image::~D3D12Image()
{
    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();

    descriptor_manager.Free(default_rtv_);
    descriptor_manager.Free(dsv_);

    for (auto& view : views_)
    {
        descriptor_manager.Free(view.second);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Image::GetRTVHandle()
{
    assert((flags_ & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) &&
        "D3D12Image::GetRTVHandle: image was not created with render-target support");

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    if (!default_rtv_.IsValid())
    {
        default_rtv_ = descriptor_manager.AllocateCPURTV();

        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = GetRTVFormat(ImageToDXGIFormat(format_));
        rtv_desc.ViewDimension = array_size_ > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2D;
        if (array_size_ > 1)
        {
            rtv_desc.Texture2DArray.MipSlice = 0;
            rtv_desc.Texture2DArray.FirstArraySlice = 0;
            rtv_desc.Texture2DArray.ArraySize = array_size_;
        }
        else
        {
            rtv_desc.Texture2D.MipSlice = 0;
            rtv_desc.Texture2D.PlaneSlice = 0;
        }

        device_.GetD3D12Device()->CreateRenderTargetView(
            resource_.Get(),
            &rtv_desc,
            descriptor_manager.GetCPU(default_rtv_));
    }

    return descriptor_manager.GetCPU(default_rtv_);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Image::GetDSVHandle()
{
    assert((flags_ & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) &&
        "D3D12Image::GetDSVHandle: image was not created with depth-stencil support");

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    if (!dsv_.IsValid())
    {
        dsv_ = descriptor_manager.AllocateCPUDSV();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = GetDSVFormat(ImageToDXGIFormat(format_));
        dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
        dsv_desc.ViewDimension = array_size_ > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;
        if (array_size_ > 1)
        {
            dsv_desc.Texture2DArray.MipSlice = 0;
            dsv_desc.Texture2DArray.FirstArraySlice = 0;
            dsv_desc.Texture2DArray.ArraySize = array_size_;
        }
        else
        {
            dsv_desc.Texture2D.MipSlice = 0;
        }

        device_.GetD3D12Device()->CreateDepthStencilView(
            resource_.Get(),
            &dsv_desc,
            descriptor_manager.GetCPU(dsv_));
    }

    return descriptor_manager.GetCPU(dsv_);
}

D3D12Descriptor const& D3D12Image::GetView(ImageView const& view)
{
    auto it = views_.find(view);
    if (it != views_.end())
    {
        return it->second;
    }

    auto result = views_.emplace(view, CreateView(view));
    return result.first->second;
}

D3D12Descriptor D3D12Image::CreateView(ImageView const& view)
{
    assert(view.mip < mip_count_ && "D3D12Image::CreateView: base mip is out of range");
    assert(view.mip_count > 0 && "D3D12Image::CreateView: mip count must be greater than zero");
    assert(view.mip + view.mip_count <= mip_count_ && "D3D12Image::CreateView: mip range exceeds image mip count");

    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    D3D12Descriptor descriptor = descriptor_manager.AllocateCPUCBVSRVUAV();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = GetSRVFormat(ImageToDXGIFormat(format_));
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension = array_size_ > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;
    if (array_size_ > 1)
    {
        srv_desc.Texture2DArray.MostDetailedMip = view.mip;
        srv_desc.Texture2DArray.MipLevels = view.mip_count;
        srv_desc.Texture2DArray.FirstArraySlice = 0;
        srv_desc.Texture2DArray.ArraySize = array_size_;
        srv_desc.Texture2DArray.PlaneSlice = 0;
        srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        srv_desc.Texture2D.MostDetailedMip = view.mip;
        srv_desc.Texture2D.MipLevels = view.mip_count;
        srv_desc.Texture2D.PlaneSlice = 0;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
    }

    device_.GetD3D12Device()->CreateShaderResourceView(
        resource_.Get(),
        &srv_desc,
        descriptor_manager.GetCPU(descriptor));

    return descriptor;
}

} // namespace gpu
