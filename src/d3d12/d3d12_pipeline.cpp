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
    }

    D3D12GraphicsPipeline::D3D12GraphicsPipeline(D3D12Device& device, GraphicsPipelineDesc const& pipeline_desc)
        : GraphicsPipeline(pipeline_desc)
        , device_(device)
    {
        auto d3d12_device = device_.GetD3D12Device();

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
        root_signature_desc.NumParameters = 0;
        root_signature_desc.pParameters = nullptr;
        root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;//D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
            std::cout << (char*)error_blob->GetBufferPointer() << std::endl;
            throw D3D12Exception(hr, __FILE__, __LINE__);
        }

        hr = (D3DCompileFromFile(StringToWstring(pipeline_desc_.ps_filename).c_str(),
            nullptr, nullptr, "main",
            "ps_5_1", compile_flags, 0, &ps_blob, &error_blob));

        if (FAILED(hr))
        {
            std::cout << (char*)error_blob->GetBufferPointer() << std::endl;
            throw D3D12Exception(hr, __FILE__, __LINE__);
        }

        D3D12_SHADER_BYTECODE vs_bytecode = {};
        vs_bytecode.BytecodeLength = vs_blob->GetBufferSize();
        vs_bytecode.pShaderBytecode = vs_blob->GetBufferPointer();

        D3D12_SHADER_BYTECODE ps_bytecode = {};
        ps_bytecode.BytecodeLength = ps_blob->GetBufferSize();
        ps_bytecode.pShaderBytecode = ps_blob->GetBufferPointer();

        ///@TODO: add blend support
        D3D12_BLEND_DESC blend_state = {};

        D3D12_RASTERIZER_DESC rasterizer_state = {};
        rasterizer_state.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizer_state.CullMode = D3D12_CULL_MODE_NONE;

        D3D12_DEPTH_STENCIL_DESC depth_stencil_state = {};
        depth_stencil_state.DepthEnable = (pipeline_desc_.depth_attachment != nullptr);
        depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depth_stencil_state.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depth_stencil_state.StencilEnable = false;

        ///@TODO: make configurable input layout
        D3D12_INPUT_ELEMENT_DESC input_element_desc = {};
        input_element_desc.SemanticName = "POSITION";
        input_element_desc.SemanticIndex = 0;
        input_element_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        input_element_desc.InputSlot = 0;
        input_element_desc.AlignedByteOffset = 0;
        input_element_desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element_desc.InstanceDataStepRate = 0;

        D3D12_INPUT_LAYOUT_DESC input_layout = {};
        input_layout.pInputElementDescs = nullptr;// &input_element_desc;
        input_layout.NumElements = 0u;//1u;

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

}
