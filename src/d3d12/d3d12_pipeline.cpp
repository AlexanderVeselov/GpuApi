#include "d3d12_api.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include "../common/utils.hpp"
#include <d3d12shader.h>
#include <dxcapi.h>

namespace gpu
{
namespace
{
ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* d3d12_device)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 0;
    root_signature_desc.pParameters = nullptr;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3D10Blob> root_signature_blob;
    ComPtr<ID3D10Blob> root_signature_error_blob;
    ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
        &root_signature_blob, &root_signature_error_blob));

    ComPtr<ID3D12RootSignature> root_signature;
    ThrowIfFailed(d3d12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

    return root_signature;
}
}

D3D12GraphicsPipeline::D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc)
    : GraphicsPipeline(pipeline_desc)
    , device_(device)
{
    auto d3d12_device = device_.GetD3D12Device();

    root_signature_ = CreateRootSignature(d3d12_device);
    D3D12ShaderManager& shader_manager = device_.GetD3D12Api().GetShaderManager();

    D3D12Shader vs_shader = shader_manager.CompileShader(pipeline_desc.vs_filename.c_str(), "main", "vs_6_0");
    D3D12_SHADER_BYTECODE vs_bytecode = {};
    vs_bytecode.BytecodeLength = vs_shader.dxc_blob->GetBufferSize();
    vs_bytecode.pShaderBytecode = vs_shader.dxc_blob->GetBufferPointer();

    D3D12Shader ps_shader = shader_manager.CompileShader(pipeline_desc.ps_filename.c_str(), "main", "ps_6_0");
    D3D12_SHADER_BYTECODE ps_bytecode = {};
    ps_bytecode.BytecodeLength = ps_shader.dxc_blob->GetBufferSize();
    ps_bytecode.pShaderBytecode = ps_shader.dxc_blob->GetBufferPointer();

    ///@TODO: add blend support
    D3D12_BLEND_DESC blend_state = {};
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

    D3D12_DEPTH_STENCIL_DESC depth_stencil_state = {};
    depth_stencil_state.DepthEnable = (pipeline_desc_.depth_attachment != nullptr);
    depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_state.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_state.StencilEnable = false;

    ///@TODO: make configurable input layout
    D3D12_INPUT_ELEMENT_DESC input_element_descs[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_LAYOUT_DESC input_layout = {};
    input_layout.pInputElementDescs = input_element_descs;
    input_layout.NumElements = 2u;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = root_signature_.Get();
    pipeline_state_desc.VS = vs_bytecode;
    pipeline_state_desc.PS = ps_bytecode;
    pipeline_state_desc.BlendState = blend_state;
    pipeline_state_desc.SampleMask = UINT_MAX;
    pipeline_state_desc.RasterizerState = rasterizer_state;
    pipeline_state_desc.DepthStencilState = depth_stencil_state;
    pipeline_state_desc.InputLayout = input_layout;
    pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Color attachments
    pipeline_state_desc.NumRenderTargets = pipeline_desc_.color_attachments.size();

    for (std::uint32_t rt_index = 0; rt_index < pipeline_desc_.color_attachments.size(); ++rt_index)
    {
        auto& rt = pipeline_desc_.color_attachments[rt_index];
        D3D12Image* d3d12_rt = static_cast<D3D12Image*>(rt.get());
        pipeline_state_desc.RTVFormats[rt_index] = d3d12_rt->GetDXGIFormat();
    }

    // Depth attachment
    if (pipeline_desc_.depth_attachment != nullptr)
    {
        D3D12Image* d3d12_depth_image = static_cast<D3D12Image*>(pipeline_desc_.depth_attachment.get());
        pipeline_state_desc.DSVFormat = d3d12_depth_image->GetDXGIFormat();
    }

    pipeline_state_desc.SampleDesc.Count = 1u;
    pipeline_state_desc.SampleDesc.Quality = 0u;

    ThrowIfFailed(d3d12_device->CreateGraphicsPipelineState(&pipeline_state_desc,
        IID_PPV_ARGS(&pipeline_state_)));
}

D3D12ComputePipeline::D3D12ComputePipeline(D3D12Device& device, char const* cs_filename)
    : device_(device)
{
    auto d3d12_device = device_.GetD3D12Device();
    D3D12ShaderManager& shader_manager = device_.GetD3D12Api().GetShaderManager();

    root_signature_ = CreateRootSignature(d3d12_device);
    D3D12Shader cs_shader = shader_manager.CompileShader(cs_filename, "main", "cs_6_0");

    D3D12_SHADER_BYTECODE cs_bytecode = {};
    cs_bytecode.BytecodeLength = cs_shader.dxc_blob->GetBufferSize();
    cs_bytecode.pShaderBytecode = cs_shader.dxc_blob->GetBufferPointer();

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = root_signature_.Get();
    pipeline_state_desc.CS = cs_bytecode;

    ThrowIfFailed(d3d12_device->CreateComputePipelineState(&pipeline_state_desc,
        IID_PPV_ARGS(&pipeline_state_)));
}

} // namespace gpu
