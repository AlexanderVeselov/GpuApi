#ifndef GPU_SWAPCHAIN_HPP_
#define GPU_SWAPCHAIN_HPP_

#include "gpu_types.hpp"
#include <vector>

namespace gpu
{
    class Swapchain
    {
    public:
        virtual void Present() = 0;
        std::vector<ImagePtr> const& GetImages() const { return swapchain_images_; }
        std::uint32_t GetImageCount() const { return (std::uint32_t)swapchain_images_.size(); }
        ImagePtr GetCurrentImage() const { return swapchain_images_[current_image_index_]; }
        std::uint32_t GetCurrentImageIndex() const { return current_image_index_; }

    protected:
        std::uint32_t current_image_index_ = 0u;
        std::vector<ImagePtr> swapchain_images_;

    };

} // namespace gpu

#endif // GPU_SWAPCHAIN_HPP_
