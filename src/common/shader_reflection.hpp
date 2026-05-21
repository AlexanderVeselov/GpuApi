#pragma once

#include "gpu_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gpu
{
enum class ShaderStage : uint32_t
{
    kVertex = 1 << 0,
    kPixel = 1 << 1,
    kGeometry = 1 << 2,
    kHull = 1 << 3,
    kDomain = 1 << 4,
    kCompute = 1 << 5
};

enum class ShaderResourceType
{
    kBuffer,
    kImage,
    kSampler,
    kAccelerationStructure
};

enum class ShaderDescriptorType
{
    kRootConstant,
    kDescriptorTable
};

enum class ShaderDescriptorRangeType
{
    kCBV,
    kSRV,
    kUAV,
    kSampler
};

struct ShaderBinding
{
    std::string name;
    uint32_t binding = 0;
    uint32_t space = 0;
    ShaderResourceType resource_type = ShaderResourceType::kBuffer;
    ShaderDescriptorType descriptor_type = ShaderDescriptorType::kDescriptorTable;
    ShaderDescriptorRangeType range_type = ShaderDescriptorRangeType::kCBV;
    uint32_t descriptor_count = 1;
    uint32_t num_32bit_values = 0;
    uint32_t stage_mask = 0;
};

struct ShaderInputParameter
{
    std::string semantic_name;
    uint32_t semantic_index = 0;
    ImageFormat format = ImageFormat::kUnknown;
    bool is_system_value = false;
};

struct ShaderReflection
{
    ShaderStage stage = ShaderStage::kVertex;
    std::vector<ShaderBinding> bindings;
    std::vector<ShaderInputParameter> input_parameters;
};

}  // namespace gpu
