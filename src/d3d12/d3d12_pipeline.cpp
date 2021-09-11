#include "d3d12_pipeline.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"

#include <d3dcompiler.h>
#include <iostream>
#include <codecvt>

namespace gpu
{
namespace
{
std::wstring StringToWstring(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

ID3D12RootSignature* CreateRootSignature(ID3D12Device* d3d12_device, ID3D12ShaderReflection* shader_reflection)
{
    D3D12_SHADER_DESC shader_desc = {};
    ThrowIfFailed(shader_reflection->GetDesc(&shader_desc));
    std::uint32_t resource_idx = 0;
    //for (std::uint32_t resource_idx = 0; resource_idx < shader_desc.BoundResources; ++resource_idx)
    //{
        D3D12_SHADER_INPUT_BIND_DESC resource_desc = {};
        ThrowIfFailed(shader_reflection->GetResourceBindingDesc(resource_idx, &resource_desc));
    //}

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 0;
    root_signature_desc.pParameters = nullptr;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3D10Blob> root_signature_blob;
    ComPtr<ID3D10Blob> root_signature_error_blob;
    ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
        &root_signature_blob, &root_signature_error_blob));

    ID3D12RootSignature* root_signature;
    ThrowIfFailed(d3d12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

    return root_signature;
}
} // namespace

D3D12GraphicsPipeline::D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc)
    : GraphicsPipeline(pipeline_desc)
    , device_(device)
{
    auto d3d12_device = device_.GetD3D12Device();

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 0;
    root_signature_desc.pParameters = nullptr;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3D10Blob> root_signature_blob;
    ComPtr<ID3D10Blob> root_signature_error_blob;
    ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
        &root_signature_blob, &root_signature_error_blob));

    ThrowIfFailed(d3d12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));

#ifndef NDEBUG
    // Enable shader debugging
    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compile_flags = 0;
#endif

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    ComPtr<ID3DBlob> error_blob;
    HRESULT hr = (D3DCompileFromFile(StringToWstring(pipeline_desc_.vs_filename).c_str(),
        nullptr, nullptr, "main",
        "vs_5_1", compile_flags, 0, &vs_blob, &error_blob));

    if (FAILED(hr))
    {
        std::string error_message = "Failed to compile shader: ";
        error_message += (char*)error_blob->GetBufferPointer();
        throw D3D12Exception(error_message.c_str(), hr, __FILE__, __LINE__);
    }

    hr = (D3DCompileFromFile(StringToWstring(pipeline_desc_.ps_filename).c_str(),
        nullptr, nullptr, "main",
        "ps_5_1", compile_flags, 0, &ps_blob, &error_blob));

    if (FAILED(hr))
    {
        std::string error_message = "Failed to compile shader: ";
        error_message += (char*)error_blob->GetBufferPointer();
        throw D3D12Exception(error_message.c_str(), hr, __FILE__, __LINE__);
    }

    D3D12_SHADER_BYTECODE vs_bytecode = {};
    vs_bytecode.BytecodeLength = vs_blob->GetBufferSize();
    vs_bytecode.pShaderBytecode = vs_blob->GetBufferPointer();

    D3D12_SHADER_BYTECODE ps_bytecode = {};
    ps_bytecode.BytecodeLength = ps_blob->GetBufferSize();
    ps_bytecode.pShaderBytecode = ps_blob->GetBufferPointer();

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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

#ifndef NDEBUG
    // Enable shader debugging
    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compile_flags = 0;
#endif

    ComPtr<ID3DBlob> cs_blob;
    ComPtr<ID3DBlob> error_blob;
    HRESULT hr = (D3DCompileFromFile(StringToWstring(cs_filename).c_str(),
        nullptr, nullptr, "main",
        "cs_5_1", compile_flags, 0, &cs_blob, &error_blob));

    if (FAILED(hr))
    {
        std::string error_message = "Failed to compile shader: ";
        error_message += (char*)error_blob->GetBufferPointer();
        throw D3D12Exception(error_message.c_str(), hr, __FILE__, __LINE__);
    }

    ComPtr<ID3D12ShaderReflection> shader_reflection;
    ThrowIfFailed(D3DReflect(cs_blob->GetBufferPointer(), cs_blob->GetBufferSize(),
        IID_PPV_ARGS(&shader_reflection)));

    root_signature_ = CreateRootSignature(d3d12_device, shader_reflection.Get());

    D3D12_SHADER_BYTECODE cs_bytecode = {};
    cs_bytecode.BytecodeLength = cs_blob->GetBufferSize();
    cs_bytecode.pShaderBytecode = cs_blob->GetBufferPointer();

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = root_signature_.Get();
    pipeline_state_desc.CS = cs_bytecode;

    ThrowIfFailed(d3d12_device->CreateComputePipelineState(&pipeline_state_desc,
        IID_PPV_ARGS(&pipeline_state_)));
}

} // namespace gpu
