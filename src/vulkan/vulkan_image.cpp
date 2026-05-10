#include "vulkan_image.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

namespace gpu
{
VkFormat ToVkFormat(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::kRGBA32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case ImageFormat::kRGBA16_Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ImageFormat::kRGBA8_SInt: return VK_FORMAT_R8G8B8A8_SINT;
    case ImageFormat::kRGBA8_UInt: return VK_FORMAT_R8G8B8A8_UINT;
    case ImageFormat::kRGBA8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat::kRGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    case ImageFormat::kRG32_Float: return VK_FORMAT_R32G32_SFLOAT;
    case ImageFormat::kRG16_Float: return VK_FORMAT_R16G16_SFLOAT;
    case ImageFormat::kR32_Float: return VK_FORMAT_R32_SFLOAT;
    case ImageFormat::kD32_Float: return VK_FORMAT_D32_SFLOAT;
    case ImageFormat::kR32_Typeless: return VK_FORMAT_R32_SFLOAT;
    case ImageFormat::kR16_Float: return VK_FORMAT_R16_SFLOAT;
    default: return VK_FORMAT_UNDEFINED;
    }
}

ImageFormat FromVkFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM: return ImageFormat::kRGBA8_UNorm;
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB: return ImageFormat::kRGBA8_SRGB;
    default: return ImageFormat::kUnknown;
    }
}

static VkImageUsageFlags ToVkImageUsage(ImageFlags flags)
{
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (HasFlag(flags, ImageFlags::kRenderTarget))
    {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (HasFlag(flags, ImageFlags::kDepthStencil))
    {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (HasFlag(flags, ImageFlags::kShaderResource))
    {
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if (HasFlag(flags, ImageFlags::kStorage))
    {
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    return usage;
}

VulkanImage::VulkanImage(
    VulkanDevice& device,
    uint32_t width,
    uint32_t height,
    ImageFormat format,
    uint32_t mip_count,
    uint32_t array_size,
    ImageFlags flags)
    : Image(width, height, format, mip_count, array_size, flags)
    , device_(device)
    , owns_image_(true)
{
    native_format_ = ToVkFormat(format);

    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent = { width, height, 1 };
    image_create_info.mipLevels = mip_count;
    image_create_info.arrayLayers = array_size;
    image_create_info.format = native_format_;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = ToVkImageUsage(flags);
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice logical_device = device_.GetDevice();
    VkResult status = vkCreateImage(logical_device, &image_create_info, nullptr, &image_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan image");

    VkMemoryRequirements memory_requirements{};
    vkGetImageMemoryRequirements(logical_device, image_, &memory_requirements);

    memory_ = device_.GetMemoryManager().AllocateMemory(
        memory_requirements,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    status = vkBindImageMemory(logical_device, image_, memory_, 0);
    VK_THROW_IF_FAILED(status, "Failed to bind Vulkan image memory");

    CreateImageView();
}

VulkanImage::VulkanImage(
    VulkanDevice& device,
    VkImage image,
    uint32_t width,
    uint32_t height,
    VkFormat native_format,
    uint32_t mip_count,
    uint32_t array_size,
    ImageFlags flags)
    : Image(width, height, FromVkFormat(native_format), mip_count, array_size, flags)
    , device_(device)
    , image_(image)
    , native_format_(native_format)
{
    CreateImageView();
}

VulkanImage::~VulkanImage()
{
    VkDevice logical_device = device_.GetDevice();

    if (image_view_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(logical_device, image_view_, nullptr);
    }

    if (memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(logical_device, memory_, nullptr);
    }

    if (owns_image_ && image_ != VK_NULL_HANDLE)
    {
        vkDestroyImage(logical_device, image_, nullptr);
    }
}

void VulkanImage::CreateImageView()
{
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (HasFlag(GetFlags(), ImageFlags::kDepthStencil))
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image_;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = native_format_;
    create_info.subresourceRange.aspectMask = aspect;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = GetMipCount();
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = GetArraySize();

    VkResult status = vkCreateImageView(device_.GetDevice(), &create_info, nullptr, &image_view_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan image view");
}

} // namespace gpu
