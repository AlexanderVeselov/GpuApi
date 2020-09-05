#include "d3d12_queue.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_command_buffer.hpp"

namespace gpu
{
    D3D12Queue::D3D12Queue(D3D12Device& device, D3D12_COMMAND_LIST_TYPE command_list_type)
        : device_(device)
        , command_list_type_(command_list_type)
    {
        auto d3d12_device = device_.GetD3D12Device();

        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.Type = command_list_type_;
        ThrowIfFailed(d3d12_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue_)));

        ThrowIfFailed(d3d12_device->CreateCommandAllocator(command_list_type_, IID_PPV_ARGS(&command_allocator_)));

    }

    CommandBufferPtr D3D12Queue::CreateCommandBuffer()
    {
        return std::make_unique<D3D12CommandBuffer>(device_, *this, command_list_type_);
    }

    void D3D12Queue::Submit(CommandBufferPtr const& cmd_buffer)
    {
        D3D12CommandBuffer* d3d12_cmd_buffer = static_cast<D3D12CommandBuffer*>(cmd_buffer.get());

        //queue_->ExecuteCommandLists(1u, );
    }

    void D3D12Queue::WaitIdle()
    {
        //queue_->
    }

} // namespace gpu
