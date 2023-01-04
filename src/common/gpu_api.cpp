#include "gpu_api.hpp"
#include "../d3d12/d3d12_api.hpp"

#include <cassert>
#include <stdexcept>

namespace gpu
{
Api* Api::Create(ApiType api_type)
{
    switch (api_type)
    {
    case ApiType::kD3D12:
        return new D3D12Api();
    default:
        assert(!"Not implemented!");
        throw std::runtime_error("Failted to create Gpu API: API type is not supported");
    }

    return nullptr;
}
} // namespace gpu
