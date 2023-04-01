#ifndef D3D12_API_HPP_
#define D3D12_API_HPP_

#include "gpu_api.hpp"
#include "d3d12_common.hpp"
#include "d3d12_shader_manager.hpp"

#include <vector>

namespace gpu
{
    using Microsoft::WRL::ComPtr;

    class D3D12Api : public Api
    {
    public:
        D3D12Api();
        DevicePtr CreateDevice() override;

        IDXGIFactory4* GetDXGIFactory() const { return dxgi_factory_.Get(); }
        D3D12ShaderManager& GetShaderManager() { return shader_manager_; }

    private:
        ComPtr<IDXGIFactory4> dxgi_factory_;
        std::vector<IDXGIAdapter1*> dxgi_adapters_;
        D3D12ShaderManager shader_manager_;
    };

} // namespace gpu

#endif // D3D12_API_HPP_
