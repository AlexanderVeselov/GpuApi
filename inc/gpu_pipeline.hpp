#pragma once

#include "gpu_types.hpp"

namespace gpu
{

class Pipeline
{
public:
    virtual void Reload() = 0;
    virtual ~Pipeline() = default;

protected:
};

class GraphicsPipeline : virtual public Pipeline
{
public:
    GraphicsPipeline(GraphicsPipelineDesc const& desc) : pipeline_desc_(desc) {}
    GraphicsPipelineDesc const& GetDesc() const { return pipeline_desc_; }

protected:
    GraphicsPipelineDesc pipeline_desc_;
};

class ComputePipeline : virtual public Pipeline
{
public:
    ComputePipeline(char const* cs_filename) : cs_filename_(cs_filename) {}
    std::string const& GetCSFilename() const { return cs_filename_; }

protected:
    std::string cs_filename_;
};

} // namespace gpu
