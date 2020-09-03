#ifndef GPU_DEVICE_HPP_
#define GPU_DEVICE_HPP_

#include "gpu_types.hpp"

namespace gpu
{
    class Queue
    {
    public:
        virtual CommandBufferPtr CreateCommandBuffer() = 0;
        virtual void Submit(CommandBufferPtr const& cmd_buffer) = 0;
        virtual void WaitIdle() = 0;

    };

} // namespace gpu

#endif // GPU_DEVICE_HPP_
