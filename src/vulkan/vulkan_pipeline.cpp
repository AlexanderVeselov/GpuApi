#include "vulkan_pipeline.hpp"

#include "vulkan_api.hpp"
#include "vulkan_descriptor_set.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_shader_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace gpu
{
namespace
{
class VulkanShaderModule
{
public:
    VulkanShaderModule(VulkanDevice& device, std::vector<uint32_t> const& spirv) : device_(device)
    {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = spirv.size() * sizeof(uint32_t);
        create_info.pCode = spirv.data();

        VkResult status = vkCreateShaderModule(device_.GetDevice(), &create_info, nullptr, &shader_module_);
        VK_THROW_IF_FAILED(status, "Failed to create Vulkan shader module");
    }

    ~VulkanShaderModule()
    {
        if (shader_module_ != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(device_.GetDevice(), shader_module_, nullptr);
        }
    }

    VkShaderModule GetShaderModule() const { return shader_module_; }

private:
    VulkanDevice& device_;
    VkShaderModule shader_module_ = VK_NULL_HANDLE;
};

VkCompareOp DepthFuncToVkCompareOp(DepthFunc depth_func)
{
    switch (depth_func)
    {
    case DepthFunc::kNone:
        return VK_COMPARE_OP_ALWAYS;
    case DepthFunc::kNever:
        return VK_COMPARE_OP_NEVER;
    case DepthFunc::kLess:
        return VK_COMPARE_OP_LESS;
    case DepthFunc::kEqual:
        return VK_COMPARE_OP_EQUAL;
    case DepthFunc::kLessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case DepthFunc::kGreater:
        return VK_COMPARE_OP_GREATER;
    case DepthFunc::kNotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case DepthFunc::kGreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case DepthFunc::kAlways:
        return VK_COMPARE_OP_ALWAYS;
    default:
        assert(false && "DepthFuncToVkCompareOp: unknown depth func");
        return VK_COMPARE_OP_ALWAYS;
    }
}

uint32_t FormatSize(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::kR32_Float:
    case ImageFormat::kR32_UInt:
    case ImageFormat::kR32_SInt:
        return 4;
    case ImageFormat::kRG32_Float:
    case ImageFormat::kRG32_UInt:
    case ImageFormat::kRG32_SInt:
        return 8;
    case ImageFormat::kRGB32_Float:
    case ImageFormat::kRGB32_UInt:
    case ImageFormat::kRGB32_SInt:
        return 12;
    case ImageFormat::kRGBA32_Float:
    case ImageFormat::kRGBA32_UInt:
    case ImageFormat::kRGBA32_SInt:
        return 16;
    default:
        throw std::runtime_error("Unsupported Vulkan vertex input format");
    }
}

void GetVertexInputDescs(ShaderReflection const& reflection, VkVertexInputBindingDescription& binding_desc,
    std::vector<VkVertexInputAttributeDescription>& attribute_descs)
{
    uint32_t offset = 0;

    for (ShaderInputParameter const& parameter : reflection.input_parameters)
    {
        if (parameter.is_system_value)
        {
            continue;
        }

        VkVertexInputAttributeDescription attribute_desc = {};
        attribute_desc.location = parameter.semantic_index;
        attribute_desc.binding = 0;
        attribute_desc.format = ToVkFormat(parameter.format);
        attribute_desc.offset = offset;
        attribute_descs.push_back(attribute_desc);

        offset += FormatSize(parameter.format);
    }

    binding_desc = {};
    binding_desc.binding = 0;
    binding_desc.stride = offset;
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shader_module)
{
    VkPipelineShaderStageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_info.stage = stage;
    create_info.module = shader_module;
    create_info.pName = "main";
    return create_info;
}
}  // namespace

VulkanPipeline::VulkanPipeline(VulkanDevice& device) : device_(device), layout_(device) {}

VulkanPipeline::~VulkanPipeline()
{
    device_.UnregisterPipeline(this);
    DestroyPipeline();
}

DescriptorSetPtr VulkanPipeline::CreateDescriptorSet()
{
    return std::make_unique<VulkanDescriptorSet>(device_, layout_);
}

