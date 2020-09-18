#ifndef D3D12_API_HPP_
#define D3D12_API_HPP_

#include "gpu_api.hpp"
#include "d3d12_common.hpp"

#include <vector>

namespace gpu
{
    using Microsoft::WRL::ComPtr;

    class D3D12Api : public Api
    {
    public:
        D3D12Api();
        DevicePtr CreateDevice() override;

        IDXGIFactory4* GetDXGIFactory() { return dxgi_factory_.Get(); }

    private:
        ComPtr<IDXGIFactory4> dxgi_factory_;
        std::vector<IDXGIAdapter1*> dxgi_adapters_;
    };

} // namespace gpu

#endif // D3D12_API_HPP_
