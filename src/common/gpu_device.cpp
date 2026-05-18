#include "gpu_device.hpp"

namespace gpu
{
SamplerPtr Device::GetSampler(SamplerDesc const& desc)
{
    auto sampler_it = sampler_cache_.find(desc);
    if (sampler_it != sampler_cache_.end())
    {
        return sampler_it->second;
    }

    SamplerPtr sampler = CreateSampler(desc);
    sampler_cache_.emplace(desc, sampler);
    return sampler;
}

}  // namespace gpu
