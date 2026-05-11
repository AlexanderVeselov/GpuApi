#pragma once

#include "d3d12_common.hpp"
#include "gpu_queue.hpp"

#include <cstdint>
#include <deque>

namespace gpu
{
class D3D12Device;

class D3D12Queue : public Queue
{
  public:
    D3D12Queue(D3D12Device& device, D3D12_COMMAND_LIST_TYPE command_list_type);
    ~D3D12Queue() override;

    CommandBufferPtr CreateCommandBuffer() override;
    void Submit(CommandBufferPtr cmd_buffer) override;
    void WaitIdle() override;

    ID3D12CommandQueue* GetQueue() const
    {
        return queue_.Get();
    }

  private:
    struct InFlightSubmission
    {
        std::uint64_t fence_value = 0;
        CommandBufferPtr command_buffer;
    };

    void CollectCompletedSubmissions();

  private:
    D3D12Device& device_;
    D3D12_COMMAND_LIST_TYPE command_list_type_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12Fence> fence_;
    HANDLE fence_event_ = NULL;
    std::uint64_t next_fence_value_ = 1;
    std::deque<InFlightSubmission> in_flight_submissions_;
};

} // namespace gpu
