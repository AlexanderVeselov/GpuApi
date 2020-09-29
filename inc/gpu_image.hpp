#ifndef GPU_IMAGE_HPP_
#define GPU_IMAGE_HPP_

#include "gpu_types.hpp"

#include <cstdint>

namespace gpu
{
    class Image
    {
    public:
        Image(std::uint32_t width, std::uint32_t height, ImageFormat format)
            : width_(width), height_(height), format_(format)
        {}

        std::uint32_t GetWidth() const { return width_; }
        std::uint32_t GetHeight() const { return height_; }
        ImageFormat GetFormat() const { return format_; }

    protected:
        std::uint32_t width_;
        std::uint32_t height_;
        ImageFormat format_;

    };
} // namespace gpu

#endif // GPU_IMAGE_HPP_
