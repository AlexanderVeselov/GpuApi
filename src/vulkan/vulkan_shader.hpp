#ifndef VULKAN_SHADER_HPP_
#define VULKAN_SHADER_HPP_

#include "vulkan_device.hpp"
#include "vulkan_shared_object.hpp"
#include <string>
#include <vulkan/vulkan.h>
#include <unordered_map>

namespace spirv_cross
{
    class Compiler;
    struct ShaderResources;
}

class VulkanShader
{
public:
    enum ShaderType
    {
        kVertex,
        kPixel,
        kGeometry,
        kCompute
    };

    struct DescriptorSet
    {
        struct Binding
        {
            union
            {
                VkDescriptorImageInfo image_info;
                VkDescriptorBufferInfo buffer_info;
            };

            VkDescriptorSetLayoutBinding vk_binding;
        };

        std::unordered_map<std::uint32_t, Binding> bindings;
        VulkanScopedObject<VkDescriptorSetLayout, vkCreateDescriptorSetLayout, vkDestroyDescriptorSetLayout> layout;
        VkDescriptorSet vk_descriptor_set;
    };

    VulkanShader(VulkanDevice & device, ShaderType shader_type, std::string const& filename);
    VkShaderModule GetShaderModule() const;
    std::vector<VkVertexInputAttributeDescription> const& GetVertexInputAttributeDescriptions() const { return vertex_input_attribute_descs_; }
    std::vector<VkVertexInputBindingDescription> const& GetVertexInputBindingDescriptions() const { return vertex_input_binding_desc_; }
    std::unordered_map<std::uint32_t, DescriptorSet> const& GetDescriptorSets() const { return descriptor_sets_; }

private:
    void FillVertexInputDescriptions(spirv_cross::Compiler const& compiler, spirv_cross::ShaderResources const& resources);
    void CreateDescriptorSets(spirv_cross::Compiler const& compiler, spirv_cross::ShaderResources const& resources);

    VulkanDevice & device_;
    ShaderType shader_type_;
    VulkanScopedObject<VkShaderModule, vkCreateShaderModule, vkDestroyShaderModule> shader_module_;
    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descs_;
    std::vector<VkVertexInputBindingDescription> vertex_input_binding_desc_;

    std::unordered_map<std::uint32_t, DescriptorSet> descriptor_sets_;

};

#endif // VULKAN_SHADER_HPP_
