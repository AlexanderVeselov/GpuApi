#include "d3d12_image.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

namespace gpu
{
    D3D12Image::D3D12Image(D3D12Device& device, std::uint32_t width, std::uint32_t height)
        : Image(width, height)
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
        resource_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resource_desc.SampleDesc = { 1, 0 };
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        ///@TODO: make it configurable!!!
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        ThrowIfFailed(d3d12_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, D3D12_RESOURCE_STATE_COMMON, &clear_value, IID_PPV_ARGS(&resource_)));

    }

    D3D12Image::D3D12Image(D3D12Device& device, ID3D12Resource* resource,
        std::uint32_t width, std::uint32_t height)
        : Image(width, height)
        , resource_(resource)
    {

    }

} // namespace gpu
