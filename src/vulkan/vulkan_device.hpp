#pragma once

#include "gpu_device.hpp"

#include "vulkan_memory_manager.hpp"

#include <vulkan/vulkan.h>

#include <memory>

namespace gpu
{
class VulkanApi;

class VulkanDevice final : public Device
{
public:
    VulkanDevice(VulkanApi& api, VkPhysicalDevice physical_device);
    ~VulkanDevice() override;

    BufferPtr CreateBuffer(std::size_t size, std::uint32_t stride, BufferFlags flags) override;
    ImagePtr CreateImage(uint32_t width, uint32_t height, ImageFormat format, ImageFlags flags, uint32_t mip_count = 1,
        uint32_t array_size = 1) override;

    Queue& GetQueue(QueueType queue_type) override;

    GraphicsPipelinePtr CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc) override;
    ComputePipelinePtr CreateComputePipeline(char const* cs_filename) override;
    void WaitIdle() override;

    SwapchainPtr CreateSwapchain(void* window_native_handle, std::uint32_t width, std::uint32_t height,
        std::uint32_t image_count) override;
    ImGuiRendererPtr CreateImGuiRenderer(void* glfw_window, Swapchain& swapchain) override;

    VulkanApi& GetApi() const { return api_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physical_device_; }
    VkDevice GetDevice() const { return device_; }
    VulkanMemoryManager& GetMemoryManager() { return memory_manager_; }

    uint32_t GetGraphicsQueueFamilyIndex() const { return graphics_queue_family_index_; }
    uint32_t GetComputeQueueFamilyIndex() const { return compute_queue_family_index_; }
    uint32_t GetTransferQueueFamilyIndex() const { return transfer_queue_family_index_; }

private:
    void FindQueueFamilyIndices();
    void CreateLogicalDevice();
    SamplerPtr CreateSampler(SamplerDesc const& desc) override;

private:
    VulkanApi& api_;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VulkanMemoryManager memory_manager_;

    uint32_t graphics_queue_family_index_ = UINT32_MAX;
    uint32_t compute_queue_family_index_ = UINT32_MAX;
    uint32_t transfer_queue_family_index_ = UINT32_MAX;

    std::unique_ptr<Queue> graphics_queue_;
    std::unique_ptr<Queue> compute_queue_;
    std::unique_ptr<Queue> transfer_queue_;
};

}  // namespace gpu
