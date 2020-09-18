#ifndef GPU_SWAPCHAIN_HPP_
#define GPU_SWAPCHAIN_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class Swapchain
    {
    public:
        virtual void Present() = 0;
        virtual std::uint32_t GetImagesCount() const = 0;
    };

} // namespace gpu

#endif // GPU_SWAPCHAIN_HPP_
