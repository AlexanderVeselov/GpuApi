#include "d3d12_sampler.hpp"

#include "d3d12_device.hpp"

#include <cfloat>

namespace gpu
{
namespace
{
D3D12_TEXTURE_ADDRESS_MODE ToD3D12AddressMode(SamplerAddressMode mode)
{
    switch (mode)
    {
    case SamplerAddressMode::kRepeat:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case SamplerAddressMode::kClampToEdge:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }

    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(SamplerComparisonFunc func)
{
    switch (func)
    {
    case SamplerComparisonFunc::kNever:
        return D3D12_COMPARISON_FUNC_NEVER;
    case SamplerComparisonFunc::kLess:
        return D3D12_COMPARISON_FUNC_LESS;
    case SamplerComparisonFunc::kEqual:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case SamplerComparisonFunc::kLessEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case SamplerComparisonFunc::kGreater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case SamplerComparisonFunc::kNotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case SamplerComparisonFunc::kGreaterEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case SamplerComparisonFunc::kAlways:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    case SamplerComparisonFunc::kNone:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    }

    return D3D12_COMPARISON_FUNC_ALWAYS;
}

D3D12_FILTER ToD3D12Filter(SamplerFilter min_filter, SamplerFilter mag_filter, bool comparison_enabled)
{
    if (min_filter == SamplerFilter::kNearest && mag_filter == SamplerFilter::kNearest)
    {
        return comparison_enabled ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;
    }

    if (min_filter == SamplerFilter::kNearest && mag_filter == SamplerFilter::kLinear)
    {
        return comparison_enabled ? D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR
                                  : D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    }

    if (min_filter == SamplerFilter::kLinear && mag_filter == SamplerFilter::kNearest)
    {
        return comparison_enabled ? D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT
                                  : D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    }

    return comparison_enabled ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
}
}  // namespace

D3D12Sampler::D3D12Sampler(D3D12Device& device, SamplerDesc const& desc) : Sampler(desc), device_(device)
{
    D3D12DescriptorManager& descriptor_manager = device_.GetDescriptorManager();
    descriptor_ = descriptor_manager.AllocateCPUSampler();

    D3D12_SAMPLER_DESC sampler_desc = {};
    bool const comparison_enabled = desc.comparison_func != SamplerComparisonFunc::kNone;
    sampler_desc.Filter = ToD3D12Filter(desc.min_filter, desc.mag_filter, comparison_enabled);
    sampler_desc.AddressU = ToD3D12AddressMode(desc.address_u);
    sampler_desc.AddressV = ToD3D12AddressMode(desc.address_v);
    sampler_desc.AddressW = ToD3D12AddressMode(desc.address_w);
    sampler_desc.MipLODBias = desc.mip_lod_bias;
    sampler_desc.MaxAnisotropy = 1;
    sampler_desc.ComparisonFunc = ToD3D12ComparisonFunc(desc.comparison_func);
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = FLT_MAX;

    device_.GetD3D12Device()->CreateSampler(&sampler_desc, descriptor_manager.GetCPU(descriptor_));
}

D3D12Sampler::~D3D12Sampler()
{
    device_.GetDescriptorManager().Free(descriptor_);
}

}  // namespace gpu
