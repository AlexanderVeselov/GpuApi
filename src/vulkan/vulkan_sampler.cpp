#include "vulkan_sampler.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

namespace gpu
{
namespace
{
    VkFilter ToVkFilter(SamplerFilter filter)
    {
        switch (filter)
        {
        case SamplerFilter::kNearest:
            return VK_FILTER_NEAREST;
        case SamplerFilter::kLinear:
            return VK_FILTER_LINEAR;
        }

        return VK_FILTER_LINEAR;
    }

    VkSamplerAddressMode ToVkAddressMode(SamplerAddressMode mode)
    {
        switch (mode)
        {
        case SamplerAddressMode::kRepeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerAddressMode::kClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }

        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }

    VkCompareOp ToVkCompareOp(SamplerComparisonFunc func)
    {
        switch (func)
        {
        case SamplerComparisonFunc::kNever:
            return VK_COMPARE_OP_NEVER;
        case SamplerComparisonFunc::kLess:
            return VK_COMPARE_OP_LESS;
        case SamplerComparisonFunc::kEqual:
            return VK_COMPARE_OP_EQUAL;
        case SamplerComparisonFunc::kLessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case SamplerComparisonFunc::kGreater:
            return VK_COMPARE_OP_GREATER;
        case SamplerComparisonFunc::kNotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case SamplerComparisonFunc::kGreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case SamplerComparisonFunc::kAlways:
            return VK_COMPARE_OP_ALWAYS;
        case SamplerComparisonFunc::kNone:
            return VK_COMPARE_OP_ALWAYS;
        }

        return VK_COMPARE_OP_ALWAYS;
    }
}  // namespace

VulkanSampler::VulkanSampler(VulkanDevice& device, SamplerDesc const& desc) : Sampler(desc), device_(device)
{
    VkSamplerCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = ToVkFilter(desc.mag_filter);
    create_info.minFilter = ToVkFilter(desc.min_filter);
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.addressModeU = ToVkAddressMode(desc.address_u);
    create_info.addressModeV = ToVkAddressMode(desc.address_v);
    create_info.addressModeW = ToVkAddressMode(desc.address_w);
    create_info.mipLodBias = desc.mip_lod_bias;
    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy = 1.0f;
    create_info.compareEnable = desc.comparison_func != SamplerComparisonFunc::kNone ? VK_TRUE : VK_FALSE;
    create_info.compareOp = ToVkCompareOp(desc.comparison_func);
    create_info.minLod = 0.0f;
    create_info.maxLod = VK_LOD_CLAMP_NONE;
    create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;

    VkResult status = vkCreateSampler(device_.GetDevice(), &create_info, nullptr, &sampler_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan sampler");
}

VulkanSampler::~VulkanSampler()
{
    if (sampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_.GetDevice(), sampler_, nullptr);
    }
}

}  // namespace gpu
