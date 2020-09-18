#ifndef GPU_IMAGE_HPP_
#define GPU_IMAGE_HPP_

#include <cstdint>

namespace gpu
{
    class Image
    {
    public:
        Image(std::uint32_t width, std::uint32_t height)
            : width_(width), height_(height)
        {}

        std::uint32_t GetWidth() const { return width_; }
        std::uint32_t GetHeight() const { return height_; }

    protected:
        std::uint32_t width_;
        std::uint32_t height_;

    };
} // namespace gpu

#endif // GPU_IMAGE_HPP_
