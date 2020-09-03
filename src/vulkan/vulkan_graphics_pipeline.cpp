#include "vulkan_exception.hpp"
#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_image.hpp"
#include "vulkan_buffer.hpp"
#include <cassert>

static std::unordered_map<std::uint32_t, VulkanShader::DescriptorSet> MergeDescriptorSets(
    std::vector<std::shared_ptr<VulkanShader>> const& shader_list)
{
    if (shader_list.empty())
    {
        // Return empty map
        return std::unordered_map<std::uint32_t, VulkanShader::DescriptorSet>();
    }

    auto result = shader_list.front()->GetDescriptorSets();

    for (std::size_t shader_idx = 1; shader_idx < shader_list.size(); ++shader_idx)
    {
        auto const& shader_descriptor_sets = shader_list[shader_idx]->GetDescriptorSets();

        // Merge shader descriptor sets into result
        for (auto const& ds : shader_descriptor_sets)
        {
            // Find descriptor set with the same index in result
            auto result_ds = result.find(ds.first);

            if (result_ds == result.end())
            {
                // Descriptor set doesn't exist in result map
                result.emplace(ds);
                continue;
            }

            for (auto const& binding : ds.second.bindings)
            {
                auto const& result_binding = result_ds->second.bindings.find(binding.first);

                if (result_binding == result_ds->second.bindings.end())
                {
                    // Binding doesn't exist in result descriptor set
                    result_ds->second.bindings.emplace(binding);
                    continue;
                }

                // Don't support using the same bindings in different shader stages for now
                throw std::runtime_error(std::string("MergeDescriptorSets(...): Binding #")
                    + std::to_string(binding.first) + " in descriptor set "
                    + std::to_string(ds.first) + " was already used!");

            }
        }
    }

    return std::move(result);

}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice & device, VulkanGraphicsPipelineState const& pipeline_state)
    : device_(device)
    , pipeline_state_(pipeline_state)
    , extent_({pipeline_state.width_, pipeline_state.height_})
{
    VkPipelineShaderStageCreateInfo vs_stage = {};
    vs_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vs_stage.module = pipeline_state.vertex_shader_->GetShaderModule();
    vs_stage.pName = "main";

    VkPipelineShaderStageCreateInfo ps_stage = {};
    ps_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ps_stage.module = pipeline_state.pixel_shader_->GetShaderModule();
    ps_stage.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] =
    {
        vs_stage,
        ps_stage
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto const& vertex_binding_descriptions = pipeline_state.vertex_shader_->GetVertexInputBindingDescriptions();
    vertex_input_state.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_binding_descriptions.size());
    vertex_input_state.pVertexBindingDescriptions = vertex_binding_descriptions.data();

    auto const& vertex_attribute_descriptions = pipeline_state.vertex_shader_->GetVertexInputAttributeDescriptions();
    vertex_input_state.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_attribute_descriptions.size());
    vertex_input_state.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent_.width;
    viewport.height = (float)extent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = extent_;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment_state;

    // Create descriptor set layout
    descriptor_sets_ = MergeDescriptorSets({ pipeline_state.vertex_shader_, pipeline_state.pixel_shader_ });

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts(descriptor_sets_.size());

    std::size_t ds_index = 0;
    for (auto & ds : descriptor_sets_)
    {
        // TODO: Add empty descriptor sets support
        assert(ds.first == ds_index && "Empty descriptor sets aren't supported!");

        VulkanShader::DescriptorSet & descriptor_set = ds.second;

        VkDescriptorSetLayoutCreateInfo layout_create_info = {};
        layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        // Convert bindings from std::unordered_map to std::vector
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (auto const& binding_it : descriptor_set.bindings)
        {
            bindings.push_back(binding_it.second.vk_binding);
        }

        layout_create_info.pBindings = bindings.data();
        layout_create_info.bindingCount = static_cast<std::uint32_t>(bindings.size());

        VkResult status = descriptor_set.layout.Create(device_.GetDevice(), layout_create_info);
        VK_THROW_IF_FAILED(status, "Failed to create descriptor set layout!");

        descriptor_set_layouts[ds_index] = descriptor_set.layout;

        // Allocate descriptor sets
        VkDescriptorSetAllocateInfo ds_allocate_info = {};
        ds_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ds_allocate_info.descriptorPool = device_.GetDescriptorPool();
        ds_allocate_info.descriptorSetCount = 1;
        ds_allocate_info.pSetLayouts = &descriptor_set.layout;
        status = vkAllocateDescriptorSets(device_.GetDevice(), &ds_allocate_info, &descriptor_set.vk_descriptor_set);
        VK_THROW_IF_FAILED(status, "Failed to allocate descriptor sets!");

        ++ds_index;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = static_cast<std::uint32_t>(descriptor_set_layouts.size());
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();

    VkDevice logical_device = device_.GetDevice();

    VkResult status = pipeline_layout_.Create(logical_device, pipeline_layout_create_info);
    VK_THROW_IF_FAILED(status, "Failed to create pipeline layout!");

    // Create render pass
    VkAttachmentDescription colorAttachment = {};
    // TODO: remove hardcoded parameters
    colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_reference;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &colorAttachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_desc;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    status = render_pass_.Create(logical_device, render_pass_create_info);
    VK_THROW_IF_FAILED(status, "Failed to create render pass!");

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_state;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterization_state;
    pipeline_create_info.pMultisampleState = &multisample_state;
    pipeline_create_info.pColorBlendState = &color_blend_state;
    pipeline_create_info.layout = pipeline_layout_;
    pipeline_create_info.renderPass = render_pass_;
    pipeline_create_info.subpass = 0;

    status = pipeline_.Reset(logical_device, pipeline_create_info);
    VK_THROW_IF_FAILED(status, "Failed to create graphics pipeline!");

    std::vector<VkImageView> attachments;
    for (std::uint32_t i = 0; i < pipeline_state_.color_attachments_.size(); ++i)
    {
        if (pipeline_state_.color_attachments_[i])
        {
            attachments.push_back(pipeline_state_.color_attachments_[i]->GetImageView());
        }
    }

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass_;
    framebuffer_create_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    framebuffer_create_info.pAttachments = attachments.data();
    framebuffer_create_info.width = extent_.width;
    framebuffer_create_info.height = extent_.height;
    framebuffer_create_info.layers = 1;

    status = framebuffer_.Create(logical_device, framebuffer_create_info);
    VK_THROW_IF_FAILED(status, "Failed to create framebuffer!");

}

