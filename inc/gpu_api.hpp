#pragma once

#include "gpu_types.hpp"

namespace gpu
{
/// Selects the backend implementation used by the RHI.
enum class ApiType
{
    kD3D12,
    kVulkan,
    kCount
};

/// Entry point for creating backend devices.
class Api
{
  public:
    virtual ~Api() = default;

    /// Creates an API object for the selected backend. The returned object is owned by the caller.
    static Api* Create(ApiType api_type);

    /// Creates a logical GPU device for this backend.
    virtual DevicePtr CreateDevice() = 0;

    /// Sets the root directory used to load shaders and resolve shader includes.
    virtual void SetShaderPath(char const* shader_path) = 0;
};

} // namespace gpu
