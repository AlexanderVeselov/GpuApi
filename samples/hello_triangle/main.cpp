#include "gpu_api.hpp"
#include "gpu_device.hpp"
#include "gpu_queue.hpp"
#include "gpu_command_buffer.hpp"
#include "gpu_sync.hpp"
#include "gpu_swapchain.hpp"

#include "GLFW/glfw3.h"

#include <iostream>
#include <cassert>

int main()
{
    int window_width = 1280;
    int window_height = 720;

    int init_status = glfwInit();

    if (init_status == GLFW_FALSE)
    {
        std::cerr << "Failed to init GLFW!\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "HelloTriangle", nullptr, nullptr);
    glfwShowWindow(window);

    /*
    auto api = gpu::Api::Create(gpu::ApiType::kD3D12);
    assert(api);

    auto device = api->CreateDevice();

    gpu::SwapchainPtr swapchain = device->CreateSwapchain();
    auto& graphics_queue = device->GetQueue(gpu::QueueType::kGraphics);
    auto cmd_buffer = graphics_queue.CreateCommandBuffer();

    auto fence = device->CreateFence();
    */

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

//    graphics_queue.Submit(cmd_buffer, fence);

    return 0;
}