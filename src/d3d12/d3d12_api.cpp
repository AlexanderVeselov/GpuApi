#include "d3d12_api.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_device.hpp"

#include <stdexcept>

namespace gpu
{
    D3D12Api::D3D12Api()
        : shader_manager_("")
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

        IDXGIAdapter1* dxgi_adapter;
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
            if (SUCCEEDED(D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                dxgi_adapters_.push_back(dxgi_adapter);
            }
        }
    }

    DevicePtr D3D12Api::CreateDevice()
    {
        if (dxgi_adapters_.empty())
        {
            throw std::runtime_error("No DXGI adapters found");
        }

        ///@NOTE: this is my GTX 1050 Ti
        return std::make_unique<D3D12Device>(*this, dxgi_adapters_[1]);
    }

} // namespace gpu
