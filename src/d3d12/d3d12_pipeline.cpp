#include "d3d12_pipeline.hpp"
#include "../common/utils.hpp"
#include "d3d12_api.hpp"
#include "d3d12_descriptor_set.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include <cassert>
#include <dxcapi.h>
#include <stdexcept>

namespace gpu
{
namespace
{
D3D12_COMPARISON_FUNC DepthFuncToD3D12(DepthFunc depth_func)
{
    switch (depth_func)
    {
    case DepthFunc::kNone:
        return D3D12_COMPARISON_FUNC_NONE;
    case DepthFunc::kNever:
        return D3D12_COMPARISON_FUNC_NEVER;
    case DepthFunc::kLess:
        return D3D12_COMPARISON_FUNC_LESS;
    case DepthFunc::kEqual:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case DepthFunc::kLessEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case DepthFunc::kGreater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case DepthFunc::kNotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case DepthFunc::kGreaterEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case DepthFunc::kAlways:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    default:
        assert(!"DepthFuncToD3D12: unknown depth func");
        return D3D12_COMPARISON_FUNC_NONE;
    }
}

DXGI_FORMAT VertexInputFormatToDXGI(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::kR32_Float:
        return DXGI_FORMAT_R32_FLOAT;
    case ImageFormat::kR32_UInt:
        return DXGI_FORMAT_R32_UINT;
    case ImageFormat::kR32_SInt:
        return DXGI_FORMAT_R32_SINT;
    case ImageFormat::kRG32_Float:
        return DXGI_FORMAT_R32G32_FLOAT;
    case ImageFormat::kRG32_UInt:
        return DXGI_FORMAT_R32G32_UINT;
    case ImageFormat::kRG32_SInt:
        return DXGI_FORMAT_R32G32_SINT;
    case ImageFormat::kRGB32_Float:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case ImageFormat::kRGB32_UInt:
        return DXGI_FORMAT_R32G32B32_UINT;
    case ImageFormat::kRGB32_SInt:
        return DXGI_FORMAT_R32G32B32_SINT;
    case ImageFormat::kRGBA32_Float:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case ImageFormat::kRGBA32_UInt:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case ImageFormat::kRGBA32_SInt:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    default:
        assert(!"VertexInputFormatToDXGI(...): unsupported vertex input format");
        return DXGI_FORMAT_UNKNOWN;
    }
}

uint32_t VertexInputFormatSize(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::kR32_Float:
    case ImageFormat::kR32_UInt:
    case ImageFormat::kR32_SInt:
        return 4;
    case ImageFormat::kRG32_Float:
    case ImageFormat::kRG32_UInt:
    case ImageFormat::kRG32_SInt:
        return 8;
    case ImageFormat::kRGB32_Float:
    case ImageFormat::kRGB32_UInt:
    case ImageFormat::kRGB32_SInt:
        return 12;
    case ImageFormat::kRGBA32_Float:
    case ImageFormat::kRGBA32_UInt:
    case ImageFormat::kRGBA32_SInt:
        return 16;
    default:
        assert(!"VertexInputFormatSize(...): unsupported vertex input format");
        return 0;
    }
}

void GetInputElementDescs(ShaderReflection const& reflection, std::vector<D3D12_INPUT_ELEMENT_DESC>& element_descs)
{
    for (ShaderInputParameter const& parameter : reflection.input_parameters)
    {
        if (parameter.is_system_value)
        {
            continue;
        }

        D3D12_INPUT_ELEMENT_DESC input_element_desc = {};
        input_element_desc.SemanticName = parameter.semantic_name.c_str();
        input_element_desc.SemanticIndex = parameter.semantic_index;
        input_element_desc.Format = VertexInputFormatToDXGI(parameter.format);
        input_element_desc.InputSlot = 0;
        input_element_desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        input_element_desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element_desc.InstanceDataStepRate = 0;

        element_descs.push_back(input_element_desc);
    }
}

uint32_t CalculateVertexStride(ShaderReflection const& reflection)
{
    uint32_t stride = 0;
    for (ShaderInputParameter const& parameter : reflection.input_parameters)
    {
        if (!parameter.is_system_value)
        {
            stride += VertexInputFormatSize(parameter.format);
        }
    }

    return stride;
}

}  // namespace

D3D12Pipeline::D3D12Pipeline(D3D12Device& device) : device_(device), layout_(device) {}

D3D12Pipeline::~D3D12Pipeline()
{
    device_.UnregisterPipeline(this);
}

DescriptorSetPtr D3D12Pipeline::CreateDescriptorSet()
{
    return std::make_unique<D3D12DescriptorSet>(device_, layout_);
}

D3D12GraphicsPipeline::D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc)
    : GraphicsPipeline(pipeline_desc), D3D12Pipeline(device)
{
    Reload();
}

