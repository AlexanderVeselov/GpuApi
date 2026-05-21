#include "d3d12_imgui_renderer.hpp"

#include "d3d12_command_buffer.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_queue.hpp"
#include "gpu_swapchain.hpp"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace gpu
{
namespace
{
constexpr uint32_t kSrvDescriptorCount = 64;
}

D3D12ImGuiRenderer::D3D12ImGuiRenderer(D3D12Device& device, void* glfw_window, Swapchain& swapchain)
    : device_(device), swapchain_(swapchain)
{
    if (glfw_window == nullptr)
    {
        throw std::runtime_error("D3D12ImGuiRenderer requires a GLFW window");
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = kSrvDescriptorCount;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device_.GetD3D12Device()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&srv_heap_)));

    descriptor_size_ = device_.GetD3D12Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(glfw_window), true);

    D3D12Queue& graphics_queue = static_cast<D3D12Queue&>(device_.GetQueue(QueueType::kGraphics));

    ImGui_ImplDX12_InitInfo init_info{};
    init_info.Device = device_.GetD3D12Device();
    init_info.CommandQueue = graphics_queue.GetQueue();
    init_info.NumFramesInFlight = static_cast<int>(swapchain.GetImageCount());
    init_info.RTVFormat = ImageToDXGIFormat(swapchain.GetFormat());
    init_info.SrvDescriptorHeap = srv_heap_.Get();
    init_info.SrvDescriptorAllocFn = AllocateSrvDescriptor;
    init_info.SrvDescriptorFreeFn = FreeSrvDescriptor;
    init_info.UserData = this;

    if (!ImGui_ImplDX12_Init(&init_info))
    {
        throw std::runtime_error("Failed to initialize ImGui D3D12 backend");
    }
}

D3D12ImGuiRenderer::~D3D12ImGuiRenderer()
{
    device_.WaitIdle();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void D3D12ImGuiRenderer::NewFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void D3D12ImGuiRenderer::Render(CommandBuffer& command_buffer)
{
    D3D12CommandBuffer* d3d12_command_buffer = dynamic_cast<D3D12CommandBuffer*>(&command_buffer);
    if (d3d12_command_buffer == nullptr)
    {
        throw std::runtime_error("D3D12ImGuiRenderer requires a D3D12 command buffer");
    }

    ImGui::Render();
    command_buffer.SetRenderTarget(swapchain_.GetCurrentImage(), nullptr);

    ID3D12DescriptorHeap* descriptor_heaps[] = {srv_heap_.Get()};
    d3d12_command_buffer->GetCommandList()->SetDescriptorHeaps(1, descriptor_heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12_command_buffer->GetCommandList());
}

void D3D12ImGuiRenderer::AllocateSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
{
    static_cast<D3D12ImGuiRenderer*>(info->UserData)->AllocateSrvDescriptor(out_cpu_handle, out_gpu_handle);
}

void D3D12ImGuiRenderer::FreeSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE)
{
    static_cast<D3D12ImGuiRenderer*>(info->UserData)->FreeSrvDescriptor(cpu_handle);
}

void D3D12ImGuiRenderer::AllocateSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
{
    uint32_t descriptor_index = 0;
    if (!free_descriptor_indices_.empty())
    {
        descriptor_index = free_descriptor_indices_.back();
        free_descriptor_indices_.pop_back();
    }
    else
    {
        if (next_descriptor_index_ >= kSrvDescriptorCount)
        {
            throw std::runtime_error("ImGui D3D12 SRV descriptor heap is full");
        }

        descriptor_index = next_descriptor_index_++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_start = srv_heap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_start = srv_heap_->GetGPUDescriptorHandleForHeapStart();
    out_cpu_handle->ptr = cpu_start.ptr + descriptor_index * descriptor_size_;
    out_gpu_handle->ptr = gpu_start.ptr + descriptor_index * descriptor_size_;
}

void D3D12ImGuiRenderer::FreeSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_start = srv_heap_->GetCPUDescriptorHandleForHeapStart();
    size_t offset = cpu_handle.ptr - cpu_start.ptr;
    if (offset % descriptor_size_ != 0)
    {
        throw std::runtime_error("Invalid ImGui D3D12 SRV descriptor handle");
    }

    free_descriptor_indices_.push_back(static_cast<uint32_t>(offset / descriptor_size_));
}

}  // namespace gpu
