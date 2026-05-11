#include "d3d12_queue.hpp"
#include "d3d12_command_buffer.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"

#include <cassert>

namespace gpu
{
D3D12Queue::D3D12Queue(D3D12Device& device, D3D12_COMMAND_LIST_TYPE command_list_type)
    : device_(device), command_list_type_(command_list_type)
{
    auto d3d12_device = device_.GetD3D12Device();

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = command_list_type_;
    ThrowIfFailed(d3d12_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue_)));
    ThrowIfFailed(d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));

    fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fence_event_ == NULL)
    {
        throw D3D12Exception("D3D12Queue: failed to create fence event",
            HRESULT_FROM_WIN32(GetLastError()), __FILE__, __LINE__);
    }
}

D3D12Queue::~D3D12Queue()
{
    WaitIdle();

    if (fence_event_ != NULL)
    {
        CloseHandle(fence_event_);
        fence_event_ = NULL;
    }
}

CommandBufferPtr D3D12Queue::CreateCommandBuffer()
{
    return std::make_unique<D3D12CommandBuffer>(device_, *this, command_list_type_);
}

void D3D12Queue::Submit(CommandBufferPtr cmd_buffer)
{
    assert(cmd_buffer && "D3D12Queue::Submit: command buffer must not be null");
    CollectCompletedSubmissions();

    D3D12CommandBuffer* d3d12_cmd_buffer = static_cast<D3D12CommandBuffer*>(cmd_buffer.get());
    d3d12_cmd_buffer->Close();

    ID3D12CommandList* cmd_lists[] = {d3d12_cmd_buffer->GetCommandList()};
    queue_->ExecuteCommandLists(1u, cmd_lists);

    const std::uint64_t fence_value = next_fence_value_++;
    ThrowIfFailed(queue_->Signal(fence_.Get(), fence_value));

    in_flight_submissions_.push_back({fence_value, std::move(cmd_buffer)});
}

void D3D12Queue::WaitIdle()
{
    const std::uint64_t last_submitted_value = next_fence_value_ - 1;
    if (last_submitted_value == 0)
    {
        return;
    }

    if (fence_->GetCompletedValue() < last_submitted_value)
    {
        ThrowIfFailed(fence_->SetEventOnCompletion(last_submitted_value, fence_event_));
        WaitForSingleObject(fence_event_, INFINITE);
    }

    in_flight_submissions_.clear();
}

void D3D12Queue::CollectCompletedSubmissions()
{
    const std::uint64_t completed_value = fence_->GetCompletedValue();
    while (!in_flight_submissions_.empty() &&
           in_flight_submissions_.front().fence_value <= completed_value)
    {
        in_flight_submissions_.pop_front();
    }
}

} // namespace gpu
