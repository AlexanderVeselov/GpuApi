#pragma once

#include "gpu_buffer.hpp"
#include "gpu_image.hpp"

#include <cstdint>

namespace gpu
{
class DescriptorSet
{
public:
    virtual ~DescriptorSet() = default;

    virtual void BindBuffer(Buffer& buffer, std::uint32_t binding, std::uint32_t space) = 0;
    virtual void BindImage(Image& image, std::uint32_t binding, std::uint32_t space) = 0;
    virtual void BindImage(Image& image, ImageView const& view, std::uint32_t binding, std::uint32_t space) = 0;

    virtual void Clear() = 0;
};

} // namespace gpu
