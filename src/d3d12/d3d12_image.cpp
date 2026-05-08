#include "d3d12_image.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include <cassert>

namespace gpu
{
D3D12Image::D3D12Image(D3D12Device& device, std::uint32_t width, std::uint32_t height,
    ImageFormat format)
    : Image(width, height, format)
    , device_(device)
{
    auto d3d12_device = device.GetD3D12Device();

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resource_desc = {};
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = width;
    resource_desc.Height = height;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = ImageToDXGIFormat(format);
    resource_desc.SampleDesc = { 1, 0 };
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ///@TODO: make it configurable!!!
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = resource_desc.Format;

    ThrowIfFailed(d3d12_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE,
        &resource_desc, D3D12_RESOURCE_STATE_COMMON, &clear_value, IID_PPV_ARGS(&resource_)));
}

D3D12Image::D3D12Image(D3D12Device& device, ID3D12Resource* resource,
    std::uint32_t width, std::uint32_t height, ImageFormat format)
    : Image(width, height, format)
    , resource_(resource)
    , device_(device)
{
}

DXGI_FORMAT D3D12Image::GetDXGIFormat() const
{
    return ImageToDXGIFormat(format_);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Image::GetRTVHandle()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;
    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();

    if (!default_rtv_.IsValid())
    {
        default_rtv_ = descriptor_manager.AllocateCPURTV();
        rtv_handle = descriptor_manager.GetCPU(default_rtv_);

        DXGI_FORMAT dxgi_format = GetDXGIFormat();

        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = (dxgi_format == DXGI_FORMAT_R32_TYPELESS) ? DXGI_FORMAT_R32_FLOAT : dxgi_format;
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv_desc.Texture2D.MipSlice = 0;
        rtv_desc.Texture2D.PlaneSlice = 0;
        device_.GetD3D12Device()->CreateRenderTargetView(resource_.Get(), &rtv_desc, rtv_handle);
        return rtv_handle;
    }
    else
    {
        return descriptor_manager.GetCPU(default_rtv_);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Image::GetDSVHandle()
{
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle;
    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    if (!dsv_.IsValid())
    {
        DXGI_FORMAT dxgi_format = GetDXGIFormat();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        // Special case for typeless format
        dsv_desc.Format = (dxgi_format == DXGI_FORMAT_R32_TYPELESS) ? DXGI_FORMAT_D32_FLOAT : dxgi_format;
        dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = 0;

        dsv_ = descriptor_manager.AllocateCPUDSV();
        dsv_handle = descriptor_manager.GetCPU(dsv_);
        device_.GetD3D12Device()->CreateDepthStencilView(resource_.Get(), &dsv_desc, dsv_handle);
        return dsv_handle;
    }
    else
    {
        return descriptor_manager.GetCPU(dsv_);
    }
}

//D3D12_GPU_DESCRIPTOR_HANDLE D3D12Image::GetSRVHandle()
//{
//
//}

} // namespace gpu
