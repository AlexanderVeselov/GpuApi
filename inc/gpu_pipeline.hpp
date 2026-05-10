#pragma once

#include "gpu_descriptor_set.hpp"
#include "gpu_types.hpp"

namespace gpu
{
/// Base class for graphics and compute pipelines.
class Pipeline
{
public:
    virtual ~Pipeline() = default;

    /// Creates a descriptor set whose layout matches this pipeline.
    virtual DescriptorSetPtr CreateDescriptorSet() = 0;

    /// Rebuilds backend pipeline state from the original creation data.
    virtual void Reload() = 0;
};

/// Graphics pipeline state object.
class GraphicsPipeline : virtual public Pipeline
{
public:
    explicit GraphicsPipeline(GraphicsPipelineDesc const& desc)
        : pipeline_desc_(desc)
    {}

    GraphicsPipelineDesc const& GetDesc() const { return pipeline_desc_; }

protected:
    GraphicsPipelineDesc pipeline_desc_;
};

/// Compute pipeline state object.
class ComputePipeline : virtual public Pipeline
{
public:
    explicit ComputePipeline(char const* cs_filename)
        : cs_filename_(cs_filename)
    {}

    std::string const& GetCSFilename() const { return cs_filename_; }

protected:
    std::string cs_filename_;
};

} // namespace gpu
