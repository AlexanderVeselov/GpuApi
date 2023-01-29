#pragma once

#include "gpu_types.hpp"

namespace gpu
{
enum class ApiType
{
    kD3D12,
    kVulkan,
    kCount
};

class Api
{
public:
    static Api* Create(ApiType api_type);
    virtual DevicePtr CreateDevice() = 0;

};

} // namespace gpu
