#include "d3d12_sync.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_device.hpp"

namespace gpu
{
    D3D12Fence::D3D12Fence(D3D12Device& device)
    {
        auto d3d12_device = device.GetD3D12Device();

        ThrowIfFailed(d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));

        event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        if (event_ == NULL)
        {
            throw D3D12Exception("Failed to create event",
                HRESULT_FROM_WIN32(GetLastError()), __FILE__, __LINE__);
        }
    }

    void D3D12Fence::Wait()
    {
        // Specify an event that should be fired when the fence reaches a certain value
        ThrowIfFailed(fence_->SetEventOnCompletion(current_value_, event_));
        // Wait for the event
        WaitForSingleObject(event_, INFINITE);
        // Update current value
        ++current_value_;
    }

} // namespace gpu