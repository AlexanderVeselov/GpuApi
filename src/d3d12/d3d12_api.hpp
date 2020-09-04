#ifndef D3D12_API_HPP_
#define D3D12_API_HPP_

#include "gpu_api.hpp"

namespace gpu
{
    class D3D12Api : public Api
    {
    public:
        virtual DevicePtr CreateDevice() override;

    };

} // namespace gpu

#endif // D3D12_API_HPP_
