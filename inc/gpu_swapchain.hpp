#pragma once

#include "gpu_types.hpp"

#include <vector>

namespace gpu
{
/// Presentation surface with a fixed set of backbuffer images.
class Swapchain
{
public:
    explicit Swapchain(ImageFormat format)
        : image_format_(format)
    {}

    virtual ~Swapchain() = default;

    /// Presents the current image and advances the swapchain image index.
    virtual void Present() = 0;

    std::vector<ImagePtr> const& GetImages() const { return swapchain_images_; }
    ImagePtr const& GetImage(uint32_t index) const { return swapchain_images_[index]; }
    uint32_t GetImageCount() const
    {
        return static_cast<uint32_t>(swapchain_images_.size());
    }

    ImagePtr GetCurrentImage() const { return swapchain_images_[current_image_index_]; }
    uint32_t GetCurrentImageIndex() const { return current_image_index_; }
    ImageFormat GetFormat() const { return image_format_; }

protected:
    uint32_t current_image_index_ = 0u;
    ImageFormat image_format_ = ImageFormat::kRGBA8_UNorm;
    std::vector<ImagePtr> swapchain_images_;
};

} // namespace gpu
