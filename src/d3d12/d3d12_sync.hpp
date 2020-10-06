#ifndef D3D12_SYNC_HPP_
#define D3D12_SYNC_HPP_

#include "gpu_sync.hpp"
#include "d3d12_common.hpp"

#include <cstdint>

namespace gpu
{
    class D3D12Device;

    class D3D12Semaphore : public Semaphore
    {
    public:

    };

    class D3D12Fence : public Fence
    {
    public:
        D3D12Fence(D3D12Device& device);
        void Wait() override;
        ID3D12Fence* GetD3D12Fence() const { return fence_.Get(); }

        void SetNewEventValue();
        std::uint64_t GetCurrentValue() const { return current_value_; }

    private:
        ComPtr<ID3D12Fence> fence_;
        HANDLE event_;
        std::uint64_t current_value_ = 0ull;

    };

} // namespace gpu

#endif // D3D12_SYNC_HPP_