void VulkanGraphicsPipeline::SetArg(std::uint32_t set, std::uint32_t binding, std::shared_ptr<VulkanBuffer> buffer)
{
    auto set_it = descriptor_sets_.find(set);

    THROW_IF(set_it == descriptor_sets_.end(), std::string("Pipeline doesn't have bound descriptor set ") + std::to_string(set));

    auto binding_it = set_it->second.bindings.find(binding);

    THROW_IF(binding_it == set_it->second.bindings.end(), std::string("Pipeline doesn't use binding ") + std::to_string(binding) + " in descriptor set " + std::to_string(set));

    VulkanShader::DescriptorSet::Binding & b = binding_it->second;

    THROW_IF(b.vk_binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && b.vk_binding.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        std::string("Binding ") + std::to_string(binding) + " in descriptor set " + std::to_string(set) + " is not a buffer binding");

    b.buffer_info.buffer = buffer->GetBuffer();
    b.buffer_info.offset = 0;
    b.buffer_info.range = VK_WHOLE_SIZE;

}

void VulkanGraphicsPipeline::CommitArgs()
{
    std::vector<VkWriteDescriptorSet> descriptor_writes;

    for (auto const& ds : descriptor_sets_)
    {
        for (auto const& b : ds.second.bindings)
        {
            VulkanShader::DescriptorSet::Binding const& binding = b.second;

            VkWriteDescriptorSet descriptor_write = {};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = ds.second.vk_descriptor_set;
            descriptor_write.dstBinding = b.first;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorCount = 1;
            descriptor_write.descriptorType = binding.vk_binding.descriptorType;

            switch (descriptor_write.descriptorType)
            {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                descriptor_write.pBufferInfo = &binding.buffer_info;
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                descriptor_write.pImageInfo = &binding.image_info;
                break;
            default:
                assert(!"Not supported!");
            }

            descriptor_writes.push_back(descriptor_write);
        }
    }

    vkUpdateDescriptorSets(device_.GetDevice(), static_cast<std::uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);

}
