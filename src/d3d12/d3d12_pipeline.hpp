#ifndef D3D12_PIPELINE_HPP_
#define D3D12_PIPELINE_HPP_

#include "gpu_pipeline.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
class D3D12Device;

class D3D12Pipeline : public Pipeline
{
public:
    D3D12Pipeline(D3D12Device& device);
    ID3D12PipelineState* GetPipelineState() const { return pipeline_state_.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return root_signature_.Get(); }
protected:
    D3D12Device& device_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
};

class D3D12GraphicsPipeline : public GraphicsPipeline, D3D12Pipeline
{
public:
    D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc);
    void Reload() override;
};

class D3D12ComputePipeline : public ComputePipeline, D3D12Pipeline
{
public:
    D3D12ComputePipeline(D3D12Device& device, char const* cs_filename);
    void Reload() override;
};

} // namespace gpu

#endif // D3D12_PIPELINE_HPP_
