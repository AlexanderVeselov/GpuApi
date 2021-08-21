#ifndef D3D12_SWAPCHAIN_HPP_
#define D3D12_SWAPCHAIN_HPP_

#include "gpu_swapchain.hpp"
#include "d3d12_common.hpp"
#include "gpu_types.hpp"

namespace gpu
{
    class D3D12Device;

    class D3D12Swapchain : public Swapchain
    {
    public:
        D3D12Swapchain(D3D12Device& device, void* window_native_handle, std::uint32_t width, std::uint32_t height);
        void Present() override;

    private:
        D3D12Device& device_;
        ComPtr<IDXGISwapChain1> swapchain_;


    };

} // namespace gpu

#endif // D3D12_SWAPCHAIN_HPP_
