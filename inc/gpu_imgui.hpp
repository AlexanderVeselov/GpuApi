#pragma once

#include "gpu_types.hpp"

namespace gpu
{
class CommandBuffer;

class ImGuiRenderer
{
  public:
    virtual ~ImGuiRenderer() = default;

    virtual void NewFrame() = 0;
    virtual void Render(CommandBuffer& command_buffer) = 0;
};

} // namespace gpu
