#include "vulkan_swapchain.hpp"

#include "vulkan_api.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace gpu
{
static VkSurfaceFormatKHR FindSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t format_count = 0;
    VkResult status =
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan surface formats");
    THROW_IF(format_count == 0, "No Vulkan surface formats are available");

    std::vector<VkSurfaceFormatKHR> available_formats(format_count);
    status = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &format_count, available_formats.data());
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan surface formats");

    for (VkSurfaceFormatKHR const& format : available_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return available_formats[0];
}

static VkPresentModeKHR FindPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t present_mode_count = 0;
    VkResult status = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &present_mode_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan present modes");
    THROW_IF(present_mode_count == 0, "No Vulkan present modes are available");

    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    status = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &present_mode_count, present_modes.data());
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan present modes");

    for (VkPresentModeKHR mode : present_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static uint32_t Clamp(uint32_t value, uint32_t min_value, uint32_t max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, void* window_native_handle, uint32_t width,
    uint32_t height, uint32_t image_count)
    : Swapchain(ImageFormat::kUnknown), device_(device)
{
    CreateSurface(window_native_handle);
    CreateSwapchain(width, height, image_count);
    AcquireNextImage();
}

VulkanSwapchain::~VulkanSwapchain()
{
    VkDevice logical_device = device_.GetDevice();

    if (swapchain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(logical_device, swapchain_, nullptr);
    }

    if (surface_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(device_.GetApi().GetInstance(), surface_, nullptr);
    }
}

void VulkanSwapchain::CreateSurface(void* window_native_handle)
{
    HWND hwnd = static_cast<HWND>(window_native_handle);
    HINSTANCE hinstance = GetModuleHandle(nullptr);

    VkWin32SurfaceCreateInfoKHR surface_create_info{};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hinstance = hinstance;
    surface_create_info.hwnd = hwnd;

    VkResult status = vkCreateWin32SurfaceKHR(
        device_.GetApi().GetInstance(), &surface_create_info, nullptr, &surface_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan Win32 surface");

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        device_.GetPhysicalDevice(), &queue_family_count, nullptr);

    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        VkBool32 present_supported = VK_FALSE;
        status = vkGetPhysicalDeviceSurfaceSupportKHR(
            device_.GetPhysicalDevice(), i, surface_, &present_supported);
        VK_THROW_IF_FAILED(status, "Failed to query Vulkan presentation support");

        if (present_supported)
        {
            present_queue_family_index_ = i;
            break;
        }
    }

    THROW_IF(present_queue_family_index_ == UINT32_MAX, "No Vulkan present queue family");
    vkGetDeviceQueue(device_.GetDevice(), present_queue_family_index_, 0, &present_queue_);
}

void VulkanSwapchain::CreateSwapchain(
    uint32_t width, uint32_t height, uint32_t requested_image_count)
{
    VkSurfaceCapabilitiesKHR surface_capabilities{};
    VkResult status = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device_.GetPhysicalDevice(), surface_, &surface_capabilities);
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan surface capabilities");

    uint32_t image_count = requested_image_count;
    if (image_count < surface_capabilities.minImageCount)
    {
        image_count = surface_capabilities.minImageCount;
    }
    if (surface_capabilities.maxImageCount > 0)
    {
        if (image_count > surface_capabilities.maxImageCount)
        {
            image_count = surface_capabilities.maxImageCount;
        }
    }

    VkSurfaceFormatKHR surface_format = FindSurfaceFormat(device_.GetPhysicalDevice(), surface_);
    image_format_ = FromVkFormat(surface_format.format);

    VkExtent2D extent{};
    if (surface_capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = surface_capabilities.currentExtent;
    }
    else
    {
        extent.width = Clamp(width, surface_capabilities.minImageExtent.width,
            surface_capabilities.maxImageExtent.width);
        extent.height = Clamp(height, surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.height);
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    uint32_t queue_family_indices[] = {
        device_.GetGraphicsQueueFamilyIndex(),
        present_queue_family_index_,
    };

    if (queue_family_indices[0] == queue_family_indices[1])
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }

    create_info.preTransform = surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = FindPresentMode(device_.GetPhysicalDevice(), surface_);
    create_info.clipped = VK_TRUE;

    status = vkCreateSwapchainKHR(device_.GetDevice(), &create_info, nullptr, &swapchain_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan swapchain");

    uint32_t actual_image_count = 0;
    status = vkGetSwapchainImagesKHR(device_.GetDevice(), swapchain_, &actual_image_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan swapchain images");

    std::vector<VkImage> images(actual_image_count);
    status = vkGetSwapchainImagesKHR(
        device_.GetDevice(), swapchain_, &actual_image_count, images.data());
    VK_THROW_IF_FAILED(status, "Failed to get Vulkan swapchain images");

    for (VkImage image : images)
    {
        swapchain_images_.push_back(
            std::make_shared<VulkanImage>(device_, image, extent.width, extent.height,
                surface_format.format, 1, 1, ImageFlags::kRenderTarget | ImageFlags::kStorage));
    }
}

void VulkanSwapchain::AcquireNextImage()
{
    VkResult status = vkAcquireNextImageKHR(device_.GetDevice(), swapchain_, UINT64_MAX,
        VK_NULL_HANDLE, VK_NULL_HANDLE, &current_image_index_);
    VK_THROW_IF_FAILED(status, "Failed to acquire next Vulkan swapchain image");
}

void VulkanSwapchain::Present()
{
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &current_image_index_;

    VkResult status = vkQueuePresentKHR(present_queue_, &present_info);
    VK_THROW_IF_FAILED(status, "Failed to present Vulkan swapchain image");

    AcquireNextImage();
}

} // namespace gpu
