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
        auto vk_destroy_acceleration_structure = reinterpret_cast<
            PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device_.GetDevice(),
            "vkDestroyAccelerationStructureKHR"));
        if (vk_destroy_acceleration_structure)
        {
            vk_destroy_acceleration_structure(device_.GetDevice(), handle_, nullptr);
        }
    }
}

}  // namespace gpu
