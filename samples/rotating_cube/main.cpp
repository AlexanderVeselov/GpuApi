#include "gpu_api.hpp"
#include "gpu_buffer.hpp"
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
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
struct Mat4
{
    float m[4][4] = {};
};

// Row-vector matrix helpers. Shaders are compiled with DXC -Zpr, so this layout can be
// copied directly into a constant buffer and used as mul(float4(position, 1), matrix).
Mat4 Mul(Mat4 const& a, Mat4 const& b)
{
    Mat4 result;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            for (int k = 0; k < 4; ++k)
            {
                result.m[row][col] += a.m[row][k] * b.m[k][col];
            }
        }
    }
    return result;
}

Mat4 RotationY(float angle)
{
    Mat4 result = {};
    float c = std::cos(angle);
    float s = std::sin(angle);
    result.m[0][0] = c;
    result.m[0][2] = -s;
    result.m[1][1] = 1.0f;
    result.m[2][0] = s;
    result.m[2][2] = c;
    result.m[3][3] = 1.0f;
    return result;
}

Mat4 RotationX(float angle)
{
    Mat4 result = {};
    float c = std::cos(angle);
    float s = std::sin(angle);
    result.m[0][0] = 1.0f;
    result.m[1][1] = c;
    result.m[1][2] = s;
    result.m[2][1] = -s;
    result.m[2][2] = c;
    result.m[3][3] = 1.0f;
    return result;
}

Mat4 Translation(float x, float y, float z)
{
    Mat4 result = {};
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][0] = x;
    result.m[3][1] = y;
    result.m[3][2] = z;
    result.m[3][3] = 1.0f;
    return result;
}

Mat4 Perspective(float fov_y, float aspect, float near_z, float far_z)
{
    Mat4 result = {};
    float y_scale = 1.0f / std::tan(fov_y * 0.5f);
    float x_scale = y_scale / aspect;
    result.m[0][0] = x_scale;
    result.m[1][1] = y_scale;
    result.m[2][2] = far_z / (far_z - near_z);
    result.m[2][3] = 1.0f;
    result.m[3][2] = -near_z * far_z / (far_z - near_z);
    return result;
}

