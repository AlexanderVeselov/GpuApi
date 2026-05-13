#pragma once

#include "d3d12_common.hpp"
#include "gpu_imgui.hpp"

#include <vector>

struct ImGui_ImplDX12_InitInfo;

namespace gpu
{
class D3D12Device;
class Swapchain;

class D3D12ImGuiRenderer final : public ImGuiRenderer
{
  public:
    D3D12ImGuiRenderer(D3D12Device& device, void* glfw_window, Swapchain& swapchain);
    ~D3D12ImGuiRenderer() override;

    void NewFrame() override;
    void Render(CommandBuffer& command_buffer) override;

  private:
    static void AllocateSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle);
    static void FreeSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);

    void AllocateSrvDescriptor(
        D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle);
    void FreeSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle);

  private:
    D3D12Device& device_;
    Swapchain& swapchain_;
    ComPtr<ID3D12DescriptorHeap> srv_heap_;
    uint32_t descriptor_size_ = 0;
    uint32_t next_descriptor_index_ = 0;
    std::vector<uint32_t> free_descriptor_indices_;
};

} // namespace gpu
