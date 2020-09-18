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

    protected:
        std::vector<ImagePtr> swapchain_images_;

    };

} // namespace gpu

#endif // GPU_SWAPCHAIN_HPP_
