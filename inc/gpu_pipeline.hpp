#ifndef GPU_PIPELINE_HPP_
#define GPU_PIPELINE_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class Pipeline
    {
    public:
        virtual void BindBuffer(BufferPtr const& buffer, std::uint32_t binding, std::uint32_t space) = 0;
        virtual void BindImage(ImagePtr const& image, std::uint32_t binding, std::uint32_t space) = 0;
    };

    class GraphicsPipeline : public Pipeline
    {
    public:

    };

    class ComputePipeline : public Pipeline
    {
    public:

    };

} // namespace gpu

#endif // GPU_PIPELINE_HPP_
