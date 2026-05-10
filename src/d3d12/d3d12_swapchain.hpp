#pragma once

#include "gpu_swapchain.hpp"
#include "d3d12_common.hpp"
#include "gpu_types.hpp"

namespace gpu
{
class D3D12Device;

class D3D12Swapchain : public Swapchain
{
public:
    D3D12Swapchain(D3D12Device& device, void* window_native_handle,
        uint32_t width, uint32_t height, uint32_t image_count);
    void Present() override;

private:
    D3D12Device& device_;
    ComPtr<IDXGISwapChain1> swapchain_;
};

} // namespace gpu
