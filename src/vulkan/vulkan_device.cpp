#include "vulkan_device.hpp"

#include "vulkan_acceleration_structure.hpp"
#include "vulkan_api.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_imgui_renderer.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_queue.hpp"
#include "vulkan_sampler.hpp"
#include "vulkan_swapchain.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace gpu
{
namespace
{
VkAccelerationStructureGeometryKHR ToVkGeometryDesc(AccelerationStructureGeometryDesc const& geometry)
{
    if (!geometry.vertex_buffer || !geometry.index_buffer)
    {
        throw std::runtime_error("VulkanDevice: acceleration-structure geometry buffers must not be null");
    }
    if (geometry.vertex_count == 0 || geometry.index_count == 0 || geometry.index_count % 3 != 0)
    {
        throw std::runtime_error("VulkanDevice: acceleration-structure geometry must contain indexed triangles");
    }

    VkAccelerationStructureGeometryKHR desc{};
    desc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    desc.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    desc.flags = geometry.opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;
    desc.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    desc.geometry.triangles.vertexFormat = ToVkFormat(geometry.vertex_format);
    desc.geometry.triangles.vertexData.deviceAddress = geometry.vertex_buffer->GetGpuAddress() + geometry.vertex_offset;
    desc.geometry.triangles.vertexStride = geometry.vertex_stride;
    desc.geometry.triangles.maxVertex = geometry.vertex_count - 1;
    desc.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    desc.geometry.triangles.indexData.deviceAddress = geometry.index_buffer->GetGpuAddress() + geometry.index_offset;
    return desc;
}

VkAccelerationStructureGeometryKHR CreateTopLevelGeometryDesc()
{
    VkAccelerationStructureGeometryKHR desc{};
    desc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    desc.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    desc.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    desc.geometry.instances.arrayOfPointers = VK_FALSE;
    return desc;
}
}  // namespace

VulkanDevice::VulkanDevice(VulkanApi& api, VkPhysicalDevice physical_device)
    : api_(api), physical_device_(physical_device), memory_manager_(*this)
{
    FindQueueFamilyIndices();
    CreateLogicalDevice();

    graphics_queue_ = std::make_unique<VulkanQueue>(*this, graphics_queue_family_index_);
    compute_queue_ = std::make_unique<VulkanQueue>(*this, compute_queue_family_index_);
    transfer_queue_ = std::make_unique<VulkanQueue>(*this, transfer_queue_family_index_);
}

VulkanDevice::~VulkanDevice()
{
    WaitIdle();
    ClearSamplerCache();
    graphics_queue_.reset();
    compute_queue_.reset();
    transfer_queue_.reset();

    if (device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device_, nullptr);
    }
}

void VulkanDevice::FindQueueFamilyIndices()
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        VkQueueFlags flags = queue_families[i].queueFlags;

        if (graphics_queue_family_index_ == UINT32_MAX && (flags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            graphics_queue_family_index_ = i;
        }

        if (compute_queue_family_index_ == UINT32_MAX && (flags & VK_QUEUE_COMPUTE_BIT) != 0)
        {
            compute_queue_family_index_ = i;
        }

        if (transfer_queue_family_index_ == UINT32_MAX && (flags & VK_QUEUE_TRANSFER_BIT) != 0)
        {
            transfer_queue_family_index_ = i;
        }
    }

    THROW_IF(graphics_queue_family_index_ == UINT32_MAX, "No Vulkan graphics queue family");
    THROW_IF(compute_queue_family_index_ == UINT32_MAX, "No Vulkan compute queue family");
    THROW_IF(transfer_queue_family_index_ == UINT32_MAX, "No Vulkan transfer queue family");
}

bool VulkanDevice::CheckRayQuerySupport() const
{
    uint32_t extension_count = 0;
    VkResult status = vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan device extensions");

    std::vector<VkExtensionProperties> extensions(extension_count);
    status = vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, extensions.data());
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan device extensions");

    auto has_extension = [&](char const* extension_name)
    {
        return std::find_if(extensions.begin(), extensions.end(), [&](VkExtensionProperties const& extension)
                   { return std::strcmp(extension.extensionName, extension_name) == 0; })
            != extensions.end();
    };

    if (!has_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) || !has_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME)
        || !has_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
        || !has_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        return false;
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
    acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features{};
    ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    ray_query_features.pNext = &acceleration_structure_features;

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
    buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.pNext = &ray_query_features;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &buffer_device_address_features;
    vkGetPhysicalDeviceFeatures2(physical_device_, &features2);

    return acceleration_structure_features.accelerationStructure == VK_TRUE && ray_query_features.rayQuery == VK_TRUE
        && buffer_device_address_features.bufferDeviceAddress == VK_TRUE;
}

