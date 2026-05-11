#include "d3d12_api.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

#include <dxgi1_6.h>

#include <algorithm>
#include <stdexcept>

namespace gpu
{
namespace
{
bool SupportsD3D12(IDXGIAdapter1* adapter)
{
    return SUCCEEDED(
        D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr));
}

bool IsHardwareAdapter(IDXGIAdapter1* adapter)
{
    DXGI_ADAPTER_DESC1 adapter_desc = {};
    ThrowIfFailed(adapter->GetDesc1(&adapter_desc));
    return (adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
}

std::uint64_t GetDedicatedVideoMemory(IDXGIAdapter1* adapter)
{
    DXGI_ADAPTER_DESC1 adapter_desc = {};
    ThrowIfFailed(adapter->GetDesc1(&adapter_desc));
    return static_cast<std::uint64_t>(adapter_desc.DedicatedVideoMemory);
}

void AddAdapterIfUsable(std::vector<ComPtr<IDXGIAdapter1>>& adapters, IDXGIAdapter1* adapter)
{
    if (!adapter || !IsHardwareAdapter(adapter) || !SupportsD3D12(adapter))
    {
        return;
    }

    adapters.emplace_back(adapter);
}
} // namespace

D3D12Api::D3D12Api() : shader_manager_("")
{
    UINT dxgi_factory_flags = 0;

#ifndef _NDEBUG
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
        {
            debug_controller->EnableDebugLayer();

            // Enable additional debug layers.
            dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory_)));

    ComPtr<IDXGIFactory6> dxgi_factory6;
    if (SUCCEEDED(dxgi_factory_.As(&dxgi_factory6)))
    {
        for (uint32_t adapter_idx = 0;; ++adapter_idx)
        {
            ComPtr<IDXGIAdapter1> dxgi_adapter;
            HRESULT status = dxgi_factory6->EnumAdapterByGpuPreference(
                adapter_idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgi_adapter));
            if (status == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }

            ThrowIfFailed(status);
            AddAdapterIfUsable(dxgi_adapters_, dxgi_adapter.Get());
        }
    }
    else
    {
        for (uint32_t adapter_idx = 0;; ++adapter_idx)
        {
            ComPtr<IDXGIAdapter1> dxgi_adapter;
            HRESULT status = dxgi_factory_->EnumAdapters1(adapter_idx, &dxgi_adapter);
            if (status == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }

            ThrowIfFailed(status);
            AddAdapterIfUsable(dxgi_adapters_, dxgi_adapter.Get());
        }

        std::sort(dxgi_adapters_.begin(), dxgi_adapters_.end(),
            [](ComPtr<IDXGIAdapter1> const& lhs, ComPtr<IDXGIAdapter1> const& rhs) {
                return GetDedicatedVideoMemory(lhs.Get()) > GetDedicatedVideoMemory(rhs.Get());
            });
    }

    if (dxgi_adapters_.empty())
    {
        ComPtr<IDXGIAdapter> warp_adapter;
        ThrowIfFailed(dxgi_factory_->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)));

        ComPtr<IDXGIAdapter1> warp_adapter1;
        ThrowIfFailed(warp_adapter.As(&warp_adapter1));

        if (SupportsD3D12(warp_adapter1.Get()))
        {
            dxgi_adapters_.push_back(warp_adapter1);
        }
    }
}

DevicePtr D3D12Api::CreateDevice()
{
    if (dxgi_adapters_.empty())
    {
        throw std::runtime_error("No D3D12-capable DXGI adapters found");
    }

    return std::make_unique<D3D12Device>(*this, dxgi_adapters_.front().Get());
}

} // namespace gpu
