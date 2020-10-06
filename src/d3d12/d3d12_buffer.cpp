#include "d3d12_buffer.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

namespace gpu
{
    D3D12Buffer::D3D12Buffer(D3D12Device& device, std::size_t size)
        : Buffer(size)
    {
        auto d3d12_device = device.GetD3D12Device();

        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;//D3D12_HEAP_TYPE_DEFAULT; ///@TODO: staging buffer support
        heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment = 0;
        resource_desc.Width = size_;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc = { 1, 0 };
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ///@TODO: make it configurable!!!
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;// D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ThrowIfFailed(d3d12_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource_)));
    }

    void* D3D12Buffer::Map()
    {
        D3D12_RANGE read_range = {};
        ///@TODO: map part of the resource
        void* out_ptr;
        ThrowIfFailed(resource_->Map(0, &read_range, &out_ptr));
        return out_ptr;
    }

    void D3D12Buffer::Unmap()
    {
        ///@TODO: unmap part of the resource
        resource_->Unmap(0, nullptr);
    }

}