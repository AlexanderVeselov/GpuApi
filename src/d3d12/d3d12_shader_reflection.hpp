#pragma once

#include "../common/shader_reflection.hpp"

struct ID3D12ShaderReflection;

namespace gpu
{
ShaderReflection BuildD3D12ShaderReflection(ID3D12ShaderReflection* reflection);
}
