#pragma once

#include <cstddef>
#include <functional>

namespace gpu
{
enum class SamplerFilter
{
    kNearest,
    kLinear
};

enum class SamplerAddressMode
{
    kRepeat,
    kClampToEdge
};

enum class SamplerComparisonFunc
{
    kNone,
    kNever,
    kLess,
    kEqual,
    kLessEqual,
    kGreater,
    kNotEqual,
    kGreaterEqual,
    kAlways
};

struct SamplerDesc
{
    SamplerFilter min_filter = SamplerFilter::kLinear;
    SamplerFilter mag_filter = SamplerFilter::kLinear;
    SamplerAddressMode address_u = SamplerAddressMode::kRepeat;
    SamplerAddressMode address_v = SamplerAddressMode::kRepeat;
    SamplerAddressMode address_w = SamplerAddressMode::kRepeat;
    SamplerComparisonFunc comparison_func = SamplerComparisonFunc::kNone;
    float mip_lod_bias = 0.0f;
};

inline bool operator==(SamplerDesc const& lhs, SamplerDesc const& rhs)
{
    return lhs.min_filter == rhs.min_filter && lhs.mag_filter == rhs.mag_filter && lhs.address_u == rhs.address_u
        && lhs.address_v == rhs.address_v && lhs.address_w == rhs.address_w
        && lhs.comparison_func == rhs.comparison_func && lhs.mip_lod_bias == rhs.mip_lod_bias;
}

struct SamplerDescHash
{
    std::size_t operator()(SamplerDesc const& desc) const
    {
        std::size_t hash = 1469598103934665603ull;
        auto combine = [&hash](auto value)
        {
            hash ^= static_cast<std::size_t>(value);
            hash *= 1099511628211ull;
        };

        combine(desc.min_filter);
        combine(desc.mag_filter);
        combine(desc.address_u);
        combine(desc.address_v);
        combine(desc.address_w);
        combine(desc.comparison_func);
        hash ^= std::hash<float>{}(desc.mip_lod_bias);
        hash *= 1099511628211ull;
        return hash;
    }
};

class Sampler
{
public:
    virtual ~Sampler() = default;

    SamplerDesc const& GetDesc() const { return desc_; }

protected:
    explicit Sampler(SamplerDesc const& desc) : desc_(desc) {}

private:
    SamplerDesc desc_;
};

}  // namespace gpu
