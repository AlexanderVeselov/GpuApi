#pragma once

#include "gpu_types.hpp"

namespace gpu
{
class Queue
{
public:
    virtual CommandBufferPtr CreateCommandBuffer() = 0;
    virtual void Submit(CommandBufferPtr const& cmd_buffer, FencePtr fence = nullptr) = 0;
    virtual void WaitIdle() = 0;

};

} // namespace gpu
