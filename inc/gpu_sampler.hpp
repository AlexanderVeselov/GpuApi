#pragma once

#include <cstddef>
#include <cstdint>
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
    uint32_t max_anisotropy = 1;
};

inline bool operator==(SamplerDesc const& lhs, SamplerDesc const& rhs)
{
    return lhs.min_filter == rhs.min_filter && lhs.mag_filter == rhs.mag_filter && lhs.address_u == rhs.address_u
        && lhs.address_v == rhs.address_v && lhs.address_w == rhs.address_w
        && lhs.comparison_func == rhs.comparison_func && lhs.mip_lod_bias == rhs.mip_lod_bias
        && lhs.max_anisotropy == rhs.max_anisotropy;
}

struct SamplerDescHash
{
    std::size_t operator()(SamplerDesc const& desc) const
    {
        constexpr std::size_t kFnvOffsetBasis = sizeof(std::size_t) == sizeof(std::uint64_t)
            ? static_cast<std::size_t>(1469598103934665603ull)
            : static_cast<std::size_t>(2166136261u);
        constexpr std::size_t kFnvPrime = sizeof(std::size_t) == sizeof(std::uint64_t)
            ? static_cast<std::size_t>(1099511628211ull)
            : static_cast<std::size_t>(16777619u);

        std::size_t hash = kFnvOffsetBasis;
        auto combine = [&hash, kFnvPrime](auto value)
        {
            hash ^= static_cast<std::size_t>(value);
            hash *= kFnvPrime;
        };

        combine(desc.min_filter);
        combine(desc.mag_filter);
        combine(desc.address_u);
        combine(desc.address_v);
        combine(desc.address_w);
        combine(desc.comparison_func);
        hash ^= std::hash<float>{}(desc.mip_lod_bias);
        hash *= kFnvPrime;
        combine(desc.max_anisotropy);
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
