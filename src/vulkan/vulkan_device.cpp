#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_validation.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_image.hpp"
#include "vulkan_memory_manager.hpp"
#include <vector>
#include <algorithm>
#include <cassert>

namespace
{
    const std::uint32_t kInvalidQueueFamilyIndex = ~0u;
}

VulkanDevice::VulkanDevice(VulkanApi & video_api,
                           VkPhysicalDevice physical_device,
                           std::vector<char const*> const& enabled_layer_names,
                           std::vector<char const*> const& enabled_extension_names,
                           VulkanSharedObject<VkSurfaceKHR> surface)
    : video_api_(video_api)
    , memory_manager_(*this)
    , physical_device_(physical_device)
    , surface_(surface)
    , graphics_queue_family_index_(kInvalidQueueFamilyIndex)
    , compute_queue_family_index_(kInvalidQueueFamilyIndex)
    , transfer_queue_family_index_(kInvalidQueueFamilyIndex)
    , present_queue_family_index_(kInvalidQueueFamilyIndex)
{
    FindQueueFamilyIndices();
    CreateLogicalDevice(enabled_layer_names, enabled_extension_names);
    CreateCommandPools();
    CreateDescriptorPool();
}

void VulkanDevice::FindQueueFamilyIndices()
{
    // Get physical device queue families count
    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);

    // Get queue families properties
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

    // Get graphics, compute and transfer queue family indices
    for (std::uint32_t i = 0; i < queue_families.size(); ++i)
    {
        if (graphics_queue_family_index_ == kInvalidQueueFamilyIndex && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphics_queue_family_index_ = i;
        }

        if (compute_queue_family_index_ == kInvalidQueueFamilyIndex && queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            compute_queue_family_index_ = i;
        }

        if (transfer_queue_family_index_ == kInvalidQueueFamilyIndex && queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            transfer_queue_family_index_ = i;
        }

        if (surface_ && present_queue_family_index_ == kInvalidQueueFamilyIndex)
        {
            VkBool32 present_support = false;
            VkResult status = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_,
                i, surface_.get(), &present_support);
            VK_THROW_IF_FAILED(status, "Failed to get surface support status!");

            if (present_support)
            {
                present_queue_family_index_ = i;
            }
        }

    }

    // Checks
    THROW_IF(graphics_queue_family_index_ == kInvalidQueueFamilyIndex, "Failed to get graphics queue family index!");
    THROW_IF(compute_queue_family_index_ == kInvalidQueueFamilyIndex, "Failed to get compute queue family index!");
    THROW_IF(transfer_queue_family_index_ == kInvalidQueueFamilyIndex, "Failed to get transfer queue family index!");
    THROW_IF(surface_ && present_queue_family_index_ == kInvalidQueueFamilyIndex, "Failed to get present queue family index!");

}

void VulkanDevice::CreateLogicalDevice(std::vector<char const*> const& enabled_layer_names, std::vector<char const*> const& enabled_extension_names)
{
    std::vector<std::uint32_t> queue_family_indices;
    std::vector<std::uint32_t> queue_counts;
    queue_family_indices.push_back(graphics_queue_family_index_);
    // CHECK: graphics queue count
    queue_counts.push_back(1);

    if (compute_queue_family_index_!= graphics_queue_family_index_)
    {
        queue_family_indices.push_back(compute_queue_family_index_);
        // CHECK: compute queue count
        queue_counts.push_back(1);
    }

    if (transfer_queue_family_index_ != graphics_queue_family_index_ && transfer_queue_family_index_ != compute_queue_family_index_)
    {
        queue_family_indices.push_back(transfer_queue_family_index_);
        // CHECK: transfer queue count
        queue_counts.push_back(1);
    }

    if (surface_)
    {
        if (present_queue_family_index_ != graphics_queue_family_index_ && present_queue_family_index_ != compute_queue_family_index_ && present_queue_family_index_ != transfer_queue_family_index_)
        {
            queue_family_indices.push_back(present_queue_family_index_);
            // One present queue
            queue_counts.push_back(1);
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_indices.size());
    std::vector<float> queue_priorities(*std::max_element(queue_counts.begin(), queue_counts.end()), 1.0f);
    for (std::uint32_t i = 0; i < queue_create_infos.size(); ++i)
    {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].pNext = nullptr;
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].queueFamilyIndex = queue_family_indices[i];
        queue_create_infos[i].queueCount = queue_counts[i];
        queue_create_infos[i].pQueuePriorities = queue_priorities.data();
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    device_create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();

    device_create_info.enabledLayerCount = static_cast<std::uint32_t>(enabled_layer_names.size());
    device_create_info.ppEnabledLayerNames = enabled_layer_names.data();

    device_create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extension_names.size());
    device_create_info.ppEnabledExtensionNames = enabled_extension_names.data();

    VkDevice device = nullptr;
    VkResult status = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device);
    VK_THROW_IF_FAILED(status, "Failed to create VkDevice!");

    logical_device_.reset(device, [](VkDevice device)
    {
        std::cout << "Destroying logical device" << std::endl;
        vkDestroyDevice(device, nullptr);
    });

}

