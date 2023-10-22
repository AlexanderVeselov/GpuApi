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
        D3D12Image(D3D12Device& device, std::uint32_t width, std::uint32_t height, ImageFormat format);
        D3D12Image(D3D12Device& device, ID3D12Resource* resource,
            std::uint32_t width, std::uint32_t height, ImageFormat format);

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();
        //D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle();
        ID3D12Resource* GetResource() const { return resource_.Get(); }
        DXGI_FORMAT GetDXGIFormat() const;

    private:
        ComPtr<ID3D12Resource> resource_;
        D3D12Device& device_;
        std::vector<std::uint32_t> srv_descriptors_;
        std::vector<std::uint32_t> uav_descriptors_;
        std::uint32_t default_rtv_ = kNullDescriptor;
    };

} // namespace gpu

#endif // D3D12_IMAGE_HPP_