void VulkanDevice::CreateLogicalDevice()
{
    std::array<uint32_t, 3> raw_indices = {
        graphics_queue_family_index_,
        compute_queue_family_index_,
        transfer_queue_family_index_,
    };

    std::vector<uint32_t> queue_family_indices;
    for (uint32_t index : raw_indices)
    {
        if (std::find(queue_family_indices.begin(), queue_family_indices.end(), index) == queue_family_indices.end())
        {
            queue_family_indices.push_back(index);
        }
    }

    float queue_priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(queue_family_indices.size());

    for (uint32_t queue_family_index : queue_family_indices)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    ray_query_supported_ = CheckRayQuerySupport();

    std::vector<char const*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    if (ray_query_supported_)
    {
        extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
    dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_features.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
    buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.pNext = &dynamic_rendering_features;
    buffer_device_address_features.bufferDeviceAddress = ray_query_supported_ ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
    acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_structure_features.pNext = &buffer_device_address_features;
    acceleration_structure_features.accelerationStructure = ray_query_supported_ ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features{};
    ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    ray_query_features.pNext = &acceleration_structure_features;
    ray_query_features.rayQuery = ray_query_supported_ ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing_features.pNext = ray_query_supported_ ? static_cast<void*>(&ray_query_features)
                                                              : static_cast<void*>(&dynamic_rendering_features);
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &descriptor_indexing_features;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    device_create_info.ppEnabledExtensionNames = extensions.data();

    VkResult status = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan device");
}

BufferPtr VulkanDevice::CreateBuffer(std::size_t size, std::uint32_t stride, BufferFlags flags)
{
    return std::make_shared<VulkanBuffer>(*this, size, stride, flags);
}

AccelerationStructurePtr VulkanDevice::CreateAccelerationStructure(AccelerationStructureType type, uint64_t size,
    uint64_t build_scratch_size)
{
    if (!SupportsRayQuery())
    {
        throw std::runtime_error("VulkanDevice::CreateAccelerationStructure: ray query is not supported");
    }
    if (size == 0)
    {
        throw std::runtime_error("VulkanDevice::CreateAccelerationStructure: size must be greater than zero");
    }

    BufferPtr storage_buffer = CreateBuffer(size, 1, BufferFlags::kAccelerationStructureStorage);
    auto* vulkan_buffer = dynamic_cast<VulkanBuffer*>(storage_buffer.get());
    THROW_IF(!vulkan_buffer, "VulkanDevice::CreateAccelerationStructure: storage buffer is not a Vulkan buffer");

    VkAccelerationStructureCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    create_info.buffer = vulkan_buffer->GetBuffer();
    create_info.offset = 0;
    create_info.size = size;
    create_info.type = type == AccelerationStructureType::kBottomLevel ? VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
                                                                       : VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    auto vk_create_acceleration_structure = reinterpret_cast<
        PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device_, "vkCreateAccelerationStructureKHR"));
    THROW_IF(!vk_create_acceleration_structure,
        "VulkanDevice::CreateAccelerationStructure: vkCreateAccelerationStructureKHR is unavailable");

    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    VkResult status = vk_create_acceleration_structure(device_, &create_info, nullptr, &handle);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan acceleration structure");

    VkAccelerationStructureDeviceAddressInfoKHR address_info{};
    address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    address_info.accelerationStructure = handle;

    auto vk_get_acceleration_structure_device_address = reinterpret_cast<
        PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device_,
        "vkGetAccelerationStructureDeviceAddressKHR"));
    THROW_IF(!vk_get_acceleration_structure_device_address,
        "VulkanDevice::CreateAccelerationStructure: vkGetAccelerationStructureDeviceAddressKHR is unavailable");

    uint64_t device_address = vk_get_acceleration_structure_device_address(device_, &address_info);
    return std::make_shared<VulkanAccelerationStructure>(*this, type, std::move(storage_buffer), build_scratch_size,
        handle, device_address);
}

