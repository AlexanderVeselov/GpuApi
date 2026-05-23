#pragma once

#include "../common/shader_reflection.hpp"

#include <cstdint>
#include <vector>

namespace gpu
{
ShaderReflection BuildVulkanShaderReflection(std::vector<uint32_t> const& spirv, char const* root_constants_name);
}
