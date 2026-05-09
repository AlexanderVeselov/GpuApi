#pragma once

#include "gpu_types.hpp"

#include <cstddef>
#include <cstdint>

namespace gpu
{
struct ImageView
{
    uint32_t mip = 0;
    uint32_t mip_count = 1;
};

inline bool operator==(ImageView const& lhs, ImageView const& rhs)
{
    return lhs.mip == rhs.mip && lhs.mip_count == rhs.mip_count;
}

struct ImageViewHash
{
    std::size_t operator()(ImageView const& view) const
    {
        return (static_cast<std::size_t>(view.mip) << 32) ^ static_cast<std::size_t>(view.mip_count);
    }
};

class Image
{
public:
    Image(std::uint32_t width, std::uint32_t height, ImageFormat format)
        : width_(width), height_(height), format_(format)
    {}
    virtual ~Image() = default;

    std::uint32_t GetWidth() const { return width_; }
    std::uint32_t GetHeight() const { return height_; }
    ImageFormat GetFormat() const { return format_; }

protected:
    std::uint32_t width_;
    std::uint32_t height_;
    ImageFormat format_;

};
} // namespace gpu
