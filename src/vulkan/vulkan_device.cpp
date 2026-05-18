#include "vulkan_device.hpp"

#include "vulkan_api.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_imgui_renderer.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_queue.hpp"
#include "vulkan_sampler.hpp"
#include "vulkan_swapchain.hpp"

#include <algorithm>
#include <array>

namespace gpu
{
VulkanDevice::VulkanDevice(VulkanApi& api, VkPhysicalDevice physical_device)
    : api_(api), physical_device_(physical_device), memory_manager_(*this)
{
    FindQueueFamilyIndices();
    CreateLogicalDevice();

    graphics_queue_ = std::make_unique<VulkanQueue>(*this, graphics_queue_family_index_);
    compute_queue_ = std::make_unique<VulkanQueue>(*this, compute_queue_family_index_);
    transfer_queue_ = std::make_unique<VulkanQueue>(*this, transfer_queue_family_index_);
}

VulkanDevice::~VulkanDevice()
{
    ClearSamplerCache();
    graphics_queue_.reset();
    compute_queue_.reset();
    transfer_queue_.reset();

    if (device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device_, nullptr);
    }
}

void VulkanDevice::FindQueueFamilyIndices()
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        VkQueueFlags flags = queue_families[i].queueFlags;

        if (graphics_queue_family_index_ == UINT32_MAX && (flags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            graphics_queue_family_index_ = i;
        }

        if (compute_queue_family_index_ == UINT32_MAX && (flags & VK_QUEUE_COMPUTE_BIT) != 0)
        {
            compute_queue_family_index_ = i;
        }

        if (transfer_queue_family_index_ == UINT32_MAX && (flags & VK_QUEUE_TRANSFER_BIT) != 0)
        {
            transfer_queue_family_index_ = i;
        }
    }

    THROW_IF(graphics_queue_family_index_ == UINT32_MAX, "No Vulkan graphics queue family");
    THROW_IF(compute_queue_family_index_ == UINT32_MAX, "No Vulkan compute queue family");
    THROW_IF(transfer_queue_family_index_ == UINT32_MAX, "No Vulkan transfer queue family");
}

void VulkanDevice::CreateLogicalDevice()
{
    std::array<uint32_t, 3> raw_indices = {
        graphics_queue_family_index_,
        compute_queue_family_index_,
        transfer_queue_family_index_,
    };

    std::vector<uint32_t> queue_family_indices;
    for (uint32_t index : raw_indices)
    {
        if (std::find(queue_family_indices.begin(), queue_family_indices.end(), index) == queue_family_indices.end())
        {
            queue_family_indices.push_back(index);
        }
    }

    float queue_priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(queue_family_indices.size());

    for (uint32_t queue_family_index : queue_family_indices)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    std::vector<char const*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
    dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_features.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing_features.pNext = &dynamic_rendering_features;
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &descriptor_indexing_features;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    device_create_info.ppEnabledExtensionNames = extensions.data();

    VkResult status = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan device");
}

BufferPtr VulkanDevice::CreateBuffer(std::size_t size, std::uint32_t stride, BufferFlags flags)
{
    return std::make_shared<VulkanBuffer>(*this, size, stride, flags);
}

ImagePtr VulkanDevice::CreateImage(uint32_t width, uint32_t height, ImageFormat format, ImageFlags flags,
    uint32_t mip_count, uint32_t array_size)
{
    return std::make_shared<VulkanImage>(*this, width, height, format, mip_count, array_size, flags);
}

SamplerPtr VulkanDevice::CreateSampler(SamplerDesc const& desc)
{
    return std::make_shared<VulkanSampler>(*this, desc);
}

Queue& VulkanDevice::GetQueue(QueueType queue_type)
{
    switch (queue_type)
    {
    case QueueType::kGraphics:
        return *graphics_queue_;
    case QueueType::kCompute:
        return *compute_queue_;
    case QueueType::kTransfer:
        return *transfer_queue_;
    default:
        throw std::runtime_error("Unsupported Vulkan queue type");
    }
}

GraphicsPipelinePtr VulkanDevice::CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc)
{
    return std::make_unique<VulkanGraphicsPipeline>(*this, pipeline_desc);
}

ComputePipelinePtr VulkanDevice::CreateComputePipeline(char const* cs_filename)
{
    return std::make_unique<VulkanComputePipeline>(*this, cs_filename);
}

SwapchainPtr VulkanDevice::CreateSwapchain(void* window_native_handle, std::uint32_t width, std::uint32_t height,
    std::uint32_t image_count)
{
    return std::make_unique<VulkanSwapchain>(*this, window_native_handle, width, height, image_count);
}

ImGuiRendererPtr VulkanDevice::CreateImGuiRenderer(void* glfw_window, Swapchain& swapchain)
{
    return std::make_unique<VulkanImGuiRenderer>(*this, glfw_window, swapchain);
}

}  // namespace gpu
