# GpuApi

GpuApi is a small C++ rendering abstraction over Direct3D 12 and Vulkan. The project is intentionally compact: it exposes the pieces needed to create devices, queues, buffers, images, swapchains, graphics pipelines, compute pipelines, descriptor sets, and command buffers without hiding the explicit GPU programming model.

The codebase is currently Windows-focused and uses HLSL shaders compiled through DXC for both backends. Vulkan shaders are compiled to SPIR-V and reflected with SPIRV-Reflect.

## Features

- Direct3D 12 and Vulkan backend selection through `gpu::Api::Create`.
- Graphics pipelines with vertex/pixel shaders, reflected vertex inputs, render target formats, and optional depth state.
- Compute pipelines with reflected resource bindings.
- Descriptor sets for buffers and images.
- Buffers with CPU mapping support.
- Images with render target, depth/stencil, shader resource, and storage usage flags.
- Swapchain creation from a Win32 window handle.
- Command buffers for draw, dispatch, barriers, clears, copies, render target binding, viewport, and scissor.
- Vulkan dynamic rendering path, without a fixed render pass/framebuffer API.

## Repository Layout

```text
inc/                     Public backend-independent API
src/common/              Shared API helpers and shader reflection types
src/d3d12/               Direct3D 12 backend
src/vulkan/              Vulkan backend
samples/hello_triangle/  Graphics pipeline sample
samples/rotating_cube/   Indexed graphics and depth-buffer sample
samples/shadertoy/       Compute pipeline sample
third_party/             Bundled DXC, GLFW binaries, and SPIRV-Reflect
```

## Requirements

- Windows
- CMake 3.22 or newer
- A C++17-capable MSVC toolchain
- Vulkan SDK available to CMake through `find_package(Vulkan)`
- GLFW discoverable by `find_package(glfw3)`

DXC and SPIRV-Reflect are included under `third_party/`.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

The sample targets are built with the library:

- `HelloTriangle`
- `RotatingCube`
- `Shadertoy`

The sample CMake files copy `dxcompiler.dll` and `dxil.dll` next to the built executables.

## Basic Usage

```cpp
#include "gpu_api.hpp"
#include "gpu_device.hpp"
#include "gpu_pipeline.hpp"
#include "gpu_queue.hpp"
#include "gpu_swapchain.hpp"

auto api = gpu::Api::Create(gpu::ApiType::kVulkan);
gpu::DevicePtr device = api->CreateDevice();

gpu::SwapchainPtr swapchain = device->CreateSwapchain(hwnd,
    width, height, 3);

gpu::GraphicsPipelineDesc desc;
desc.vs_filename = "shader.vs";
desc.ps_filename = "shader.ps";
desc.color_attachment_formats = { swapchain->GetFormat() };

gpu::GraphicsPipelinePtr pipeline = device->CreateGraphicsPipeline(desc);
gpu::Queue& queue = device->GetQueue(gpu::QueueType::kGraphics);

gpu::CommandBufferPtr cmd = queue.CreateCommandBuffer();
gpu::ImagePtr backbuffer = swapchain->GetCurrentImage();

cmd->TransitionBarrier(backbuffer, gpu::ImageLayout::kPresent, gpu::ImageLayout::kRenderTarget);
cmd->ClearImage(backbuffer, 0.1f, 0.1f, 0.1f, 1.0f);
cmd->BindPipeline(pipeline);
cmd->SetRenderTarget(backbuffer, nullptr);
cmd->Draw(3);
cmd->TransitionBarrier(backbuffer, gpu::ImageLayout::kRenderTarget, gpu::ImageLayout::kPresent);

queue.Submit(std::move(cmd));
swapchain->Present();
```

## Samples

`samples/hello_triangle` creates a swapchain, graphics pipeline, vertex buffer, descriptor set, and renders a triangle.

`samples/rotating_cube` renders an indexed, depth-tested cube with a per-frame MVP constant buffer.

`samples/shadertoy` creates a storage image, dispatches a compute shader into it, then copies the result into the swapchain image.

Switching backends is an one-line change:

```cpp
auto api = gpu::Api::Create(gpu::ApiType::kD3D12);
// or
auto api = gpu::Api::Create(gpu::ApiType::kVulkan);
```

## Current Limitations

- The platform layer is Win32-only.
- Shaders are loaded by filename from the sample working directory.
- Descriptor image binding currently uses the default image view; explicit `ImageView` mip/slice views need more Vulkan image view support.
- Synchronization is intentionally simple and explicit; callers are responsible for correct layout transitions.
- The API is still evolving and favors clarity over broad feature coverage.
