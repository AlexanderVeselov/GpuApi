#ifndef D3D12_QUEUE_HPP_
#define D3D12_QUEUE_HPP_

#include "gpu_queue.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
    class D3D12Device;

    class D3D12Queue : public Queue
    {
    public:
        D3D12Queue(D3D12Device& device, D3D12_COMMAND_LIST_TYPE command_list_type);

        CommandBufferPtr CreateCommandBuffer() override;
        void Submit(CommandBufferPtr const& cmd_buffer, FencePtr fence) override;
        void WaitIdle() override;

        ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
        ID3D12CommandAllocator* GetCommandAllocator() const { return command_allocator_.Get(); }

    private:
        D3D12Device& device_;
        D3D12_COMMAND_LIST_TYPE command_list_type_;
        ComPtr<ID3D12CommandQueue> queue_;
        ComPtr<ID3D12CommandAllocator> command_allocator_;

    };

} // namespace gpu

#endif // D3D12_QUEUE_HPP_
