#pragma once

#include "videoapi/vulkan_shared_object.hpp"
#include "videoapi/vulkan_device.hpp"
#include <vulkan/vulkan.h>
#include <vector>

class VulkanImage;

class VulkanSwapchain 
{
public:
    VulkanSwapchain(VulkanDevice & device, uint32_t width, uint32_t height);
    size_t GetImagesCount() const { return swapchain_images_.size(); }
    std::shared_ptr<VulkanImage> GetImage(size_t index) const { return swapchain_images_[index]; }
    uint32_t GetCurrentImageIndex() const { return current_image_index_; }
    void Present();

private:
    VulkanDevice & device_;
    VulkanScopedObject<VkSwapchainKHR, vkCreateSwapchainKHR, vkDestroySwapchainKHR> swapchain_;
    std::vector<std::shared_ptr<VulkanImage>> swapchain_images_;
    VkFormat swapchain_image_format_;
    uint32_t current_image_index_;
    VkQueue present_queue_;

};
