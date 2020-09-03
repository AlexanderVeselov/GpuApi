#ifndef VULKAN_SHARED_OBJECT_HPP_
#define VULKAN_SHARED_OBJECT_HPP_

#include <memory>
#include <vulkan/vulkan.h>

template <class T>
using VulkanSharedObject = std::shared_ptr<std::remove_pointer_t<T>>;

template <class VkObject, auto create_func, auto destroy_func>
class VulkanScopedObject
{
public:
    VulkanScopedObject()
        : object_(VK_NULL_HANDLE)
        , device_(VK_NULL_HANDLE)
    {}

    template <class CreateInfo>
    VkResult Create(VkDevice device, CreateInfo const& create_info)
    {
        if (object_ != VK_NULL_HANDLE)
        {
            destroy_func(device_, object_, nullptr);
        }

        device_ = device;

        return create_func(device, &create_info, nullptr, &object_);
    }

    void Attach(VkDevice device, VkObject object)
    {
        object_ = object;
        device_ = device;
    }

    ~VulkanScopedObject()
    {
        if (object_ != VK_NULL_HANDLE)
        {
            destroy_func(device_, object_, nullptr);
        }
    }

    operator VkObject() const
    {
        return object_;
    }

    VkObject* operator& ()
    {
        return &object_;
    }

private:
    VkObject object_;
    VkDevice device_;

};

template <auto create_func, auto destroy_func>
class VulkanScopedObject<VkPipeline, create_func, destroy_func>
{
public:
    VulkanScopedObject()
        : object_(VK_NULL_HANDLE)
        , device_(VK_NULL_HANDLE)
    {}

    template <class CreateInfo>
    VkResult Reset(VkDevice device, CreateInfo const& create_info)
    {
        if (object_ != VK_NULL_HANDLE)
        {
            destroy_func(device_, object_, nullptr);
        }

        device_ = device;

        return create_func(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &object_);
    }

    ~VulkanScopedObject()
    {
        if (object_ != VK_NULL_HANDLE)
        {
            destroy_func(device_, object_, nullptr);
        }
    }

    operator VkPipeline() const
    {
        return object_;
    }

private:
    VkPipeline object_;
    VkDevice device_;

};

#endif // VULKAN_SHARED_OBJECT_HPP_