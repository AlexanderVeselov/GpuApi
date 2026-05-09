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
    D3D12Image(D3D12Device& device, uint32_t width, uint32_t height, ImageFormat format);

    D3D12Image(
        D3D12Device& device,
        ID3D12Resource* resource,
        uint32_t width,
        uint32_t height,
        ImageFormat format);

    D3D12Image(
        D3D12Device& device,
        uint32_t width,
        uint32_t height,
        ImageFormat format,
        uint32_t mip_count,
        uint32_t array_size,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES initial_state);

    ~D3D12Image() override;

    ID3D12Resource* GetResource() const
    {
        return resource_.Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();

    D3D12Descriptor const& GetView(ImageView const& view);

    D3D12_RESOURCE_STATES GetCurrentState() const
    {
        return current_state_;
    }

    void SetCurrentState(D3D12_RESOURCE_STATES state)
    {
        current_state_ = state;
    }

private:
    D3D12Descriptor CreateView(const ImageView& view);

private:
    D3D12Device& device_;

    ComPtr<ID3D12Resource> resource_;

    uint32_t mip_count_ = 1;
    uint32_t array_size_ = 1;

    D3D12_RESOURCE_FLAGS flags_;
    D3D12_RESOURCE_STATES current_state_;

    std::unordered_map<
        ImageView,
        D3D12Descriptor,
        ImageViewHash> views_;

    D3D12Descriptor default_rtv_;
    D3D12Descriptor dsv_;
};

} // namespace gpu