AccelerationStructurePtr
VulkanDevice::CreateBottomLevelAccelerationStructure(std::vector<AccelerationStructureGeometryDesc> const& geometries)
{
    if (geometries.empty())
    {
        throw std::runtime_error("VulkanDevice::CreateBottomLevelAccelerationStructure: geometry list is empty");
    }

    std::vector<VkAccelerationStructureGeometryKHR> vk_geometries;
    std::vector<uint32_t> primitive_counts;
    vk_geometries.reserve(geometries.size());
    primitive_counts.reserve(geometries.size());
    for (AccelerationStructureGeometryDesc const& geometry : geometries)
    {
        vk_geometries.push_back(ToVkGeometryDesc(geometry));
        primitive_counts.push_back(geometry.index_count / 3);
    }

    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.geometryCount = static_cast<uint32_t>(vk_geometries.size());
    build_info.pGeometries = vk_geometries.data();

    VkAccelerationStructureBuildSizesInfoKHR size_info{};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    auto vk_get_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device_,
        "vkGetAccelerationStructureBuildSizesKHR"));
    THROW_IF(!vk_get_build_sizes,
        "VulkanDevice::CreateBottomLevelAccelerationStructure: vkGetAccelerationStructureBuildSizesKHR is unavailable");

    vk_get_build_sizes(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, primitive_counts.data(),
        &size_info);

    return CreateAccelerationStructure(AccelerationStructureType::kBottomLevel, size_info.accelerationStructureSize,
        size_info.buildScratchSize);
}

AccelerationStructurePtr VulkanDevice::CreateTopLevelAccelerationStructure(uint32_t instance_count)
{
    if (instance_count == 0)
    {
        throw std::runtime_error(
            "VulkanDevice::CreateTopLevelAccelerationStructure: instance count must be greater than zero");
    }

    VkAccelerationStructureGeometryKHR geometry = CreateTopLevelGeometryDesc();
    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR size_info{};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    auto vk_get_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device_,
        "vkGetAccelerationStructureBuildSizesKHR"));
    THROW_IF(!vk_get_build_sizes,
        "VulkanDevice::CreateTopLevelAccelerationStructure: vkGetAccelerationStructureBuildSizesKHR is unavailable");

    vk_get_build_sizes(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &instance_count,
        &size_info);

    return CreateAccelerationStructure(AccelerationStructureType::kTopLevel, size_info.accelerationStructureSize,
        size_info.buildScratchSize);
}

ImagePtr VulkanDevice::CreateImage(uint32_t width, uint32_t height, ImageFormat format, ImageFlags flags,
    uint32_t mip_count, uint32_t array_size)
{
    return std::make_shared<VulkanImage>(*this, width, height, format, mip_count, array_size, flags);
}

SamplerPtr VulkanDevice::CreateSampler(SamplerDesc const& desc)
{
    return std::make_shared<VulkanSampler>(*this, desc);
}

Queue& VulkanDevice::GetQueue(QueueType queue_type)
{
    switch (queue_type)
    {
    case QueueType::kGraphics:
        return *graphics_queue_;
    case QueueType::kCompute:
        return *compute_queue_;
    case QueueType::kTransfer:
        return *transfer_queue_;
    default:
        throw std::runtime_error("Unsupported Vulkan queue type");
    }
}

GraphicsPipelinePtr VulkanDevice::CreateGraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc)
{
    auto pipeline = std::make_unique<VulkanGraphicsPipeline>(*this, pipeline_desc);
    RegisterPipeline(pipeline.get());
    return pipeline;
}

ComputePipelinePtr VulkanDevice::CreateComputePipeline(char const* cs_filename, char const* root_constants_name)
{
    auto pipeline = std::make_unique<VulkanComputePipeline>(*this, cs_filename, root_constants_name);
    RegisterPipeline(pipeline.get());
    return pipeline;
}

void VulkanDevice::WaitIdle()
{
    if (graphics_queue_)
    {
        graphics_queue_->WaitIdle();
    }

    if (compute_queue_)
    {
        compute_queue_->WaitIdle();
    }

    if (transfer_queue_)
    {
        transfer_queue_->WaitIdle();
    }
}

SwapchainPtr VulkanDevice::CreateSwapchain(void* window_native_handle, std::uint32_t width, std::uint32_t height,
    std::uint32_t image_count)
{
    return std::make_unique<VulkanSwapchain>(*this, window_native_handle, width, height, image_count);
}

ImGuiRendererPtr VulkanDevice::CreateImGuiRenderer(void* glfw_window, Swapchain& swapchain)
{
    return std::make_unique<VulkanImGuiRenderer>(*this, glfw_window, swapchain);
}

}  // namespace gpu
