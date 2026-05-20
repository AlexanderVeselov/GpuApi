#pragma once

#include "gpu_buffer.hpp"
#include "gpu_image.hpp"
#include "gpu_imgui.hpp"
#include "gpu_sampler.hpp"
#include "gpu_types.hpp"

#include <unordered_map>

namespace gpu
{
class D3D12Pipeline;
class VulkanPipeline;

struct PipelineReloadResult
{
    bool success = true;
    uint32_t reloaded_count = 0;
    std::string error;
};

/// Logical GPU device. Creates resources, queues, pipelines, and presentation objects.
class Device
{
public:
    virtual ~Device() = default;

    /// Creates a buffer with a fixed element stride and explicit usage flags.
    virtual BufferPtr CreateBuffer(size_t size, uint32_t stride, BufferFlags flags) = 0;

    /// Creates an image with usage flags and optional subresource counts.
    virtual ImagePtr CreateImage(uint32_t width, uint32_t height, ImageFormat format, ImageFlags flags,
        uint32_t mip_count = 1, uint32_t array_size = 1) = 0;

    /// Returns a cached immutable sampler state object.
    SamplerPtr GetSampler(SamplerDesc const& desc);

    /// Returns a queue supporting the requested command class.
    virtual Queue& GetQueue(QueueType queue_type) = 0;

    /// Creates a graphics pipeline from shader paths and render target formats.
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) = 0;

    /// Creates a compute pipeline from a compute shader path.
    virtual ComputePipelinePtr CreateComputePipeline(char const* cs_filename) = 0;

    /// Attempts to reload all pipelines. A pipeline keeps its old state if compilation fails or its shader layout
    /// changes.
    PipelineReloadResult ReloadPipelines();

    /// Blocks until all device queues are idle.
    virtual void WaitIdle() = 0;

    /// Creates a swapchain for a native platform window handle.
    virtual SwapchainPtr CreateSwapchain(void* window_native_handle, uint32_t width, uint32_t height,
        uint32_t image_count) = 0;

    /// Creates an ImGui renderer bound to a GLFW window and swapchain.
    virtual ImGuiRendererPtr CreateImGuiRenderer(void* glfw_window, Swapchain& swapchain) = 0;

protected:
    void ClearSamplerCache() { sampler_cache_.clear(); }
    void RegisterPipeline(Pipeline* pipeline);

private:
    friend class D3D12Pipeline;
    friend class VulkanPipeline;
    void UnregisterPipeline(Pipeline* pipeline);
    virtual SamplerPtr CreateSampler(SamplerDesc const& desc) = 0;

private:
    std::unordered_map<SamplerDesc, SamplerPtr, SamplerDescHash> sampler_cache_;
    std::vector<Pipeline*> pipelines_;
};

}  // namespace gpu
