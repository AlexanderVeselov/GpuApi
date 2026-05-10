#pragma once

#include "vulkan_shared_object.hpp"
#include "vulkan_memory_manager.hpp"
#include <vulkan/vulkan.h>
#include <vector>

class VulkanApi;
class VulkanSwapchain;
class VulkanShader;
class VulkanGraphicsPipelineState;
class VulkanGraphicsPipeline;
class VulkanCommandBuffer;
class VulkanBuffer;
class VulkanImage;
class VulkanMemoryManager;

class VulkanDevice
{
public:
    VulkanDevice(VulkanApi & video_api, VkPhysicalDevice physical_device, std::vector<char const*> const& enabled_layer_names, std::vector<char const*> const& enabled_extension_names, VulkanSharedObject<VkSurfaceKHR> surface);

    uint32_t GetGraphicsQueueFamilyIndex() const;
    uint32_t GetComputeQueueFamilyIndex() const;
    uint32_t GetTransferQueueFamilyIndex() const;
    uint32_t GetPresentQueueFamilyIndex() const;

    VkPhysicalDevice GetPhysicalDevice() const;
    VkDevice GetDevice() const;
    VkSurfaceKHR GetSurface() const;
    VkDescriptorPool GetDescriptorPool() const { return descriptor_pool_; }

    std::shared_ptr<VulkanSwapchain> CreateSwapchain(uint32_t width, uint32_t height);
    std::shared_ptr<VulkanShader> CreateVertexShader(std::string const& filename);
    std::shared_ptr<VulkanShader> CreatePixelShader(std::string const& filename);
    std::shared_ptr<VulkanGraphicsPipeline> CreateGraphicsPipeline(VulkanGraphicsPipelineState const& pipeline_state);
    std::shared_ptr<VulkanCommandBuffer> CreateGraphicsCommandBuffer();
    std::shared_ptr<VulkanBuffer> CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
    std::shared_ptr<VulkanImage> CreateImage(VkImage image, VkFormat format);
    
    void SubmitGraphicsCommandBuffer(std::shared_ptr<VulkanCommandBuffer> command_buffer);
    void GraphicsWaitIdle();

    VulkanMemoryManager & GetMemoryManager() { return memory_manager_; }

private:
    VulkanApi & video_api_;
    VulkanMemoryManager memory_manager_;

    void FindQueueFamilyIndices();
    void CreateLogicalDevice(std::vector<char const*> const& enabled_layer_names, std::vector<char const*> const& enabled_extension_names);
    void CreateCommandPools();
    void CreateDescriptorPool();

    VkPhysicalDevice physical_device_;
    VulkanSharedObject<VkDevice> logical_device_;
    VulkanSharedObject<VkSurfaceKHR> surface_;

    uint32_t graphics_queue_family_index_;
    uint32_t compute_queue_family_index_;
    uint32_t transfer_queue_family_index_;
    uint32_t present_queue_family_index_;

    VulkanSharedObject<VkCommandPool> graphics_command_pool_;
    VulkanSharedObject<VkCommandPool> compute_command_pool_;
    VulkanSharedObject<VkCommandPool> transfer_command_pool_;

    VulkanScopedObject<VkDescriptorPool, vkCreateDescriptorPool, vkDestroyDescriptorPool> descriptor_pool_;

};
