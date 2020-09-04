#include "d3d12_api.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_device.hpp"

#include <stdexcept>

namespace gpu
{
    D3D12Api::D3D12Api()
    {
        UINT dxgi_factory_flags = 0;

        ///@TODO: add debug interface

        ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory_)));

        ComPtr<IDXGIAdapter1> dxgi_adapter;
        for (std::uint32_t adapter_idx = 0;
            dxgi_factory_->EnumAdapters1(adapter_idx, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND; ++adapter_idx)
        {
            DXGI_ADAPTER_DESC1 adapter_desc;
            ThrowIfFailed(dxgi_adapter->GetDesc1(&adapter_desc));

            if (adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create
            // the actual device yet.
            ComPtr<ID3D12Device> test_device;
            if (SUCCEEDED(D3D12CreateDevice(dxgi_adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                dxgi_adapters_.push_back(dxgi_adapter.Get());
            }
        }
    }

    DevicePtr D3D12Api::CreateDevice()
    {
        if (dxgi_adapters_.empty())
        {
            throw std::runtime_error("No DXGI adapters found");
        }

        return std::make_unique<D3D12Device>(*this, dxgi_adapters_[0]);
    }

} // namespace gpu
