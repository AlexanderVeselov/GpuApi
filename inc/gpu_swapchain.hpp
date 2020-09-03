#ifndef GPU_SWAPCHAIN_HPP_
#define GPU_SWAPCHAIN_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class PipelineInterface
    {
    public:
        virtual void BindBuffer(BufferPtr const& buffer, std::uint32_t binding, std::uint32_t space) = 0;
        virtual void BindImage(ImagePtr const& image, std::uint32_t binding, std::uint32_t space) = 0;
    };

    class GraphicsPipeline
    {
    public:
        virtual PipelineInterface& GetInterface() = 0;

    };

    class ComputePipeline
    {
    public:
        virtual PipelineInterface& GetInterface() = 0;

    };

} // namespace gpu

#endif // GPU_SWAPCHAIN_HPP_
