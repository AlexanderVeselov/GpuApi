#pragma once

#include "gpu_acceleration_structure.hpp"

#include <vulkan/vulkan.h>

namespace gpu
{

class VulkanDevice;

class VulkanAccelerationStructure final : public AccelerationStructure
{
public:
    VulkanAccelerationStructure(VulkanDevice& device, AccelerationStructureType type, BufferPtr storage_buffer,
        uint64_t build_scratch_size, VkAccelerationStructureKHR handle, uint64_t device_address);
    ~VulkanAccelerationStructure() override;

    VkAccelerationStructureKHR GetHandle() const { return handle_; }
    uint64_t GetDeviceAddress() const { return device_address_; }

private:
    VulkanDevice& device_;
    VkAccelerationStructureKHR handle_ = VK_NULL_HANDLE;
    uint64_t device_address_ = 0;
};

}  // namespace gpu