void D3D12GraphicsPipeline::Reload()
{
    auto d3d12_device = device_.GetD3D12Device();

    D3D12ShaderManager& shader_manager = device_.GetD3D12Api().GetShaderManager();

    D3D12Shader vs_shader = shader_manager.CompileShader(pipeline_desc_.vs_filename.c_str(), "main", "vs_6_0");
    D3D12_SHADER_BYTECODE vs_bytecode = {};
    vs_bytecode.BytecodeLength = vs_shader.dxc_blob->GetBufferSize();
    vs_bytecode.pShaderBytecode = vs_shader.dxc_blob->GetBufferPointer();

    D3D12Shader ps_shader = shader_manager.CompileShader(pipeline_desc_.ps_filename.c_str(), "main", "ps_6_0");
    D3D12_SHADER_BYTECODE ps_bytecode = {};
    ps_bytecode.BytecodeLength = ps_shader.dxc_blob->GetBufferSize();
    ps_bytecode.pShaderBytecode = ps_shader.dxc_blob->GetBufferPointer();

    if (!pipeline_state_)
    {
        layout_.Build({&vs_shader, &ps_shader});
    }
    else
    {
        // Check if the new pipeline layout is compatible with the existing layout.
        D3D12PipelineLayout new_layout(device_);
        new_layout.Build({&vs_shader, &ps_shader});
        if (!layout_.IsCompatibleWith(new_layout))
        {
            throw std::runtime_error("D3D12GraphicsPipeline::Reload: pipeline layout changed");
        }
    }

    ///@TODO: add blend support
    D3D12_BLEND_DESC blend_state = {};
    blend_state.AlphaToCoverageEnable = FALSE;
    blend_state.IndependentBlendEnable = FALSE;
    blend_state.RenderTarget[0].BlendEnable = FALSE;
    blend_state.RenderTarget[0].LogicOpEnable = FALSE;
    blend_state.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blend_state.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    blend_state.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend_state.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend_state.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blend_state.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend_state.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blend_state.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RASTERIZER_DESC rasterizer_state = {};
    rasterizer_state.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_state.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_state.FrontCounterClockwise = FALSE;
    rasterizer_state.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_state.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_state.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_state.DepthClipEnable = TRUE;
    rasterizer_state.MultisampleEnable = FALSE;
    rasterizer_state.AntialiasedLineEnable = FALSE;
    rasterizer_state.ForcedSampleCount = 0;

    D3D12_DEPTH_STENCIL_DESC depth_stencil_state = {};
    depth_stencil_state.DepthEnable = pipeline_desc_.depth_enabled;
    depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_state.DepthFunc = DepthFuncToD3D12(pipeline_desc_.depth_func);
    depth_stencil_state.StencilEnable = FALSE;
    depth_stencil_state.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil_state.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_stencil_state.FrontFace = {};
    depth_stencil_state.BackFace = {};

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_descs;
    GetInputElementDescs(vs_shader.reflection, input_element_descs);
    vertex_stride_ = CalculateVertexStride(vs_shader.reflection);

    D3D12_INPUT_LAYOUT_DESC input_layout = {};
    input_layout.pInputElementDescs = input_element_descs.data();
    input_layout.NumElements = (uint32_t)input_element_descs.size();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = layout_.GetRootSignature();
    pipeline_state_desc.VS = vs_bytecode;
    pipeline_state_desc.PS = ps_bytecode;
    pipeline_state_desc.BlendState = blend_state;
    pipeline_state_desc.SampleMask = UINT_MAX;
    pipeline_state_desc.RasterizerState = rasterizer_state;
    pipeline_state_desc.DepthStencilState = depth_stencil_state;
    pipeline_state_desc.InputLayout = input_layout;
    pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Color attachments
    pipeline_state_desc.NumRenderTargets = pipeline_desc_.color_attachment_formats.size();
    assert(pipeline_state_desc.NumRenderTargets < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

    for (uint32_t rt_index = 0; rt_index < pipeline_desc_.color_attachment_formats.size(); ++rt_index)
    {
        auto img_format = pipeline_desc_.color_attachment_formats[rt_index];
        pipeline_state_desc.RTVFormats[rt_index] = ImageToDXGIFormat(img_format);
    }

    // Depth attachment
    if (pipeline_desc_.depth_enabled)
    {
        pipeline_state_desc.DSVFormat = ImageToDXGIFormat(pipeline_desc_.depth_attachment_format);
    }

    pipeline_state_desc.SampleDesc.Count = 1u;
    pipeline_state_desc.SampleDesc.Quality = 0u;

    ComPtr<ID3D12PipelineState> new_pipeline_state;
    ThrowIfFailed(d3d12_device->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(&new_pipeline_state)));

    pipeline_state_ = std::move(new_pipeline_state);
}

D3D12ComputePipeline::D3D12ComputePipeline(D3D12Device& device, char const* cs_filename)
    : ComputePipeline(cs_filename), D3D12Pipeline(device)
{
    Reload();
}

void D3D12ComputePipeline::Reload()
{
    auto d3d12_device = device_.GetD3D12Device();
    D3D12ShaderManager& shader_manager = device_.GetD3D12Api().GetShaderManager();

    D3D12Shader cs_shader = shader_manager.CompileShader(cs_filename_.c_str(), "main", "cs_6_5");
    D3D12_SHADER_BYTECODE cs_bytecode = {};
    cs_bytecode.BytecodeLength = cs_shader.dxc_blob->GetBufferSize();
    cs_bytecode.pShaderBytecode = cs_shader.dxc_blob->GetBufferPointer();

    if (!pipeline_state_)
    {
        layout_.Build({&cs_shader});
    }
    else
    {
        // Check if the new pipeline layout is compatible with the existing layout.
        D3D12PipelineLayout new_layout(device_);
        new_layout.Build({&cs_shader});
        if (!layout_.IsCompatibleWith(new_layout))
        {
            throw std::runtime_error("D3D12ComputePipeline::Reload: pipeline layout changed");
        }
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = layout_.GetRootSignature();
    pipeline_state_desc.CS = cs_bytecode;

    ComPtr<ID3D12PipelineState> new_pipeline_state;
    ThrowIfFailed(d3d12_device->CreateComputePipelineState(&pipeline_state_desc, IID_PPV_ARGS(&new_pipeline_state)));

    pipeline_state_ = std::move(new_pipeline_state);
}

}  // namespace gpu
