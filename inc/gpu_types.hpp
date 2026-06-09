#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gpu
{
/// Unique ownership pointer used for API objects that cannot be shared safely.
template <class T>
using Ptr = std::unique_ptr<T>;

/// Shared ownership pointer used for GPU resources referenced by command buffers.
template <class T>
using SharedPtr = std::shared_ptr<T>;

class Api;
class AccelerationStructure;
class Device;
class Buffer;
class CommandBuffer;
class DescriptorSet;
class Device;
class Image;
class Sampler;
class Pipeline;
class GraphicsPipeline;
class ComputePipeline;
class ImGuiRenderer;
class Queue;
class Swapchain;

using DevicePtr = Ptr<Device>;
using AccelerationStructurePtr = SharedPtr<AccelerationStructure>;
using CommandBufferPtr = Ptr<CommandBuffer>;
using DescriptorSetPtr = Ptr<DescriptorSet>;
using GraphicsPipelinePtr = Ptr<GraphicsPipeline>;
using ComputePipelinePtr = Ptr<ComputePipeline>;
using ImGuiRendererPtr = Ptr<ImGuiRenderer>;
using SwapchainPtr = Ptr<Swapchain>;

using BufferPtr = SharedPtr<Buffer>;
using ImagePtr = SharedPtr<Image>;
using SamplerPtr = SharedPtr<Sampler>;

/// Queue capability class.
enum class QueueType
{
    kGraphics = 0,
    kCompute = 1,
    kTransfer = 2
};

/// Explicit image layout used by command buffer barriers.
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

/// Backend-independent resource format.
enum class ImageFormat
{
    kUnknown,
    kRGBA32_Float,
    kRGBA32_UInt,
    kRGBA32_SInt,
    kRGBA16_Float,
    kRGBA8_SInt,
    kRGBA8_UInt,
    kRGBA8_UNorm,
    kRGBA8_SRGB,
    kBGRA8_UNorm,
    kBGRA8_SRGB,
    kRGB32_Float,
    kRGB32_UInt,
    kRGB32_SInt,
    kRG32_Float,
    kRG32_UInt,
    kRG32_SInt,
    kRG16_Float,
    kR32_Float,
    kR32_UInt,
    kR32_SInt,
    kD32_Float,
    kR32_Typeless,
    kR16_Float
};

/// Depth comparison mode for graphics pipelines.
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

enum class CullMode
{
    kNone,
    kFront,
    kBack
};

enum class BlendFactor
{
    kZero,
    kOne,
    kSrcAlpha,
    kOneMinusSrcAlpha
};

enum class BlendOp
{
    kAdd
};

enum ColorWriteMask : uint8_t
{
    kColorWriteNone = 0,
    kColorWriteR = 1 << 0,
    kColorWriteG = 1 << 1,
    kColorWriteB = 1 << 2,
    kColorWriteA = 1 << 3,
    kColorWriteRGBA = kColorWriteR | kColorWriteG | kColorWriteB | kColorWriteA
};

struct ColorBlendAttachmentDesc
{
    bool blend_enabled = false;
    BlendFactor src_color_blend_factor = BlendFactor::kOne;
    BlendFactor dst_color_blend_factor = BlendFactor::kZero;
    BlendOp color_blend_op = BlendOp::kAdd;
    BlendFactor src_alpha_blend_factor = BlendFactor::kOne;
    BlendFactor dst_alpha_blend_factor = BlendFactor::kZero;
    BlendOp alpha_blend_op = BlendOp::kAdd;
    uint8_t color_write_mask = kColorWriteRGBA;
};

/// Raster viewport in floating-point pixels.
struct Viewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

/// Integer scissor rectangle in framebuffer pixels.
struct Rect
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

/// Immutable graphics pipeline creation parameters.
struct GraphicsPipelineDesc
{
    std::string vs_filename;
    std::string ps_filename;
    std::string root_constants_name = "g_RootConstants";
    CullMode cull_mode = CullMode::kNone;
    bool depth_enabled = false;
    bool depth_write_enabled = true;
    DepthFunc depth_func = DepthFunc::kLess;
    std::vector<ImageFormat> color_attachment_formats;
    std::vector<ColorBlendAttachmentDesc> color_blend_attachments;
    ImageFormat depth_attachment_format = ImageFormat::kUnknown;
};

}  // namespace gpu
