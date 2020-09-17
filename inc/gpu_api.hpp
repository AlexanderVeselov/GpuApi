#ifndef GPU_API_HPP_
#define GPU_API_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class Api
    {
    public:
        static Api* CreateD3D12Api();
        virtual DevicePtr CreateDevice() = 0;

    };

} // namespace gpu

#endif // GPU_API_HPP_
