#ifndef D3D12_QUEUE_HPP_
#define D3D12_QUEUE_HPP_

#include "gpu_queue.hpp"

namespace gpu
{
    class D3D12Queue : public Queue
    {
    public:
        CommandBufferPtr CreateCommandBuffer() override;
        void Submit(CommandBufferPtr const& cmd_buffer) override;
        void WaitIdle() override;

    };

} // namespace gpu

#endif // D3D12_QUEUE_HPP_
