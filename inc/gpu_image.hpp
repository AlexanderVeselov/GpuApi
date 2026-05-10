#pragma once

#include "gpu_types.hpp"

#include <cstddef>
#include <cstdint>

namespace gpu
{
enum class ImageFlags : uint32_t
{
    kNone = 0,
    kRenderTarget = 1 << 0,
    kDepthStencil = 1 << 1,
    kShaderResource = 1 << 2,
    kStorage = 1 << 3
};

inline ImageFlags operator|(ImageFlags lhs, ImageFlags rhs)
{
    return static_cast<ImageFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline ImageFlags operator&(ImageFlags lhs, ImageFlags rhs)
{
    return static_cast<ImageFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline ImageFlags& operator|=(ImageFlags& lhs, ImageFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline bool HasFlag(ImageFlags flags, ImageFlags flag)
{
    return static_cast<uint32_t>(flags & flag) != 0;
}

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
    Image(
        uint32_t width,
        uint32_t height,
        ImageFormat format,
        uint32_t mip_count,
        uint32_t array_size,
        ImageFlags flags)
        : width_(width)
        , height_(height)
        , format_(format)
        , mip_count_(mip_count)
        , array_size_(array_size)
        , flags_(flags)
    {}
    virtual ~Image() = default;

    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }
    ImageFormat GetFormat() const { return format_; }
    uint32_t GetMipCount() const { return mip_count_; }
    uint32_t GetArraySize() const { return array_size_; }
    ImageFlags GetFlags() const { return flags_; }

protected:
    uint32_t width_;
    uint32_t height_;
    ImageFormat format_;
    uint32_t mip_count_;
    uint32_t array_size_;
    ImageFlags flags_;

};
} // namespace gpu
