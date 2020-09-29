#include "d3d12_image.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include <cassert>

namespace gpu
{
    namespace
    {
        DXGI_FORMAT ImageToDXGIFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::kUnknown:
                return DXGI_FORMAT_UNKNOWN;
            case ImageFormat::kRGBA32_Float:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case ImageFormat::kRGBA16_Float:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case ImageFormat::kRGBA8_SInt:
                return DXGI_FORMAT_R8G8B8A8_SINT;
            case ImageFormat::kRGBA8_UInt:
                return DXGI_FORMAT_R8G8B8A8_UINT;
            case ImageFormat::kRGBA8_UNorm:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::kRGBA8_SRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case ImageFormat::kRG32_Float:
                return DXGI_FORMAT_R32G32_FLOAT;
            case ImageFormat::kRG16_Float:
                return DXGI_FORMAT_R16G16_FLOAT;
            case ImageFormat::kR32_Float:
                return DXGI_FORMAT_R32_FLOAT;
            case ImageFormat::kR16_Float:
                return DXGI_FORMAT_R16_FLOAT;
            default:
                assert(!"ImageToDXGIFormat: Unknown image format");
                return DXGI_FORMAT_UNKNOWN;
            }
        }
    }

    D3D12Image::D3D12Image(D3D12Device& device, std::uint32_t width, std::uint32_t height,
        ImageFormat format)
        : Image(width, height, format)
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

        // Create default RTV
        rtv_handle_ = device.GetRTVDescManager().AllocateDescriptor();
        device.GetD3D12Device()->CreateRenderTargetView(resource_.Get(), nullptr, rtv_handle_);

    }

    D3D12Image::D3D12Image(D3D12Device& device, ID3D12Resource* resource,
        std::uint32_t width, std::uint32_t height, ImageFormat format)
        : Image(width, height, format)
        , resource_(resource)
    {
        // Create default RTV
        rtv_handle_ = device.GetRTVDescManager().AllocateDescriptor();
        device.GetD3D12Device()->CreateRenderTargetView(resource_.Get(), nullptr, rtv_handle_);
    }

} // namespace gpu
