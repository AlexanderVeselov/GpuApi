#ifndef D3D12_SWAPCHAIN_HPP_
#define D3D12_SWAPCHAIN_HPP_

#include "gpu_swapchain.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
    class D3D12Device;

    class D3D12Swapchain : public Swapchain
    {
    public:
        D3D12Swapchain(D3D12Device& device, HWND hwnd, std::uint32_t width, std::uint32_t height);
        void Present() override;
        std::uint32_t GetImagesCount() const override { return images_count_; }

    private:
        D3D12Device& device_;
        ComPtr<IDXGISwapChain1> swapchain_;
        std::uint32_t images_count_;

    };

} // namespace gpu

#endif // D3D12_SWAPCHAIN_HPP_
