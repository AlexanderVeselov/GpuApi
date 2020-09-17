#include "d3d12_swapchain.hpp"

namespace gpu
{
    D3D12Swapchain::D3D12Swapchain(Device& device, HWND hwnd)
        : device_(device)
    {

    }

    void D3D12Swapchain::Present()
    {
        swapchain_->Present(1, 0);
    }

}
