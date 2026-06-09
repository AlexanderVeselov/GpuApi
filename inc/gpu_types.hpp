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

/// Triangle face culling mode used by rasterization.
enum class CullMode
{
    kNone,
    kFront,
    kBack
};

/// Blend factor used by graphics pipeline color and alpha blending.
enum class BlendFactor
{
    kZero,
    kOne,
    kSrcAlpha,
    kOneMinusSrcAlpha
};

/// Blend operation used by graphics pipeline color and alpha blending.
enum class BlendOp
{
    kAdd
};

/// Per-channel color write mask for a render target attachment.
enum ColorWriteMask : uint8_t
{
    kColorWriteNone = 0,
    kColorWriteR = 1 << 0,
    kColorWriteG = 1 << 1,
    kColorWriteB = 1 << 2,
    kColorWriteA = 1 << 3,
    kColorWriteRGBA = kColorWriteR | kColorWriteG | kColorWriteB | kColorWriteA
};

/// Per-render-target blending description for a graphics pipeline.
/// If fewer entries are provided than there are color attachments, the remaining
/// attachments default to disabled blending with full color writes enabled.
struct ColorBlendAttachmentDesc
{
    /// Enables fixed-function blending for this attachment.
    bool blend_enabled = false;
    /// Multiplier applied to the fragment color before blending.
    BlendFactor src_color_blend_factor = BlendFactor::kOne;
    /// Multiplier applied to the existing render target color before blending.
    BlendFactor dst_color_blend_factor = BlendFactor::kZero;
    /// Operation applied to the scaled source and destination color values.
    BlendOp color_blend_op = BlendOp::kAdd;
    /// Multiplier applied to the fragment alpha before blending.
    BlendFactor src_alpha_blend_factor = BlendFactor::kOne;
    /// Multiplier applied to the existing render target alpha before blending.
    BlendFactor dst_alpha_blend_factor = BlendFactor::kZero;
    /// Operation applied to the scaled source and destination alpha values.
    BlendOp alpha_blend_op = BlendOp::kAdd;
    /// Bitmask describing which color channels are written by this attachment.
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
    /// Vertex shader source filename resolved relative to the active shader search path.
    std::string vs_filename;
    /// Pixel/fragment shader source filename resolved relative to the active shader search path.
    std::string ps_filename;
    /// Name of the global constant buffer/root constants struct expected by the shaders.
    std::string root_constants_name = "g_RootConstants";
    /// Rasterization culling mode.
    CullMode cull_mode = CullMode::kNone;
    /// Enables depth testing.
    bool depth_enabled = false;
    /// Enables writing passing fragments to the depth attachment when depth testing is enabled.
    bool depth_write_enabled = true;
    /// Comparison function used by the depth test.
    DepthFunc depth_func = DepthFunc::kLess;
    /// Formats of the color attachments bound during rendering.
    std::vector<ImageFormat> color_attachment_formats;
    /// Optional per-attachment blend state. Missing entries fall back to disabled blending.
    std::vector<ColorBlendAttachmentDesc> color_blend_attachments;
    /// Format of the depth attachment when depth testing is enabled.
    ImageFormat depth_attachment_format = ImageFormat::kUnknown;
};

}  // namespace gpu
