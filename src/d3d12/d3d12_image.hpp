#pragma once

#include "gpu_image.hpp"
#include "d3d12_common.hpp"
#include "d3d12_descriptor_manager.hpp"
#include <cstdint>
#include <unordered_map>

namespace gpu
{
class D3D12Device;

class D3D12Image final : public Image
{
public:
    D3D12Image(
        D3D12Device& device,
        uint32_t width,
        uint32_t height,
        ImageFormat format,
        uint32_t mip_count,
        uint32_t array_size,
        ImageFlags flags);

    D3D12Image(
        D3D12Device& device,
        ID3D12Resource* resource,
        uint32_t width,
        uint32_t height,
        ImageFormat format,
        uint32_t mip_count,
        uint32_t array_size,
        ImageFlags flags);

    ~D3D12Image() override;

    ID3D12Resource* GetResource() const
    {
        return resource_.Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();

    D3D12Descriptor const& GetView(ImageView const& view);
    D3D12Descriptor const& GetUAV(ImageView const& view);

private:
    D3D12Descriptor CreateSRV(ImageView const& view);
    D3D12Descriptor CreateUAV(ImageView const& view);

private:
    D3D12Device& device_;

    ComPtr<ID3D12Resource> resource_;

    std::unordered_map<ImageView, D3D12Descriptor, ImageViewHash> srvs_;
    std::unordered_map<ImageView, D3D12Descriptor, ImageViewHash> uavs_;

    D3D12Descriptor default_rtv_;
    D3D12Descriptor dsv_;
};

} // namespace gpu
