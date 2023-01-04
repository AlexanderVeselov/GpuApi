#include "gpu_api.hpp"
#include "gpu_device.hpp"
#include "gpu_queue.hpp"
#include "gpu_command_buffer.hpp"
#include "gpu_sync.hpp"
#include "gpu_swapchain.hpp"

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#include "GLFW/glfw3native.h"
#undef CreateWindow

#include <iostream>
#include <cassert>

GLFWwindow* CreateWindow(std::uint32_t width, std::uint32_t height)
{
    int init_status = glfwInit();

    if (init_status == GLFW_FALSE)
    {
        throw std::runtime_error("Failed to init GLFW!");
        return nullptr;
    }

    GLFWwindow* window = glfwCreateWindow((int)width, (int)height, "HelloTriangle", nullptr, nullptr);

    if (window == nullptr)
    {
        char const* description;
        glfwGetError(&description);

        throw std::runtime_error((std::string("Failed to create GLFW window (") + description + ")").c_str());
        return nullptr;
    }

    glfwShowWindow(window);

    return window;
}

int main()
{
    try
    {
        std::uint32_t window_width = 1280;
        std::uint32_t window_height = 720;

        GLFWwindow* window = CreateWindow(window_width, window_height);
        void* window_native_handle = glfwGetWin32Window(window);

        auto api = gpu::Api::Create(gpu::ApiType::kD3D12);
        assert(api);

        auto device = api->CreateDevice();

        std::uint32_t swapchain_image_count = 3u;
        gpu::SwapchainPtr swapchain = device->CreateSwapchain(window_native_handle,
            window_width, window_height, swapchain_image_count);

        auto& graphics_queue = device->GetQueue(gpu::QueueType::kGraphics);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            gpu::CommandBufferPtr cmd_buffer = graphics_queue.CreateCommandBuffer();
            gpu::ImagePtr swapchain_image = swapchain->GetCurrentImage();
            cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kPresent, gpu::ImageLayout::kRenderTarget);
            cmd_buffer->ClearImage(swapchain_image, 0.5f, 0.5f, 1.0f, 1.0f);
            cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kRenderTarget, gpu::ImageLayout::kPresent);
            cmd_buffer->End();

            graphics_queue.Submit(cmd_buffer);
            swapchain->Present();
            glfwSwapBuffers(window);
        }
    }
    catch (std::exception const& ex)
    {
        std::cerr << "Application error: " << ex.what() << std::endl;
        return -1;
    }

    return 0;
}