#include "vulkan_api.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_validation.hpp"

#include <cstring>
#include <vector>

namespace gpu
{
namespace
{
#ifndef NDEBUG
bool IsValidationLayerAvailable(char const* layer_name)
{
    uint32_t layer_count = 0;
    VkResult status = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan instance layers");

    std::vector<VkLayerProperties> available_layers(layer_count);
    status = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan instance layers");

    for (VkLayerProperties const& layer : available_layers)
    {
        if (std::strcmp(layer.layerName, layer_name) == 0)
        {
            return true;
        }
    }

    return false;
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;
}
#endif
}  // namespace

VulkanApi::VulkanApi() : shader_manager_("")
{
    CreateInstance();
#ifndef NDEBUG
    SetupDebugMessenger();
#endif
}

VulkanApi::~VulkanApi()
{
#ifndef NDEBUG
    if (debug_messenger_ != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
    }
#endif

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
    }
}

void VulkanApi::CreateInstance()
{
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "GpuApi";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "GpuApi";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    std::vector<char const*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

#ifndef NDEBUG
    for (char const* layer_name : g_validation_layer_names)
    {
        THROW_IF(!IsValidationLayerAvailable(layer_name),
            std::string("Vulkan validation layer is not available: ") + layer_name);
    }
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    PopulateDebugMessengerCreateInfo(debug_create_info);
    create_info.enabledLayerCount = static_cast<uint32_t>(g_validation_layer_names.size());
    create_info.ppEnabledLayerNames = g_validation_layer_names.data();
    create_info.pNext = &debug_create_info;
#endif

    VkResult status = vkCreateInstance(&create_info, nullptr, &instance_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan instance");
}

void VulkanApi::SetupDebugMessenger()
{
#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    PopulateDebugMessengerCreateInfo(create_info);

    VkResult status = CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan debug messenger");
#endif
}

VkPhysicalDevice VulkanApi::ChoosePhysicalDevice() const
{
    uint32_t physical_device_count = 0;
    VkResult status = vkEnumeratePhysicalDevices(instance_, &physical_device_count, nullptr);
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan physical devices");
    THROW_IF(physical_device_count == 0, "No Vulkan physical devices are available");

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    status = vkEnumeratePhysicalDevices(instance_, &physical_device_count, physical_devices.data());
    VK_THROW_IF_FAILED(status, "Failed to enumerate Vulkan physical devices");

    VkPhysicalDevice best_device = physical_devices[0];
    uint64_t best_score = 0;

    for (VkPhysicalDevice physical_device : physical_devices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical_device, &properties);

        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        uint64_t dedicated_memory = 0;
        for (uint32_t i = 0; i < memory_properties.memoryHeapCount; ++i)
        {
            if ((memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
            {
                dedicated_memory += memory_properties.memoryHeaps[i].size;
            }
        }

        uint64_t score = dedicated_memory;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1ull << 62;
        }

        if (score > best_score)
        {
            best_score = score;
            best_device = physical_device;
        }
    }

    return best_device;
}

DevicePtr VulkanApi::CreateDevice()
{
    return std::make_unique<VulkanDevice>(*this, ChoosePhysicalDevice());
}

void VulkanApi::SetShaderPath(char const* shader_path)
{
    shader_manager_.SetShaderPath(shader_path);
}

}  // namespace gpu
