#include "gpu_api.hpp"
#include "gpu_device.hpp"
#include "gpu_buffer.hpp"
#include "gpu_image.hpp"
#include "gpu_queue.hpp"
#include "gpu_command_buffer.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_pipeline.hpp"
#include "gpu_swapchain.hpp"

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#include "GLFW/glfw3native.h"
#undef CreateWindow

#include <iostream>
#include <cassert>
#include <thread>
#include <utility>

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

        gpu::GraphicsPipelinePtr pipeline;

        gpu::GraphicsPipelineDesc pipeline_desc;
        pipeline_desc.vs_filename = "shader.vs";
        pipeline_desc.ps_filename = "shader.ps";
        pipeline_desc.color_attachment_formats = { swapchain->GetFormat() };
        pipeline_desc.depth_enabled = false;
        pipeline = device->CreateGraphicsPipeline(pipeline_desc);

        struct Vertex
        {
            float position[3];
            float color[3];
        };

        Vertex vertices[3] = {
            { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
            {  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f },
            {  0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f },
        };

        gpu::BufferPtr vertex_buffer = device->CreateBuffer(sizeof(Vertex) * 3, sizeof(Vertex), gpu::BufferFlags::kCpuAccess);
        Vertex* data = (Vertex*)vertex_buffer->Map();
        memcpy(data, vertices, sizeof(vertices));
        vertex_buffer->Unmap();

        gpu::BufferPtr scale_buffer = device->CreateBuffer(sizeof(float), sizeof(float),
            gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kConstant);
        float* scale = (float*)scale_buffer->Map();
        *scale = 3.5f;
        scale_buffer->Unmap();

        gpu::DescriptorSetPtr descriptor_set = pipeline->CreateDescriptorSet();
        descriptor_set->BindBuffer(*scale_buffer, 0, 0);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            gpu::CommandBufferPtr cmd_buffer = graphics_queue.CreateCommandBuffer();
            gpu::ImagePtr swapchain_image = swapchain->GetCurrentImage();
            cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kPresent, gpu::ImageLayout::kRenderTarget);
            cmd_buffer->ClearImage(swapchain_image, 0.5f, 0.5f, 1.0f, 1.0f);
            cmd_buffer->BindPipeline(pipeline);
            cmd_buffer->BindDescriptorSet(descriptor_set);
            cmd_buffer->SetViewport(swapchain_image->GetWidth(), swapchain_image->GetHeight());
            cmd_buffer->SetScissorRect(swapchain_image->GetWidth(), swapchain_image->GetHeight());
            cmd_buffer->SetRenderTarget(swapchain_image, nullptr);
            cmd_buffer->SetVertexBuffer(vertex_buffer, sizeof(Vertex));
            cmd_buffer->Draw(3);
            cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kRenderTarget, gpu::ImageLayout::kPresent);

            cmd_buffer->End();

            graphics_queue.Submit(std::move(cmd_buffer));
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
