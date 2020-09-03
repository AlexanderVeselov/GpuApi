#ifndef VULKAN_EXCEPTION_HPP_
#define VULKAN_EXCEPTION_HPP_

#include <vulkan/vulkan.h>
#include <stdexcept>

inline char* const GetVkErrorString(VkResult error_code)
{
    switch (error_code)
    {
    case 0: return "VK_SUCCESS";
    case 1: return "VK_NOT_READY";
    case 2: return "VK_TIMEOUT";
    case 3: return "VK_EVENT_SET";
    case 4: return "VK_EVENT_RESET";
    case 5: return "VK_INCOMPLETE";
    case -1: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case -2: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case -3: return "VK_ERROR_INITIALIZATION_FAILED";
    case -4: return "VK_ERROR_DEVICE_LOST";
    case -5: return "VK_ERROR_MEMORY_MAP_FAILED";
    case -6: return "VK_ERROR_LAYER_NOT_PRESENT";
    case -7: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case -8: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case -9: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case -10: return "VK_ERROR_TOO_MANY_OBJECTS";
    case -11: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case -12: return "VK_ERROR_FRAGMENTED_POOL";
    case -1000069000: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case -1000072003: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case -1000000000: return "VK_ERROR_SURFACE_LOST_KHR";
    case -1000000001: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case 1000001003: return "VK_SUBOPTIMAL_KHR";
    case -1000001004: return "VK_ERROR_OUT_OF_DATE_KHR";
    case -1000003001: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case -1000011001: return "VK_ERROR_VALIDATION_FAILED_EXT";
    case -1000012000: return "VK_ERROR_INVALID_SHADER_NV";
    case -1000161000: return "VK_ERROR_FRAGMENTATION_EXT";
    case -1000174001: return "VK_ERROR_NOT_PERMITTED_EXT";
    default: return "Unknown Vulkan Error";
    }
}

class VulkanException : public std::exception
{
public:
    VulkanException(std::string const& message, VkResult error_code)
        : std::exception((message + " (" + GetVkErrorString(error_code) + ")").c_str())
    {}
};

#define VK_THROW_IF_FAILED(err_code, msg) \
    if ((err_code) != VK_SUCCESS)         \
    {                                     \
        throw VulkanException(std::string(__FUNCTION__) + "(...): " + msg, err_code); \
    }

#define THROW_IF(condition, msg) \
    if (condition)               \
    {                            \
        throw std::runtime_error(std::string(__FUNCTION__) + "(...): " + msg); \
    }

#endif // VULKAN_EXCEPTION_HPP_