GLFWwindow* CreateWindow(uint32_t width, uint32_t height)
{
    if (glfwInit() == GLFW_FALSE)
    {
        throw std::runtime_error("Failed to init GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window =
        glfwCreateWindow((int)width, (int)height, "RotatingCube", nullptr, nullptr);
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
} // namespace

int main()
{
    try
    {
        uint32_t window_width = 1280;
        uint32_t window_height = 720;

        GLFWwindow* window = CreateWindow(window_width, window_height);
        void* window_native_handle = glfwGetWin32Window(window);

        // Switch this to gpu::ApiType::kVulkan to run the same sample through the Vulkan backend.
        auto api = gpu::Api::Create(gpu::ApiType::kD3D12);
        assert(api);

        gpu::DevicePtr device = api->CreateDevice();
        gpu::SwapchainPtr swapchain =
            device->CreateSwapchain(window_native_handle, window_width, window_height, 3);
        // The depth buffer is a regular API image. The sample clears it explicitly every frame.
        gpu::ImagePtr depth_image = device->CreateImage(window_width, window_height,
            gpu::ImageFormat::kD32_Float, gpu::ImageFlags::kDepthStencil);

        // The pipeline description mirrors the render target formats used later by SetRenderTarget.
        gpu::GraphicsPipelineDesc pipeline_desc;
        pipeline_desc.vs_filename = "shader.vs";
        pipeline_desc.ps_filename = "shader.ps";
        pipeline_desc.color_attachment_formats = {swapchain->GetFormat()};
        pipeline_desc.depth_enabled = true;
        pipeline_desc.depth_func = gpu::DepthFunc::kLess;
        pipeline_desc.depth_attachment_format = gpu::ImageFormat::kD32_Float;
        gpu::GraphicsPipelinePtr pipeline = device->CreateGraphicsPipeline(pipeline_desc);

        struct Vertex
        {
            float position[3];
            float color[3];
        };

        Vertex vertices[] = {
            {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
            {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
            {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
            {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            {{1.0f, -1.0f, 1.0f}, {0.2f, 0.4f, 1.0f}},
        };

        uint32_t indices[] = {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            0, 4, 5, 0, 5, 1,
            1, 5, 6, 1, 6, 2,
            3, 2, 6, 3, 6, 7,
            4, 0, 3, 4, 3, 7,
        };

        // Upload buffers are enough for this sample. A real renderer would stage into GPU-local
        // memory.
        gpu::BufferPtr vertex_buffer =
            device->CreateBuffer(sizeof(vertices), sizeof(Vertex), gpu::BufferFlags::kCpuAccess);
        void* vertex_data = vertex_buffer->Map();
        std::memcpy(vertex_data, vertices, sizeof(vertices));
        vertex_buffer->Unmap();

        gpu::BufferPtr index_buffer =
            device->CreateBuffer(sizeof(indices), sizeof(uint32_t), gpu::BufferFlags::kCpuAccess);
        void* index_data = index_buffer->Map();
        std::memcpy(index_data, indices, sizeof(indices));
        index_buffer->Unmap();

        gpu::BufferPtr constants = device->CreateBuffer(
            sizeof(Mat4), sizeof(Mat4), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kConstant);
        gpu::DescriptorSetPtr descriptor_set = pipeline->CreateDescriptorSet();
        descriptor_set->BindBuffer(*constants, 0, 0);

        gpu::Queue& queue = device->GetQueue(gpu::QueueType::kGraphics);
        // The swapchain image is acquired in present layout each frame. The depth image persists,
        // so the sample tracks its current layout across frames.
        gpu::ImageLayout depth_layout = gpu::ImageLayout::kUndefined;
        auto start_time = std::chrono::steady_clock::now();

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            auto now = std::chrono::steady_clock::now();
            float t = std::chrono::duration<float>(now - start_time).count();
            // Compose a simple model-view-projection transform for a cube centered at the origin.
            Mat4 world = Mul(RotationX(t * 0.55f), RotationY(t));
            Mat4 view = Translation(0.0f, 0.0f, 4.5f);
            Mat4 projection = Perspective(60.0f * 3.1415926535f / 180.0f,
                (float)window_width / (float)window_height, 0.1f, 100.0f);
            Mat4 mvp = Mul(Mul(world, view), projection);

            void* constants_data = constants->Map();
            std::memcpy(constants_data, &mvp, sizeof(mvp));
            constants->Unmap();

            gpu::ImagePtr swapchain_image = swapchain->GetCurrentImage();
            gpu::CommandBufferPtr cmd_buffer = queue.CreateCommandBuffer();

            // Clear color through transfer layout, then render through render-target layout.
            cmd_buffer->TransitionBarrier(
                swapchain_image, gpu::ImageLayout::kPresent, gpu::ImageLayout::kCopyDst);
            cmd_buffer->ClearImage(swapchain_image, 0.04f, 0.05f, 0.08f, 1.0f);
            cmd_buffer->TransitionBarrier(
                swapchain_image, gpu::ImageLayout::kCopyDst, gpu::ImageLayout::kRenderTarget);

            // Depth clear also uses transfer layout before binding the image as a depth target.
            cmd_buffer->TransitionBarrier(depth_image, depth_layout, gpu::ImageLayout::kCopyDst);
            cmd_buffer->ClearDepthImage(depth_image, 1.0f);
            cmd_buffer->TransitionBarrier(
                depth_image, gpu::ImageLayout::kCopyDst, gpu::ImageLayout::kRenderTarget);
            depth_layout = gpu::ImageLayout::kRenderTarget;

            // Bind state and issue the indexed cube draw.
            cmd_buffer->BindPipeline(pipeline);
            cmd_buffer->BindDescriptorSet(descriptor_set);
            cmd_buffer->SetViewport(
                gpu::Viewport{0.0f, 0.0f, (float)window_width, (float)window_height, 0.0f, 1.0f});
            cmd_buffer->SetScissor(gpu::Rect{0, 0, (int32_t)window_width, (int32_t)window_height});
            cmd_buffer->SetRenderTarget(swapchain_image, depth_image);
            cmd_buffer->SetVertexBuffer(vertex_buffer, sizeof(Vertex));
            cmd_buffer->SetIndexBuffer(index_buffer);
            cmd_buffer->DrawIndexed(static_cast<uint32_t>(std::size(indices)));
            cmd_buffer->TransitionBarrier(
                swapchain_image, gpu::ImageLayout::kRenderTarget, gpu::ImageLayout::kPresent);

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
