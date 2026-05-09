#pragma once

#include <memory>
#include <string>
#include <vector>

namespace gpu
{
template <class T>
using Ptr = std::unique_ptr<T>;

template <class T>
using SharedPtr = std::shared_ptr<T>;

class Api;
class Device;
class Buffer;
class CommandBuffer;
class DescriptorSet;
class Device;
class Image;
class Pipeline;
class GraphicsPipeline;
class ComputePipeline;
class Queue;
class Swapchain;
class Semaphore;
class Fence;

// Unique resources
using DevicePtr             = Ptr<Device>;
using CommandBufferPtr      = Ptr<CommandBuffer>;
using DescriptorSetPtr      = Ptr<DescriptorSet>;
//using PipelinePtr           = Ptr<Pipeline>;
using GraphicsPipelinePtr   = Ptr<GraphicsPipeline>;
using ComputePipelinePtr    = Ptr<ComputePipeline>;
using SwapchainPtr          = Ptr<Swapchain>;

// Shared resources
using BufferPtr             = SharedPtr<Buffer>;
using ImagePtr              = SharedPtr<Image>;
using SemaphorePtr          = SharedPtr<Semaphore>;
using FencePtr              = SharedPtr<Fence>;

enum class QueueType
{
    kGraphics = 0,
    kCompute  = 1,
    kTransfer = 2
};

enum class ImageLayout
{
    kUndefined,
    kPresent,
    kCopySrc,
    kCopyDst,
    kRenderTarget,
    kShaderRead,
    kShaderReadWrite
};

enum class ImageFormat
{
    kUnknown,
    kRGBA32_Float,
    kRGBA16_Float,
    kRGBA8_SInt,
    kRGBA8_UInt,
    kRGBA8_UNorm,
    kRGBA8_SRGB,
    kRG32_Float,
    kRG16_Float,
    kR32_Float,
    kD32_Float,
    kR32_Typeless,
    kR16_Float
};

enum class DepthFunc
{
    kNone,
    kNever,
    kLess,
    kEqual,
    kLessEqual,
    kGreater,
    kNotEqual,
    kGreaterEqual,
    kAlways
};

struct Viewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

struct Rect
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct GraphicsPipelineDesc
{
    std::string vs_filename;
    std::string ps_filename;
    bool depth_enabled = false;
    DepthFunc depth_func = DepthFunc::kLess;
    std::vector<ImageFormat> color_attachment_formats;
    ImageFormat depth_attachment_format = ImageFormat::kUnknown;
};

} // namespace gpu