void VulkanDevice::CreateCommandPools()
{
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    VkCommandPool command_pool;
    VkDevice device = GetDevice();
    VkResult status;
    // Graphics command pool
    cmd_pool_create_info.queueFamilyIndex = GetGraphicsQueueFamilyIndex();
    status = vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &command_pool);
    VK_THROW_IF_FAILED(status, "Failed to create graphics command pool!");
    graphics_command_pool_.reset(command_pool, [device](VkCommandPool command_pool)
    {
        vkDestroyCommandPool(device, command_pool, nullptr);
    });

    // Compute command pool
    cmd_pool_create_info.queueFamilyIndex = GetComputeQueueFamilyIndex();
    status = vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &command_pool);
    VK_THROW_IF_FAILED(status, "Failed to create compute command pool!");
    compute_command_pool_.reset(command_pool, [device](VkCommandPool command_pool)
    {
        vkDestroyCommandPool(device, command_pool, nullptr);
    });

    // Transfer command pool
    cmd_pool_create_info.queueFamilyIndex = GetTransferQueueFamilyIndex();
    status = vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &command_pool);
    VK_THROW_IF_FAILED(status, "Failed to create transfer command pool!");
    transfer_command_pool_.reset(command_pool, [device](VkCommandPool command_pool)
    {
        vkDestroyCommandPool(device, command_pool, nullptr);
    });

}

void VulkanDevice::CreateDescriptorPool()
{
    const std::uint32_t kMaxDescriptorCount = 128;
    const std::uint32_t kMaxDescriptorSets = 128;

    VkDescriptorPoolSize descriptor_pool_sizes[] =
    {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxDescriptorCount}
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]);
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    descriptor_pool_create_info.maxSets = kMaxDescriptorSets;

    VkResult status = descriptor_pool_.Create(GetDevice(), descriptor_pool_create_info);
    VK_THROW_IF_FAILED(status, "Failed to create descriptor pool!");

}

std::uint32_t VulkanDevice::GetGraphicsQueueFamilyIndex() const
{
    return graphics_queue_family_index_;
};

std::uint32_t VulkanDevice::GetComputeQueueFamilyIndex() const
{
    return compute_queue_family_index_;
}

std::uint32_t VulkanDevice::GetTransferQueueFamilyIndex() const
{
    return transfer_queue_family_index_;
}

std::uint32_t VulkanDevice::GetPresentQueueFamilyIndex() const
{
    return present_queue_family_index_;
}

VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
{
    return physical_device_;
}

VkDevice VulkanDevice::GetDevice() const
{
    return logical_device_.get();
}

VkSurfaceKHR VulkanDevice::GetSurface() const
{
    return surface_.get();
}

std::shared_ptr<VulkanSwapchain> VulkanDevice::CreateSwapchain(std::uint32_t width, std::uint32_t height)
{
    assert(surface_);
    return std::make_shared<VulkanSwapchain>(*this, width, height);
}

std::shared_ptr<VulkanShader> VulkanDevice::CreateVertexShader(std::string const& filename)
{
    return std::make_shared<VulkanShader>(*this, VulkanShader::kVertex, filename);
}

std::shared_ptr<VulkanShader> VulkanDevice::CreatePixelShader(std::string const& filename)
{
    return std::make_shared<VulkanShader>(*this, VulkanShader::kPixel, filename);
}

std::shared_ptr<VulkanGraphicsPipeline> VulkanDevice::CreateGraphicsPipeline(VulkanGraphicsPipelineState const& pipeline_state)
{
    return std::make_shared<VulkanGraphicsPipeline>(*this, pipeline_state);
}

std::shared_ptr<VulkanCommandBuffer> VulkanDevice::CreateGraphicsCommandBuffer()
{
    return std::make_shared<VulkanCommandBuffer>(*this, graphics_command_pool_.get());
}

std::shared_ptr<VulkanBuffer> VulkanDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
{
    return std::make_shared<VulkanBuffer>(*this, size, usage);
}

std::shared_ptr<VulkanImage> VulkanDevice::CreateImage(VkImage image, VkFormat format)
{
    return std::make_shared<VulkanImage>(*this, image, format);
}

void VulkanDevice::SubmitGraphicsCommandBuffer(std::shared_ptr<VulkanCommandBuffer> command_buffer)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    VkCommandBuffer cmd_buffer = command_buffer->GetCommandBuffer();
    submitInfo.pCommandBuffers = &cmd_buffer;

    VkQueue graphics_queue;
    vkGetDeviceQueue(GetDevice(), GetGraphicsQueueFamilyIndex(), 0, &graphics_queue);

    VkResult status = vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    VK_THROW_IF_FAILED(status, "Failed to submit command buffer!");

}

void VulkanDevice::GraphicsWaitIdle()
{
    VkQueue graphics_queue;
    vkGetDeviceQueue(GetDevice(), GetGraphicsQueueFamilyIndex(), 0, &graphics_queue);

    VkResult status = vkQueueWaitIdle(graphics_queue);
    VK_THROW_IF_FAILED(status, "Failed to wait idle!");

}
