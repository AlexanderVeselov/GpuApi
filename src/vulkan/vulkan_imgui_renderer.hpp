#pragma once

#include "gpu_imgui.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class Swapchain;
class VulkanDevice;

class VulkanImGuiRenderer final : public ImGuiRenderer
{
public:
    VulkanImGuiRenderer(VulkanDevice& device, void* glfw_window, Swapchain& swapchain);
    ~VulkanImGuiRenderer() override;

    void NewFrame() override;
    void Render(CommandBuffer& command_buffer) override;

private:
    VulkanDevice& device_;
    Swapchain& swapchain_;
};

}  // namespace gpu
