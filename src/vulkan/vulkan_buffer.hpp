#pragma once

#include "gpu_buffer.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{
class VulkanDevice;

class VulkanBuffer final : public Buffer
{
  public:
    VulkanBuffer(VulkanDevice& device, uint64_t size, uint32_t stride, BufferFlags flags);
    ~VulkanBuffer() override;

    VkBuffer GetBuffer() const
    {
        return buffer_;
    }
    uint32_t GetStride() const
    {
        return stride_;
    }
    BufferFlags GetFlags() const
    {
        return flags_;
    }

    void* Map() override;
    void Unmap() override;

  private:
    VulkanDevice& device_;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    BufferFlags flags_ = BufferFlags::kNone;
    uint32_t stride_ = 0;
    bool mapped_ = false;
};

} // namespace gpu
