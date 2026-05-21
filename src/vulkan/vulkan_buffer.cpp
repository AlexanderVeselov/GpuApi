#include "vulkan_buffer.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

namespace gpu
{
static bool RequiresDeviceAddress(BufferFlags flags)
{
    return HasFlag(flags, BufferFlags::kAccelerationStructureBuildInput)
        || HasFlag(flags, BufferFlags::kAccelerationStructureStorage);
}

static VkBufferUsageFlags ToVkBufferUsage(BufferFlags flags)
{
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (HasFlag(flags, BufferFlags::kConstant))
    {
        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    if (HasFlag(flags, BufferFlags::kShaderResource))
    {
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (HasFlag(flags, BufferFlags::kStorage))
    {
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (HasFlag(flags, BufferFlags::kAccelerationStructureBuildInput))
    {
        usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }

    if (HasFlag(flags, BufferFlags::kAccelerationStructureStorage))
    {
        usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (RequiresDeviceAddress(flags))
    {
        usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    return usage;
}

VulkanBuffer::VulkanBuffer(VulkanDevice& device, uint64_t size, uint32_t stride, BufferFlags flags)
    : Buffer(size), device_(device), flags_(flags), stride_(stride)
{
    THROW_IF(RequiresDeviceAddress(flags) && !device_.SupportsRayQuery(),
        "Vulkan acceleration-structure buffers require ray query / buffer device address support");

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = ToVkBufferUsage(flags);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice logical_device = device_.GetDevice();
    VkResult status = vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &buffer_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan buffer");

    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(logical_device, buffer_, &memory_requirements);

    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (HasFlag(flags, BufferFlags::kCpuAccess))
    {
        memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    VkMemoryAllocateFlags allocate_flags = 0;
    if (RequiresDeviceAddress(flags))
    {
        allocate_flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    }

    memory_ = device_.GetMemoryManager().AllocateMemory(memory_requirements, memory_flags, allocate_flags);

    status = vkBindBufferMemory(logical_device, buffer_, memory_, 0);
    VK_THROW_IF_FAILED(status, "Failed to bind Vulkan buffer memory");
}

VulkanBuffer::~VulkanBuffer()
{
    VkDevice logical_device = device_.GetDevice();

    if (memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(logical_device, memory_, nullptr);
    }

    if (buffer_ != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(logical_device, buffer_, nullptr);
    }
}

void* VulkanBuffer::Map()
{
    THROW_IF(!HasFlag(flags_, BufferFlags::kCpuAccess), "Vulkan buffer is not CPU-accessible");
    THROW_IF(mapped_, "Vulkan buffer is already mapped");

    void* data = nullptr;
    VkResult status = vkMapMemory(device_.GetDevice(), memory_, 0, GetSize(), 0, &data);
    VK_THROW_IF_FAILED(status, "Failed to map Vulkan buffer memory");

    mapped_ = true;
    return data;
}

void VulkanBuffer::Unmap()
{
    THROW_IF(!mapped_, "Vulkan buffer is not mapped");

    vkUnmapMemory(device_.GetDevice(), memory_);
    mapped_ = false;
}

uint64_t VulkanBuffer::GetGpuAddress() const
{
    THROW_IF(!HasFlag(flags_, BufferFlags::kAccelerationStructureBuildInput)
            && !HasFlag(flags_, BufferFlags::kAccelerationStructureStorage),
        "VulkanBuffer::GetGpuAddress: buffer was not created with acceleration-structure support");

    VkBufferDeviceAddressInfo address_info{};
    address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    address_info.buffer = buffer_;
    return vkGetBufferDeviceAddress(device_.GetDevice(), &address_info);
}

}  // namespace gpu
