#include "vulkan_descriptor_set.hpp"

#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_sampler.hpp"

#include <stdexcept>
#include <string>

namespace gpu
{
namespace
{
    std::string BindingName(uint32_t binding, uint32_t space)
    {
        return "(binding = " + std::to_string(binding) + ", space = " + std::to_string(space) + ")";
    }

    VkImageLayout GetDescriptorImageLayout(VulkanBinding const& binding)
    {
        if (binding.vk_descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        {
            return VK_IMAGE_LAYOUT_GENERAL;
        }

        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}  // namespace

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice& device, VulkanPipelineLayout const& layout)
    : device_(device), layout_(layout)
{
    CreateDescriptorPool();
    AllocateDescriptorSets();
}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
    Clear();
}

void VulkanDescriptorSet::BindBuffer(Buffer& buffer, uint32_t binding, uint32_t space)
{
    auto* vulkan_buffer = dynamic_cast<VulkanBuffer*>(&buffer);
    THROW_IF(!vulkan_buffer, "Buffer does not belong to the Vulkan backend");

    VulkanBinding const& vulkan_binding = FindBinding(binding, space);
    if (vulkan_binding.resource_type != ShaderResourceType::kBuffer)
    {
        throw std::runtime_error("VulkanDescriptorSet::BindBuffer: pipeline binding " + BindingName(binding, space)
            + " is not a buffer binding");
    }

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = vulkan_buffer->GetBuffer();
    buffer_info.offset = 0;
    buffer_info.range = vulkan_buffer->GetSize();

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_sets_[vulkan_binding.set];
    write.dstBinding = vulkan_binding.binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = vulkan_binding.vk_descriptor_type;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device_.GetDevice(), 1, &write, 0, nullptr);
}

void VulkanDescriptorSet::BindImage(Image& image, uint32_t binding, uint32_t space)
{
    BindImage(image, ImageView{}, binding, space);
}

void VulkanDescriptorSet::BindImage(Image& image, ImageView const& view, uint32_t binding, uint32_t space)
{
    auto* vulkan_image = dynamic_cast<VulkanImage*>(&image);
    THROW_IF(!vulkan_image, "Image does not belong to the Vulkan backend");

    VulkanBinding const& vulkan_binding = FindBinding(binding, space);
    if (vulkan_binding.resource_type != ShaderResourceType::kImage)
    {
        throw std::runtime_error("VulkanDescriptorSet::BindImage: pipeline binding " + BindingName(binding, space)
            + " is not an image binding");
    }

    VkDescriptorImageInfo image_info = {};
    image_info.imageView = vulkan_image->GetView(view);
    image_info.imageLayout = GetDescriptorImageLayout(vulkan_binding);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_sets_[vulkan_binding.set];
    write.dstBinding = vulkan_binding.binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = vulkan_binding.vk_descriptor_type;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(device_.GetDevice(), 1, &write, 0, nullptr);
}

void VulkanDescriptorSet::BindSampler(Sampler& sampler, uint32_t binding, uint32_t space)
{
    auto* vulkan_sampler = dynamic_cast<VulkanSampler*>(&sampler);
    THROW_IF(!vulkan_sampler, "Sampler does not belong to the Vulkan backend");

    VulkanBinding const& vulkan_binding = FindBinding(binding, space);
    if (vulkan_binding.resource_type != ShaderResourceType::kSampler)
    {
        throw std::runtime_error("VulkanDescriptorSet::BindSampler: pipeline binding " + BindingName(binding, space)
            + " is not a sampler binding");
    }

    VkDescriptorImageInfo sampler_info = {};
    sampler_info.sampler = vulkan_sampler->GetSampler();

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_sets_[vulkan_binding.set];
    write.dstBinding = vulkan_binding.binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = vulkan_binding.vk_descriptor_type;
    write.pImageInfo = &sampler_info;

    vkUpdateDescriptorSets(device_.GetDevice(), 1, &write, 0, nullptr);
}

void VulkanDescriptorSet::Clear()
{
    descriptor_sets_.clear();

    if (descriptor_pool_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device_.GetDevice(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
}

VulkanBinding const& VulkanDescriptorSet::FindBinding(uint32_t binding, uint32_t space) const
{
    for (VulkanBinding const& vulkan_binding : layout_.GetBindings())
    {
        if (vulkan_binding.binding == binding && vulkan_binding.set == space)
        {
            return vulkan_binding;
        }
    }

    throw std::runtime_error("VulkanDescriptorSet: pipeline layout binding " + BindingName(binding, space)
        + " was not found");
}

void VulkanDescriptorSet::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> pool_sizes;

    for (VulkanBinding const& binding : layout_.GetBindings())
    {
        if (binding.descriptor_type != ShaderDescriptorType::kDescriptorTable)
        {
            continue;
        }

        VkDescriptorPoolSize* pool_size = nullptr;
        for (VkDescriptorPoolSize& existing : pool_sizes)
        {
            if (existing.type == binding.vk_descriptor_type)
            {
                pool_size = &existing;
                break;
            }
        }

        if (!pool_size)
        {
            VkDescriptorPoolSize new_pool_size = {};
            new_pool_size.type = binding.vk_descriptor_type;
            pool_sizes.push_back(new_pool_size);
            pool_size = &pool_sizes.back();
        }

        pool_size->descriptorCount += binding.descriptor_count;
    }

    if (pool_sizes.empty())
    {
        return;
    }

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.maxSets = static_cast<uint32_t>(layout_.GetDescriptorSetLayouts().size());
    create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    create_info.pPoolSizes = pool_sizes.data();

    VkResult status = vkCreateDescriptorPool(device_.GetDevice(), &create_info, nullptr, &descriptor_pool_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan descriptor pool");
}

void VulkanDescriptorSet::AllocateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts = layout_.GetDescriptorSetLayouts();
    if (descriptor_set_layouts.empty())
    {
        return;
    }

    descriptor_sets_.resize(descriptor_set_layouts.size(), VK_NULL_HANDLE);

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool_;
    allocate_info.descriptorSetCount = static_cast<uint32_t>(descriptor_set_layouts.size());
    allocate_info.pSetLayouts = descriptor_set_layouts.data();

    VkResult status = vkAllocateDescriptorSets(device_.GetDevice(), &allocate_info, descriptor_sets_.data());
    VK_THROW_IF_FAILED(status, "Failed to allocate Vulkan descriptor sets");
}

}  // namespace gpu
