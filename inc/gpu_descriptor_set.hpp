#pragma once

#include "gpu_buffer.hpp"
#include "gpu_image.hpp"
#include "gpu_sampler.hpp"

#include <cstdint>

namespace gpu
{
/// CPU-side collection of resource bindings compatible with a pipeline.
class DescriptorSet
{
public:
    virtual ~DescriptorSet() = default;

    /// Binds a buffer to a shader register and register space.
    virtual void BindBuffer(Buffer& buffer, uint32_t binding, uint32_t space = 0) = 0;

    /// Binds the image default view to a shader register and register space.
    virtual void BindImage(Image& image, uint32_t binding, uint32_t space = 0) = 0;

    /// Binds a specific image view to a shader register and register space.
    virtual void BindImage(Image& image, ImageView const& view, uint32_t binding, uint32_t space = 0) = 0;

    /// Binds a sampler to a shader register and register space.
    virtual void BindSampler(Sampler& sampler, uint32_t binding, uint32_t space = 0) = 0;

    /// Removes all bindings from this descriptor set.
    virtual void Clear() = 0;
};

}  // namespace gpu