void VulkanPipeline::DestroyPipeline()
{
    if (pipeline_ != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device_.GetDevice(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice& device, GraphicsPipelineDesc const& pipeline_desc)
    : GraphicsPipeline(pipeline_desc), VulkanPipeline(device)
{
    Reload();
}

void VulkanGraphicsPipeline::Reload()
{
    VulkanShaderManager& shader_manager = device_.GetApi().GetShaderManager();
    VulkanShader vs_shader = shader_manager.CompileShader(pipeline_desc_.vs_filename.c_str(), "main", "vs_6_0");
    VulkanShader ps_shader = shader_manager.CompileShader(pipeline_desc_.ps_filename.c_str(), "main", "ps_6_0");

    VulkanPipelineLayout new_layout(device_);
    new_layout.Build({&vs_shader.reflection, &ps_shader.reflection});
    if (pipeline_ != VK_NULL_HANDLE && !layout_.IsCompatibleWith(new_layout))
    {
        throw std::runtime_error("VulkanGraphicsPipeline::Reload: shader layout changed");
    }
    if (pipeline_ == VK_NULL_HANDLE)
    {
        layout_.Build({&vs_shader.reflection, &ps_shader.reflection});
    }

    VulkanShaderModule vs_module(device_, vs_shader.spirv);
    VulkanShaderModule ps_module(device_, ps_shader.spirv);

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        GetShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs_module.GetShaderModule()),
        GetShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, ps_module.GetShaderModule()),
    };

    VkVertexInputBindingDescription vertex_binding_desc = {};
    std::vector<VkVertexInputAttributeDescription> vertex_attribute_descs;
    GetVertexInputDescs(vs_shader.reflection, vertex_binding_desc, vertex_attribute_descs);
    vertex_stride_ = vertex_binding_desc.stride;

    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = vertex_attribute_descs.empty() ? 0u : 1u;
    vertex_input.pVertexBindingDescriptions = vertex_attribute_descs.empty() ? nullptr : &vertex_binding_desc;
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descs.size());
    vertex_input.pVertexAttributeDescriptions = vertex_attribute_descs.empty() ? nullptr
                                                                               : vertex_attribute_descs.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = pipeline_desc_.depth_enabled;
    depth_stencil_state.depthWriteEnable = pipeline_desc_.depth_enabled;
    depth_stencil_state.depthCompareOp = DepthFuncToVkCompareOp(pipeline_desc_.depth_func);
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = VK_FALSE;

    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments(pipeline_desc_.color_attachment_formats
            .size());
    for (VkPipelineColorBlendAttachmentState& attachment : color_blend_attachments)
    {
        attachment.blendEnable = VK_FALSE;
        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.attachmentCount = static_cast<uint32_t>(color_blend_attachments.size());
    color_blend_state.pAttachments = color_blend_attachments.empty() ? nullptr : color_blend_attachments.data();

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    std::vector<VkFormat> color_attachment_formats;
    color_attachment_formats.reserve(pipeline_desc_.color_attachment_formats.size());
    for (ImageFormat format : pipeline_desc_.color_attachment_formats)
    {
        color_attachment_formats.push_back(ToVkFormat(format));
    }

    VkPipelineRenderingCreateInfo rendering_create_info = {};
    rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_create_info.colorAttachmentCount = static_cast<uint32_t>(color_attachment_formats.size());
    rendering_create_info.pColorAttachmentFormats = color_attachment_formats.empty() ? nullptr
                                                                                     : color_attachment_formats.data();
    rendering_create_info.depthAttachmentFormat = pipeline_desc_.depth_enabled
        ? ToVkFormat(pipeline_desc_.depth_attachment_format)
        : VK_FORMAT_UNDEFINED;
    rendering_create_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = &rendering_create_info;
    create_info.stageCount = 2;
    create_info.pStages = shader_stages;
    create_info.pVertexInputState = &vertex_input;
    create_info.pInputAssemblyState = &input_assembly;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pMultisampleState = &multisample_state;
    create_info.pDepthStencilState = &depth_stencil_state;
    create_info.pColorBlendState = &color_blend_state;
    create_info.pDynamicState = &dynamic_state;
    create_info.layout = layout_.GetPipelineLayout();
    create_info.renderPass = VK_NULL_HANDLE;
    create_info.subpass = 0;

    VkPipeline new_pipeline = VK_NULL_HANDLE;
    VkResult status = vkCreateGraphicsPipelines(device_.GetDevice(),
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &new_pipeline);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan graphics pipeline");

    DestroyPipeline();
    pipeline_ = new_pipeline;
}

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice& device, char const* cs_filename)
    : ComputePipeline(cs_filename), VulkanPipeline(device)
{
    Reload();
}

void VulkanComputePipeline::Reload()
{
    VulkanShader cs_shader = device_.GetApi().GetShaderManager().CompileShader(cs_filename_.c_str(), "main", "cs_6_0");
    VulkanPipelineLayout new_layout(device_);
    new_layout.Build({&cs_shader.reflection});
    if (pipeline_ != VK_NULL_HANDLE && !layout_.IsCompatibleWith(new_layout))
    {
        throw std::runtime_error("VulkanComputePipeline::Reload: shader layout changed");
    }
    if (pipeline_ == VK_NULL_HANDLE)
    {
        layout_.Build({&cs_shader.reflection});
    }

    VulkanShaderModule cs_module(device_, cs_shader.spirv);

    VkComputePipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.stage = GetShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, cs_module.GetShaderModule());
    create_info.layout = layout_.GetPipelineLayout();

    VkPipeline new_pipeline = VK_NULL_HANDLE;
    VkResult status = vkCreateComputePipelines(device_.GetDevice(),
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &new_pipeline);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan compute pipeline");

    DestroyPipeline();
    pipeline_ = new_pipeline;
}

}  // namespace gpu
