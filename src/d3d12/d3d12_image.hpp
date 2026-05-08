#pragma once

#include "gpu_image.hpp"
#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include <cstdint>

namespace gpu
{
class D3D12Device;

class D3D12Image : public Image
{
public:
    D3D12Image(D3D12Device& device, std::uint32_t width, std::uint32_t height, ImageFormat format);
    D3D12Image(D3D12Device& device, ID3D12Resource* resource,
        std::uint32_t width, std::uint32_t height, ImageFormat format);

    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();
    //D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle();
    ID3D12Resource* GetResource() const { return resource_.Get(); }
    DXGI_FORMAT GetDXGIFormat() const;

private:
    ComPtr<ID3D12Resource> resource_;
    D3D12Device& device_;
    std::vector<D3D12Descriptor> srv_descriptors_;
    std::vector<D3D12Descriptor> uav_descriptors_;
    D3D12Descriptor default_rtv_;
    D3D12Descriptor dsv_;
};

} // namespace gpu
