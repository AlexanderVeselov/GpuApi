#pragma once

#include "gpu_types.hpp"

namespace gpu
{
/// GPU execution queue. Owns backend synchronization needed for submitted command buffers.
class Queue
{
  public:
    virtual ~Queue() = default;

    /// Creates a command buffer that can be submitted to this queue.
    virtual CommandBufferPtr CreateCommandBuffer() = 0;

    /// Closes and submits a command buffer. Ownership transfers to the queue.
    virtual void Submit(CommandBufferPtr cmd_buffer) = 0;

    /// Blocks until all submitted work on this queue is complete.
    virtual void WaitIdle() = 0;
};

} // namespace gpu
