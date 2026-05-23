#pragma once

#include "gpu_descriptor_set.hpp"
#include "gpu_types.hpp"

#include <stdexcept>

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
    explicit GraphicsPipeline(GraphicsPipelineDesc const& desc) : pipeline_desc_(desc)
    {
        if (pipeline_desc_.root_constants_name.empty())
        {
            throw std::runtime_error("GraphicsPipeline root constants name must not be empty");
        }
    }

    GraphicsPipelineDesc const& GetDesc() const { return pipeline_desc_; }

protected:
    GraphicsPipelineDesc pipeline_desc_;
};

/// Compute pipeline state object.
class ComputePipeline : virtual public Pipeline
{
public:
    explicit ComputePipeline(char const* cs_filename, char const* root_constants_name = "g_RootConstants")
        : cs_filename_(cs_filename), root_constants_name_(root_constants_name)
    {
        if (root_constants_name_.empty())
        {
            throw std::runtime_error("ComputePipeline root constants name must not be empty");
        }
    }

    std::string const& GetCSFilename() const { return cs_filename_; }

    std::string const& GetRootConstantsName() const { return root_constants_name_; }

protected:
    std::string cs_filename_;
    std::string root_constants_name_;
};

}  // namespace gpu
