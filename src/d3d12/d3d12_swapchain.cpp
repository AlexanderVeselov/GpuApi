#include "d3d12_swapchain.hpp"
#include "d3d12_api.hpp"
#include "d3d12_device.hpp"
#include "d3d12_queue.hpp"
#include "d3d12_exception.hpp"

namespace gpu
{
    D3D12Swapchain::D3D12Swapchain(D3D12Device& device, HWND hwnd,
        std::uint32_t width, std::uint32_t height)
        : device_(device)
        , images_count_(3) ///@TODO: configurable
    {
        auto& api = device_.GetD3D12Api();
        auto dxgi_factory = api.GetDXGIFactory();

        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.Width = width;
        swapchain_desc.Height = height;
        swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.SampleDesc = { 1, 0 };
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = images_count_;
        swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        Queue& gfx_queue = device_.GetQueue(QueueType::kGraphics);
        D3D12Queue& d3d12_queue = static_cast<D3D12Queue&>(gfx_queue);

        ThrowIfFailed(dxgi_factory->CreateSwapChainForHwnd(d3d12_queue.GetQueue(), hwnd,
            &swapchain_desc, nullptr, nullptr, &swapchain_));

        //swapchain_->GetBuffer()

    }

    void D3D12Swapchain::Present()
    {
        ThrowIfFailed(swapchain_->Present(1, 0));
    }

}
