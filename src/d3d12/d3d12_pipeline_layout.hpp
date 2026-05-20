#pragma once

#include "../common/shader_reflection.hpp"
#include "d3d12_common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gpu
{
class D3D12Device;
struct D3D12Shader;

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
    uint32_t binding = 0;
    uint32_t space = 0;
    uint32_t root_parameter_index = 0;
    ResourceType type = ResourceType::kBuffer;
    DescriptorType descriptor_type = DescriptorType::kDescriptorTable;
    D3D12_DESCRIPTOR_RANGE_TYPE range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    uint32_t descriptor_count = 1;
    uint32_t num_32bit_values = 0;
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
};

class D3D12PipelineLayout
{
public:
    explicit D3D12PipelineLayout(D3D12Device& device);
    D3D12PipelineLayout(D3D12Device& device, std::vector<D3D12Shader const*> const& shaders);

    void Build(std::vector<D3D12Shader const*> const& shaders);
    void Clear();

    ID3D12RootSignature* GetRootSignature() const { return root_signature_.Get(); }
    std::vector<D3D12Binding> const& GetBindings() const { return bindings_; }

    bool IsCompatibleWith(D3D12PipelineLayout const& other) const;

    bool HasBinding(uint32_t binding, uint32_t space) const;
    D3D12Binding const& FindBinding(uint32_t binding, uint32_t space) const;

private:
    void AddShaderReflection(ShaderReflection const& reflection);
    void AddOrMergeBinding(D3D12Binding const& binding);
    void CreateRootSignature();

private:
    D3D12Device& device_;
    std::vector<D3D12Binding> bindings_;
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges_;
    std::vector<D3D12_ROOT_PARAMETER> root_parameters_;
    ComPtr<ID3D12RootSignature> root_signature_;
};

}  // namespace gpu
