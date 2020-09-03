#include "vulkan_api.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_device.hpp"
#include "vulkan_validation.hpp"

namespace gpu
{

static void CheckVulkanLayersSupport(std::vector<char const*> const& required_layer_names)
{
    // Get available layer count
    std::uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    // Retrieve available layers
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (auto const& layer_name : required_layer_names)
    {
        bool layer_found = false;
        for (auto const& available_layer : available_layers)
        {
            if (strcmp(layer_name, available_layer.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            throw std::runtime_error("CheckVulkanLayersSupport(...): "
                "required layer '" + std::string(layer_name) + "'is not available");
        }
    }

}

VulkanApi::VulkanApi(std::vector<char const*> const& enabled_extensions, bool enable_validation)
    : validation_enabled_(enable_validation)
{
    // Create basic vulkan instance
    CreateInstance(enabled_extensions);

    if (validation_enabled_)
    {
        // Create debug messenger that prints information from validation layers
        CreateDebugMessenger();
    }

}

VulkanApi::~VulkanApi()
{
    if (debug_messenger_)
    {
        DestroyDebugUtilsMessengerEXT(instance_.get(), debug_messenger_, nullptr);
    }
}

void VulkanApi::CreateInstance(std::vector<char const*> enabled_extensions)
{
    // Setup VkApplicationInfo
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "CGLabs";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    // Create Vulkan instance

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    if (validation_enabled_)
    {
        CheckVulkanLayersSupport(g_validation_layer_names);

        create_info.ppEnabledLayerNames = g_validation_layer_names.data();
        create_info.enabledLayerCount = static_cast<std::uint32_t>(g_validation_layer_names.size());

        // Add extension that required by validation layers message callback
        enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    create_info.ppEnabledExtensionNames = enabled_extensions.data();
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size());

    VkInstance instance;
    VkResult error_code = vkCreateInstance(&create_info, nullptr, &instance);
    VK_THROW_IF_FAILED(error_code, "Failed to create VkInstance!");

    instance_.reset(instance, [](VkInstance instance)
    {
        vkDestroyInstance(instance, nullptr);
    });
}

void VulkanApi::CreateDebugMessenger()
{
    // Create debug utils messenger
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {};
    messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    messenger_create_info.pfnUserCallback = DebugCallback;
    messenger_create_info.pUserData = nullptr;

    VkResult error_code = CreateDebugUtilsMessengerEXT(instance_.get(), &messenger_create_info, nullptr, &debug_messenger_);
    VK_THROW_IF_FAILED(error_code, "Failed to create debug messenger!");

}

std::shared_ptr<VulkanDevice> VulkanApi::CreateDevice(std::vector<char const*> const& enabled_extensions, std::uint32_t physical_device_index, std::function<VkSurfaceKHR(VkInstance)> surface_creation_callback)
{
    // Get physical device count
    std::uint32_t physical_device_count = 0;
    VkResult error_code = vkEnumeratePhysicalDevices(instance_.get(), &physical_device_count, nullptr);
    VK_THROW_IF_FAILED(error_code, "Failed to enumerate physical devices!");

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance_.get(), &physical_device_count, physical_devices.data());

    VulkanSharedObject<VkSurfaceKHR> surface;

    VkInstance instance = instance_.get();
    surface.reset(surface_creation_callback(instance), [instance](VkSurfaceKHR surface)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    });

    return std::make_shared<VulkanDevice>(*this, physical_devices[physical_device_index], g_validation_layer_names, enabled_extensions, surface);

}

VulkanSharedObject<VkInstance> VulkanApi::GetInstance() const
{
    return instance_;
}

} // namespace gpu
