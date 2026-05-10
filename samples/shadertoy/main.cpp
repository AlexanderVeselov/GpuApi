#include "gpu_api.hpp"
#include "gpu_command_buffer.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_device.hpp"
#include "gpu_image.hpp"
#include "gpu_pipeline.hpp"
#include "gpu_queue.hpp"
#include "gpu_swapchain.hpp"

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#include "GLFW/glfw3native.h"
#undef CreateWindow

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
GLFWwindow* CreateWindow(std::uint32_t width, std::uint32_t height)
{
    if (glfwInit() == GLFW_FALSE)
    {
        throw std::runtime_error("Failed to init GLFW");
    }

    GLFWwindow* window = glfwCreateWindow((int)width, (int)height, "Shadertoy",
        nullptr, nullptr);
    if (!window)
    {
        char const* description = nullptr;
        glfwGetError(&description);
        throw std::runtime_error(std::string("Failed to create GLFW window: ") +
            (description ? description : "unknown error"));
    }

    glfwShowWindow(window);
    return window;
}

std::uint32_t DivideAndRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1) / divisor;
}
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

        gpu::DevicePtr device = api->CreateDevice();
        gpu::SwapchainPtr swapchain = device->CreateSwapchain(window_native_handle,
            window_width, window_height, 3);
        gpu::ImagePtr output_image = device->CreateImage(window_width, window_height,
            swapchain->GetFormat(),
            1,
            1,
            gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);

        gpu::ComputePipelinePtr pipeline = device->CreateComputePipeline("shader.cs");
        gpu::DescriptorSetPtr descriptor_set = pipeline->CreateDescriptorSet();
        descriptor_set->BindImage(*output_image, 0, 0);

        gpu::Queue& queue = device->GetQueue(gpu::QueueType::kGraphics);
        gpu::ImageLayout output_layout = gpu::ImageLayout::kUndefined;

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            gpu::ImagePtr swapchain_image = swapchain->GetCurrentImage();
            gpu::CommandBufferPtr cmd_buffer = queue.CreateCommandBuffer();
            cmd_buffer->TransitionBarrier(output_image, output_layout,
                gpu::ImageLayout::kShaderReadWrite);
            cmd_buffer->BindPipeline(pipeline);
            cmd_buffer->BindDescriptorSet(descriptor_set);
            cmd_buffer->Dispatch(DivideAndRoundUp(swapchain_image->GetWidth(), 8),
                DivideAndRoundUp(swapchain_image->GetHeight(), 8), 1);
            cmd_buffer->TransitionBarrier(output_image, gpu::ImageLayout::kShaderReadWrite,
                gpu::ImageLayout::kCopySrc);
            output_layout = gpu::ImageLayout::kCopySrc;
            cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kPresent,
                gpu::ImageLayout::kCopyDst);
            cmd_buffer->CopyImage(swapchain_image.get(), output_image.get());
            cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kCopyDst,
                gpu::ImageLayout::kPresent);
            queue.Submit(std::move(cmd_buffer));
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
