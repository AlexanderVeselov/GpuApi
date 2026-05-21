# GpuApi

GpuApi is a compact C++17 rendering abstraction over Direct3D 12 and Vulkan.
It exposes the pieces needed by the renderer without trying to hide the
explicit GPU programming model: devices, queues, command buffers, buffers,
images, swapchains, pipelines, descriptor sets, samplers and acceleration
structures.

The library is currently Windows-focused. Shaders are written in HLSL and
compiled with DXC for both backends. Vulkan shaders are compiled to SPIR-V and
reflected with SPIRV-Reflect.

## Features

- Direct3D 12 and Vulkan backend selection through `gpu::Api::Create`
- Graphics pipelines with reflected vertex inputs, render target formats and optional depth state
- Compute pipelines with reflected resource bindings
- Descriptor sets for buffers, images, image arrays, samplers and acceleration structures
- Buffer creation with CPU access, shader-resource, storage and acceleration-structure usage flags
- Images with render target, depth/stencil, shader resource, storage, mip and array support
- Swapchain creation from a Win32 window handle
- Command buffers for draw, dispatch, copies, clears, barriers, render target binding, viewport and scissor
- Runtime pipeline hot reload through `Device::ReloadPipelines`
- Hardware ray tracing support through Ray Query acceleration structures
- [Dear ImGui](https://github.com/ocornut/imgui) integration for both backends

## Repository Layout

```text
inc/                     Public backend-independent API
src/common/              Shared API helpers, pipeline reload and shader reflection types
src/d3d12/               Direct3D 12 backend
src/vulkan/              Vulkan backend
samples/hello_triangle/  Graphics pipeline sample
samples/rotating_cube/   Indexed graphics and depth-buffer sample
samples/shadertoy/       Compute pipeline sample
third_party/             Bundled DXC, GLFW binaries and SPIRV-Reflect
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

The sample CMake files copy `dxcompiler.dll` and `dxil.dll` next to the built
executables.

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

Switching backends is a one-line change:

```cpp
auto api = gpu::Api::Create(gpu::ApiType::kD3D12);
// or
auto api = gpu::Api::Create(gpu::ApiType::kVulkan);
```

## Samples

`samples/hello_triangle` creates a swapchain, graphics pipeline, vertex buffer,
descriptor set and renders a triangle.

`samples/rotating_cube` renders an indexed, depth-tested cube with a per-frame
MVP constant buffer.

`samples/shadertoy` creates a storage image, dispatches a compute shader into
it, then copies the result into the swapchain image.

## Current Limitations

- The platform layer is Win32-only.
- Shaders are loaded by filename from the configured shader path.
- Hardware ray tracing is limited to Ray Query from compute shaders.
- Ray tracing pipelines, shader binding tables and `DispatchRays` are not part of the API.
- Descriptor image binding currently uses the default image view unless a specific `ImageView` is provided.
- Synchronization is intentionally explicit; callers are responsible for image layout transitions and pass ordering.
- The API is still evolving and favors clarity over broad feature coverage.
