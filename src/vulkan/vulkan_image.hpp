#pragma once

#include "gpu_image.hpp"

#include <vulkan/vulkan.h>

#include <unordered_map>

namespace gpu
{
class VulkanDevice;

class VulkanImage final : public Image
{
public:
    VulkanImage(VulkanDevice& device, uint32_t width, uint32_t height, ImageFormat format, uint32_t mip_count,
        uint32_t array_size, ImageFlags flags);

    VulkanImage(VulkanDevice& device, VkImage image, uint32_t width, uint32_t height, VkFormat native_format,
        uint32_t mip_count, uint32_t array_size, ImageFlags flags);

    ~VulkanImage() override;

    VkImage GetImage() const { return image_; }
    VkImageView GetImageView() { return GetView(ImageView{}); }
    VkImageView GetView(ImageView const& view);

private:
    VkImageView CreateImageView(ImageView const& view);

private:
    VulkanDevice& device_;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkFormat native_format_ = VK_FORMAT_UNDEFINED;
    std::unordered_map<ImageView, VkImageView, ImageViewHash> image_views_;
    bool owns_image_ = false;
};

VkFormat ToVkFormat(ImageFormat format);
ImageFormat FromVkFormat(VkFormat format);

}  // namespace gpu
