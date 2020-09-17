#ifndef D3D12_SWAPCHAIN_HPP_
#define D3D12_SWAPCHAIN_HPP_

#include "gpu_swapchain.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
    class Device;

    class D3D12Swapchain : public Swapchain
    {
    public:
        D3D12Swapchain(Device& device, HWND hwnd);
        void Present() override;

    private:
        Device& device_;
        ComPtr<IDXGISwapChain> swapchain_;
    };

} // namespace gpu

#endif // D3D12_SWAPCHAIN_HPP_
