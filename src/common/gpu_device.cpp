#include "gpu_device.hpp"

#include "gpu_pipeline.hpp"

#include <algorithm>
#include <exception>

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

void Device::RegisterPipeline(Pipeline* pipeline)
{
    if (!pipeline)
    {
        return;
    }

    if (std::find(pipelines_.begin(), pipelines_.end(), pipeline) == pipelines_.end())
    {
        pipelines_.push_back(pipeline);
    }
}

void Device::UnregisterPipeline(Pipeline* pipeline)
{
    auto it = std::find(pipelines_.begin(), pipelines_.end(), pipeline);
    if (it != pipelines_.end())
    {
        pipelines_.erase(it);
    }
}

PipelineReloadResult Device::ReloadRegisteredPipelines()
{
    PipelineReloadResult result = {};
    for (Pipeline* pipeline : pipelines_)
    {
        try
        {
            pipeline->Reload();
            ++result.reloaded_count;
        }
        catch (std::exception const& ex)
        {
            result.success = false;
            result.error = ex.what();
            return result;
        }
    }

    return result;
}

}  // namespace gpu
