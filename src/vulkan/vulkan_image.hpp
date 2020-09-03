#ifndef VULKAN_IMAGE_HPP_
#define VULKAN_IMAGE_HPP_

#include "vulkan_shared_object.hpp"
#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>

class VulkanImage
{
public:
    VulkanImage(VulkanDevice & device, VkImage image, VkFormat format);
    VkImageView GetImageView() const { return image_view_; }

private:
    VulkanDevice & device_;
    VulkanScopedObject<VkImageView, vkCreateImageView, vkDestroyImageView> image_view_;

};

#endif // VULKAN_IMAGE_HPP_
