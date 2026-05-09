#pragma once

#include "gpu_pipeline.hpp"
#include "d3d12_common.hpp"
#include "d3d12_pipeline_layout.hpp"

namespace gpu
{
class D3D12Device;

class D3D12Pipeline : virtual public Pipeline
{
public:
    D3D12Pipeline(D3D12Device& device);
    ID3D12PipelineState* GetPipelineState() const { return pipeline_state_.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return layout_.GetRootSignature(); }
    D3D12PipelineLayout const& GetLayout() const { return layout_; }
    std::vector<D3D12Binding> const& GetBindings() const { return layout_.GetBindings(); }
protected:
    D3D12Device& device_;
    D3D12PipelineLayout layout_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
};

class D3D12GraphicsPipeline : public GraphicsPipeline, public D3D12Pipeline
{
public:
    D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc);
    void Reload() override;
};

class D3D12ComputePipeline : public ComputePipeline, public D3D12Pipeline
{
public:
    D3D12ComputePipeline(D3D12Device& device, char const* cs_filename);
    void Reload() override;
};

} // namespace gpu
