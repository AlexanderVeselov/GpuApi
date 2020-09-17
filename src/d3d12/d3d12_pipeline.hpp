#ifndef D3D12_PIPELINE_HPP_
#define D3D12_PIPELINE_HPP_

#include "gpu_pipeline.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
    class D3D12Device;

    class D3D12GraphicsPipeline : public GraphicsPipeline
    {
    public:
        D3D12GraphicsPipeline(D3D12Device& device, char const* vs_filename, char const* ps_filename);

    private:
        D3D12Device& device_;
        ComPtr<ID3D12RootSignature> root_signature_;
        ComPtr<ID3D12PipelineState> pipeline_state_;
    };

    class D3D12ComputePipeline : public GraphicsPipeline
    {
    public:

    };

} // namespace gpu

#endif // D3D12_PIPELINE_HPP_
