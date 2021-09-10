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
        D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc);
        ID3D12PipelineState* GetPipelineState() const { return pipeline_state_.Get(); }
        ID3D12RootSignature* GetRootSignature() const { return root_signature_.Get(); }
        D3D12Device& GetD3D12Device() const { return device_; }

    private:
        D3D12Device& device_;
        ComPtr<ID3D12RootSignature> root_signature_;
        ComPtr<ID3D12PipelineState> pipeline_state_;
    };

    class D3D12ComputePipeline : public ComputePipeline
    {
    public:
        D3D12ComputePipeline(D3D12Device& device, char const* cs_filename);
        ID3D12PipelineState* GetPipelineState() const { return pipeline_state_.Get(); }
        ID3D12RootSignature* GetRootSignature() const { return root_signature_.Get(); }
        D3D12Device& GetD3D12Device() const { return device_; }

    private:
        D3D12Device& device_;
        ComPtr<ID3D12RootSignature> root_signature_;
        ComPtr<ID3D12PipelineState> pipeline_state_;
    };

} // namespace gpu

#endif // D3D12_PIPELINE_HPP_
