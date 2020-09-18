#ifndef D3D12_IMAGE_HPP_
#define D3D12_IMAGE_HPP_

#include "gpu_image.hpp"
#include "d3d12_common.hpp"
#include <cstdint>

namespace gpu
{
    class D3D12Device;

    class D3D12Image : public Image
    {
    public:
        D3D12Image(D3D12Device& device, std::uint32_t width, std::uint32_t height);
        D3D12Image(D3D12Device& device, ID3D12Resource* resource,
            std::uint32_t width, std::uint32_t height);

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return rtv_handle_; }

    private:
        ComPtr<ID3D12Resource> resource_;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle_ = {};

    };
} // namespace gpu

#endif // D3D12_IMAGE_HPP_
