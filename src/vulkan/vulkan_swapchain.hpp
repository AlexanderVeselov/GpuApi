#pragma once

#include "gpu_swapchain.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanDevice;

class VulkanSwapchain final : public Swapchain
{
public:
    VulkanSwapchain(VulkanDevice& device, void* window_native_handle, uint32_t width, uint32_t height,
        uint32_t image_count);
    ~VulkanSwapchain() override;

    void Present() override;

private:
    void CreateSurface(void* window_native_handle);
    void CreateSwapchain(uint32_t width, uint32_t height, uint32_t image_count);
    void AcquireNextImage();

private:
    VulkanDevice& device_;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    VkFence image_acquired_fence_ = VK_NULL_HANDLE;
    uint32_t present_queue_family_index_ = UINT32_MAX;
};

}  // namespace gpu
