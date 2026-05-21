#include "vulkan_acceleration_structure.hpp"

#include "vulkan_device.hpp"

namespace gpu
{

VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDevice& device, AccelerationStructureType type,
    BufferPtr storage_buffer, VkAccelerationStructureKHR handle, uint64_t device_address)
    : AccelerationStructure(type, std::move(storage_buffer))
    , device_(device)
    , handle_(handle)
    , device_address_(device_address)
{
}

VulkanAccelerationStructure::~VulkanAccelerationStructure()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        vkDestroyAccelerationStructureKHR(device_.GetDevice(), handle_, nullptr);
    }
}

}  // namespace gpu
