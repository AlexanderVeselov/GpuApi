#ifndef GPU_PIPELINE_HPP_
#define GPU_PIPELINE_HPP_

#include "gpu_types.hpp"

namespace gpu
{
enum class BindingType
{
    kBuffer,
    kImage,
    kSampler
};

struct Binding
{
    std::string name;
    std::uint32_t binding;
    std::uint32_t space;
    BindingType type;
    union
    {
        BufferPtr buffer;
        ImagePtr image;
        // SamplerPtr sampler;
    };
};

class Pipeline
{
public:
    void BindBuffer(BufferPtr const& buffer, std::uint32_t binding, std::uint32_t space);
    void BindImage(ImagePtr const& image, std::uint32_t binding, std::uint32_t space);
    virtual void Reload() = 0;

protected:
    std::vector<Binding> bindings_;
};

class GraphicsPipeline : public Pipeline
{
public:
    GraphicsPipeline(GraphicsPipelineDesc const& desc) : pipeline_desc_(desc) {}
    GraphicsPipelineDesc const& GetDesc() const { return pipeline_desc_; }

protected:
    GraphicsPipelineDesc pipeline_desc_;
};

class ComputePipeline : public Pipeline
{
public:
    ComputePipeline(char const* cs_filename) : cs_filename_(cs_filename) {}
    std::string const& GetCSFilename() const { return cs_filename_; }

protected:
    std::string cs_filename_;
};

} // namespace gpu

#endif // GPU_PIPELINE_HPP_
