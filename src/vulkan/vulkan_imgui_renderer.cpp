#include "vulkan_imgui_renderer.hpp"

#include "gpu_swapchain.hpp"
#include "vulkan_api.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"
#include "vulkan_image.hpp"
#include "vulkan_queue.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>

namespace gpu
{
namespace
{
void CheckVkResult(VkResult status)
{
    VK_THROW_IF_FAILED(status, "ImGui Vulkan backend error");
}
} // namespace

VulkanImGuiRenderer::VulkanImGuiRenderer(
    VulkanDevice& device, void* glfw_window, Swapchain& swapchain)
    : device_(device), swapchain_(swapchain)
{
    if (glfw_window == nullptr)
    {
        throw std::runtime_error("VulkanImGuiRenderer requires a GLFW window");
    }

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(glfw_window), true);

    VkFormat color_attachment_format = ToVkFormat(swapchain.GetFormat());

    VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{};
    pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipeline_rendering_create_info.colorAttachmentCount = 1;
    pipeline_rendering_create_info.pColorAttachmentFormats = &color_attachment_format;

    VulkanQueue& graphics_queue = static_cast<VulkanQueue&>(device_.GetQueue(QueueType::kGraphics));

    uint32_t image_count = std::max(2u, swapchain.GetImageCount());

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = device_.GetApi().GetInstance();
    init_info.PhysicalDevice = device_.GetPhysicalDevice();
    init_info.Device = device_.GetDevice();
    init_info.QueueFamily = device_.GetGraphicsQueueFamilyIndex();
    init_info.Queue = graphics_queue.GetQueue();
    init_info.DescriptorPoolSize = 64;
    init_info.MinImageCount = image_count;
    init_info.ImageCount = image_count;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeline_rendering_create_info;
    init_info.UseDynamicRendering = true;
    init_info.CheckVkResultFn = CheckVkResult;

    if (!ImGui_ImplVulkan_Init(&init_info))
    {
        throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
    }
}

VulkanImGuiRenderer::~VulkanImGuiRenderer()
{
    device_.GetQueue(QueueType::kGraphics).WaitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanImGuiRenderer::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void VulkanImGuiRenderer::Render(CommandBuffer& command_buffer)
{
    VulkanCommandBuffer* vulkan_command_buffer =
        dynamic_cast<VulkanCommandBuffer*>(&command_buffer);
    if (vulkan_command_buffer == nullptr)
    {
        throw std::runtime_error("VulkanImGuiRenderer requires a Vulkan command buffer");
    }

    ImGui::Render();
    command_buffer.SetRenderTarget(swapchain_.GetCurrentImage(), nullptr);
    ImGui_ImplVulkan_RenderDrawData(
        ImGui::GetDrawData(), vulkan_command_buffer->GetCommandBuffer());
}

} // namespace gpu
