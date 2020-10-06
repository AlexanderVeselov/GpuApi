#ifndef D3D12_BUFFER_HPP_
#define D3D12_BUFFER_HPP_

#include "gpu_buffer.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
    class D3D12Device;

    class D3D12Buffer : public Buffer
    {
    public:
        D3D12Buffer(D3D12Device& device, std::size_t size);
        ID3D12Resource* GetResource() const { return resource_.Get(); }
        void* Map() override;
        void Unmap() override;

    private:
        ComPtr<ID3D12Resource> resource_;


    };
} // namespace gpu


#endif // D3D12_BUFFER_HPP_
