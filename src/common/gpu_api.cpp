#include "gpu_api.hpp"
#include "../d3d12/d3d12_api.hpp"
#if GPU_API_HAS_VULKAN
#include "../vulkan/vulkan_api.hpp"
#endif

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
    case ApiType::kVulkan:
#if GPU_API_HAS_VULKAN
        return new VulkanApi();
#else
        throw std::runtime_error("Failed to create Gpu API: Vulkan backend was not built");
#endif
    default:
        assert(!"Not implemented!");
        throw std::runtime_error("Failted to create Gpu API: API type is not supported");
    }

    return nullptr;
}
}  // namespace gpu
