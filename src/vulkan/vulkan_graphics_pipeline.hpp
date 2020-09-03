#ifndef VULKAN_GRAPHICS_PIPELINE_HPP_
#define VULKAN_GRAPHICS_PIPELINE_HPP_

#include "vulkan_shared_object.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>
#include <memory>

class VulkanImage;
class VulkanBuffer;

class VulkanGraphicsPipelineState
{
public:
    VulkanGraphicsPipelineState(std::uint32_t width, std::uint32_t height)
        : width_(width)
        , height_(height)
    {}

    void SetVertexShader(std::shared_ptr<VulkanShader> vertex_shader) { vertex_shader_ = vertex_shader; }
    void SetPixelShader(std::shared_ptr<VulkanShader> pixel_shader) { pixel_shader_ = pixel_shader; }
    void SetColorAttachment(std::uint32_t id, std::shared_ptr<VulkanImage> image)
    {
        if (color_attachments_.size() < id + 1)
        {
            color_attachments_.resize(id + 1);
        }

        color_attachments_[id] = image;

    }

private:
    friend class VulkanGraphicsPipeline;

    std::uint32_t width_;
    std::uint32_t height_;
    std::shared_ptr<VulkanShader> vertex_shader_;
    std::shared_ptr<VulkanShader> pixel_shader_;
    std::vector<std::shared_ptr<VulkanImage>> color_attachments_;

};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(VulkanDevice & device, VulkanGraphicsPipelineState const& pipeline_state);
    void CommitArgs();
    void SetArg(std::uint32_t set, std::uint32_t binding, std::shared_ptr<VulkanBuffer> buffer);

    VkRenderPass GetRenderPass() const { return render_pass_; }
    VkPipeline GetPipeline() const { return pipeline_; }
    VkFramebuffer GetFramebuffer() const { return framebuffer_; }
    VkExtent2D GetExtent() const { return extent_; }
    VkPipelineLayout GetLayout() const { return pipeline_layout_; }
    std::unordered_map<std::uint32_t, VulkanShader::DescriptorSet> const& GetDescriptorSets() const { return descriptor_sets_; }

private:
    VulkanDevice & device_;
    VulkanGraphicsPipelineState pipeline_state_;
    VulkanScopedObject<VkPipelineLayout, vkCreatePipelineLayout, vkDestroyPipelineLayout> pipeline_layout_;
    VulkanScopedObject<VkRenderPass, vkCreateRenderPass, vkDestroyRenderPass> render_pass_;
    VulkanScopedObject<VkPipeline, vkCreateGraphicsPipelines, vkDestroyPipeline> pipeline_;
    VulkanScopedObject<VkFramebuffer, vkCreateFramebuffer, vkDestroyFramebuffer>  framebuffer_;
    std::unordered_map<std::uint32_t, VulkanShader::DescriptorSet> descriptor_sets_;
    VkExtent2D extent_;

};

#endif // VULKAN_GRAPHICS_PIPELINE_HPP_
