#ifndef D3D12_PIPELINE_HPP_
#define D3D12_PIPELINE_HPP_

#include "gpu_pipeline.hpp"
#include "d3d12_common.hpp"

namespace gpu
{
class D3D12Device;


struct D3D12Binding
{
    enum class ResourceType
    {
        kBuffer,
        kImage,
        kSampler
    };

    enum class DescriptorType
    {
        kRootConstant,
        kDescriptorTable
    };

    std::string name;
    std::uint32_t binding;
    std::uint32_t space;
    std::uint32_t root_parameter_index;
    ResourceType type;
    DescriptorType descriptor_type;
    //union
    //{
    //    BufferPtr buffer;
    //    ImagePtr image;
    //    // SamplerPtr sampler;
    //};
};

class D3D12Pipeline : virtual public Pipeline
{
public:
    D3D12Pipeline(D3D12Device& device);
    void BindBuffer(BufferPtr const& buffer, std::uint32_t binding, std::uint32_t space) override;
    void BindImage(ImagePtr const& image, std::uint32_t binding, std::uint32_t space) override;
    ID3D12PipelineState* GetPipelineState() const { return pipeline_state_.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return root_signature_.Get(); }
    std::vector<D3D12Binding> const& GetBindings() const { return bindings_; }
protected:
    D3D12Device& device_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
    std::vector<D3D12Binding> bindings_;
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

#endif // D3D12_PIPELINE_HPP_
