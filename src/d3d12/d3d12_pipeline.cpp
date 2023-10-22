#include "d3d12_api.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_image.hpp"
#include "../common/utils.hpp"
#include <d3d12shader.h>
#include <dxcapi.h>
#include <cassert>

namespace gpu
{
namespace
{
D3D12_SHADER_VISIBILITY GetShaderVisibility(D3D12_SHADER_VERSION_TYPE version_type)
{
    switch (version_type)
    {
    case D3D12_SHVER_PIXEL_SHADER:
        return D3D12_SHADER_VISIBILITY_PIXEL;
    case D3D12_SHVER_VERTEX_SHADER:
        return D3D12_SHADER_VISIBILITY_VERTEX;
    case D3D12_SHVER_GEOMETRY_SHADER:
        return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case D3D12_SHVER_HULL_SHADER:
        return D3D12_SHADER_VISIBILITY_HULL;
    case D3D12_SHVER_DOMAIN_SHADER:
        return D3D12_SHADER_VISIBILITY_DOMAIN;
    case D3D12_SHVER_COMPUTE_SHADER:
        return D3D12_SHADER_VISIBILITY_ALL;
    default:
        assert(!"Unknown shader type");
        return D3D12_SHADER_VISIBILITY_ALL;
    }
}

D3D12_DESCRIPTOR_RANGE_TYPE GetRangeType(D3D_SHADER_INPUT_TYPE type)
{
    switch (type)
    {
    case D3D_SIT_CBUFFER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case D3D_SIT_SAMPLER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    default:
        assert(!"GetRangeType(...): D3D_SHADER_INPUT_TYPE is not supported");
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }

}

D3D12_ROOT_PARAMETER_TYPE GetRootParameterType(D3D_SHADER_INPUT_TYPE type)
{
    switch (type)
    {
    case D3D_SIT_CBUFFER:
        return D3D12_ROOT_PARAMETER_TYPE_CBV;
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
        return D3D12_ROOT_PARAMETER_TYPE_SRV;
    case D3D_SIT_SAMPLER:
        return D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
        return D3D12_ROOT_PARAMETER_TYPE_UAV;
    default:
        assert(!"GetRootParameterType(...): D3D_SHADER_INPUT_TYPE is not supported");
        return D3D12_ROOT_PARAMETER_TYPE_SRV;
    }
}

void CollectRootParameters(ID3D12ShaderReflection* reflection,
    std::vector<D3D12_DESCRIPTOR_RANGE>& descriptor_ranges,
    std::vector<D3D12_ROOT_PARAMETER>& root_parameters)
{
    // Get shader description
    D3D12_SHADER_DESC shader_desc = {};
    ThrowIfFailed(reflection->GetDesc(&shader_desc));
    for (std::uint32_t i = 0; i < shader_desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC resource_desc = {};
        ThrowIfFailed(reflection->GetResourceBindingDesc(i, &resource_desc));
        ID3D12ShaderReflectionConstantBuffer* buf = reflection->GetConstantBufferByIndex(i);

        if (std::string(resource_desc.Name) == "$Globals")
        {
            assert(!"Root constants are not supported for now");
        }
        printf("Resource %s\n", resource_desc.Name);

        D3D12_SHADER_BUFFER_DESC buffer_desc = {};
        buf->GetDesc(&buffer_desc);

        D3D12_ROOT_PARAMETER root_parameter = {};
        root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//GetRootParameterType(resource_desc.Type);
        root_parameter.ShaderVisibility = GetShaderVisibility(
            (D3D12_SHADER_VERSION_TYPE)D3D12_SHVER_GET_TYPE(shader_desc.Version));

        if (root_parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            assert(!"Root constants are not supported for now");
            root_parameter.Constants.ShaderRegister = resource_desc.BindPoint;
            root_parameter.Constants.RegisterSpace = resource_desc.Space;
            root_parameter.Constants.Num32BitValues = buffer_desc.Size / 4;
        }
        else
        {
            D3D12_DESCRIPTOR_RANGE descriptor_range;
            descriptor_range.RangeType = GetRangeType(resource_desc.Type);
            descriptor_range.NumDescriptors = resource_desc.BindCount;
            descriptor_range.BaseShaderRegister = resource_desc.BindPoint;
            descriptor_range.RegisterSpace = resource_desc.Space;
            descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            root_parameter.DescriptorTable.NumDescriptorRanges = 1;
            root_parameter.DescriptorTable.pDescriptorRanges = &descriptor_range;
            descriptor_ranges.push_back(descriptor_range);
        }

        root_parameters.push_back(root_parameter);
    }
}

DXGI_FORMAT ShaderReflectionTypeToFormat(D3D_REGISTER_COMPONENT_TYPE component_type, BYTE mask)
{
    if (component_type == D3D_REGISTER_COMPONENT_FLOAT32)
    {
        if (mask == 0x1) return DXGI_FORMAT_R32_FLOAT;
        if (mask == 0x3) return DXGI_FORMAT_R32G32_FLOAT;
        if (mask == 0x7) return DXGI_FORMAT_R32G32B32_FLOAT;
        if (mask == 0xF) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
    else if (component_type == D3D_REGISTER_COMPONENT_UINT32)
    {
        if (mask == 0x1) return DXGI_FORMAT_R32_UINT;
        if (mask == 0x3) return DXGI_FORMAT_R32G32_UINT;
        if (mask == 0x7) return DXGI_FORMAT_R32G32B32_UINT;
        if (mask == 0xF) return DXGI_FORMAT_R32G32B32A32_UINT;
    }
    else if (component_type == D3D_REGISTER_COMPONENT_SINT32)
    {
        if (mask == 0x1) return DXGI_FORMAT_R32_SINT;
        if (mask == 0x3) return DXGI_FORMAT_R32G32_SINT;
        if (mask == 0x7) return DXGI_FORMAT_R32G32B32_SINT;
        if (mask == 0xF) return DXGI_FORMAT_R32G32B32A32_SINT;
    }
    assert(!"ShaderReflectionTypeToFormat(...): unknown format");
    return DXGI_FORMAT_UNKNOWN;
}

void GetInputElementDescs(ID3D12ShaderReflection* reflection,
    std::vector<D3D12_INPUT_ELEMENT_DESC>& element_descs)
{
    // Get the shader's input parameter count
    D3D12_SHADER_DESC shader_desc;
    reflection->GetDesc(&shader_desc);

    for (std::uint32_t i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc;
        reflection->GetInputParameterDesc(i, &param_desc);

        // Skip system-generated semantics (e.g., SV_VertexID)
        if (param_desc.SystemValueType != D3D_NAME_UNDEFINED)
        {
            continue;
        }

        // Create an input element description
        D3D12_INPUT_ELEMENT_DESC input_element_desc = {};
        input_element_desc.SemanticName = param_desc.SemanticName;
        input_element_desc.SemanticIndex = param_desc.SemanticIndex;
        input_element_desc.Format = ShaderReflectionTypeToFormat(param_desc.ComponentType, param_desc.Mask);
        input_element_desc.InputSlot = 0;
        input_element_desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        input_element_desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element_desc.InstanceDataStepRate = 0;

        // Add the input element description to the vector
        element_descs.push_back(input_element_desc);
    }
}

std::size_t FindBinding(std::vector<D3D12Binding> const& bindings, std::uint32_t binding, std::uint32_t space)
{
    for (std::size_t i = 0; i < bindings.size(); ++i)
    {
        if (bindings[i].binding == binding && bindings[i].space == space)
        {
            return i;
        }
    }

    return bindings.size();
}
}

D3D12Pipeline::D3D12Pipeline(D3D12Device& device)
    : device_(device)
{
}

void D3D12Pipeline::BindBuffer(BufferPtr const& buffer, std::uint32_t binding, std::uint32_t space)
{
    // Find binding
    std::size_t binding_index = FindBinding(bindings_, binding, space);
    if (binding_index == bindings_.size())
    {
        static std::string error_message = "Binding (binding = " + std::to_string(binding) + ", space = "
            + std::to_string(space) + ") not found";
        throw std::runtime_error(error_message);
    }

    // Check if the binding is a buffer binding
    if (bindings_[binding_index].type != D3D12Binding::ResourceType::kBuffer)
    {
        static std::string error_message = "Binding (binding = " + std::to_string(binding) + ", space = "
            + std::to_string(space) + ") is not a buffer binding";
        throw std::runtime_error(error_message);
    }

    //bindings_[binding_index].buffer = buffer;
}

void D3D12Pipeline::BindImage(ImagePtr const& image, std::uint32_t binding, std::uint32_t space)
{
    // Find binding
    std::size_t binding_index = FindBinding(bindings_, binding, space);
    if (binding_index == bindings_.size())
    {
        static std::string error_message = "Binding (binding = " + std::to_string(binding) + ", space = "
            + std::to_string(space) + ") not found";
        throw std::runtime_error(error_message);
    }

    // Check if the binding is an image binding
    if (bindings_[binding_index].type != D3D12Binding::ResourceType::kImage)
    {
        static std::string error_message = "Binding (binding = " + std::to_string(binding) + ", space = "
            + std::to_string(space) + ") is not an image binding";
        throw std::runtime_error(error_message);
    }

    //bindings_[binding_index].image = image;
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

    std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges;
    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    D3D12Shader vs_shader = shader_manager.CompileShader(pipeline_desc_.vs_filename.c_str(), "main", "vs_6_0");
    D3D12_SHADER_BYTECODE vs_bytecode = {};
    vs_bytecode.BytecodeLength = vs_shader.dxc_blob->GetBufferSize();
    vs_bytecode.pShaderBytecode = vs_shader.dxc_blob->GetBufferPointer();
    CollectRootParameters(vs_shader.reflection.Get(), descriptor_ranges, root_parameters);

    D3D12Shader ps_shader = shader_manager.CompileShader(pipeline_desc_.ps_filename.c_str(), "main", "ps_6_0");
    D3D12_SHADER_BYTECODE ps_bytecode = {};
    ps_bytecode.BytecodeLength = ps_shader.dxc_blob->GetBufferSize();
    ps_bytecode.pShaderBytecode = ps_shader.dxc_blob->GetBufferPointer();
    CollectRootParameters(ps_shader.reflection.Get(), descriptor_ranges, root_parameters);

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = (UINT)root_parameters.size();
    root_signature_desc.pParameters = root_parameters.data();
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3D10Blob> root_signature_blob;
    {
        ComPtr<ID3D10Blob> root_signature_error_blob;
        HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
            &root_signature_blob, &root_signature_error_blob);
        if (FAILED(hr))
        {
            std::string error_blob_message = (char*)root_signature_error_blob->GetBufferPointer();
            error_blob_message.pop_back(); // remove \n

            static std::string error_message = "Failed to serialize graphics root signature "
                "(vs: " + pipeline_desc_.vs_filename + ", ps: " + pipeline_desc_.ps_filename + "): "
                + error_blob_message;
            throw D3D12Exception(error_message.c_str(), hr, __FILE__, __LINE__);
        }
    }

    ComPtr<ID3D12RootSignature> root_signature;
    ThrowIfFailed(d3d12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));

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

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_descs;
    GetInputElementDescs(vs_shader.reflection.Get(), input_element_descs);

    D3D12_INPUT_LAYOUT_DESC input_layout = {};
    input_layout.pInputElementDescs = input_element_descs.data();
    input_layout.NumElements = (std::uint32_t)input_element_descs.size();

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
    : ComputePipeline(cs_filename), D3D12Pipeline(device)
{
    Reload();
}

void D3D12ComputePipeline::Reload()
{
    auto d3d12_device = device_.GetD3D12Device();
    D3D12ShaderManager& shader_manager = device_.GetD3D12Api().GetShaderManager();

    D3D12Shader cs_shader = shader_manager.CompileShader(cs_filename_.c_str(), "main", "cs_6_0");
    D3D12_SHADER_BYTECODE cs_bytecode = {};
    cs_bytecode.BytecodeLength = cs_shader.dxc_blob->GetBufferSize();
    cs_bytecode.pShaderBytecode = cs_shader.dxc_blob->GetBufferPointer();

    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges;
    CollectRootParameters(cs_shader.reflection.Get(), descriptor_ranges, root_parameters);

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = (UINT)root_parameters.size();
    root_signature_desc.pParameters = root_parameters.data();

    ComPtr<ID3D10Blob> root_signature_blob;
    ComPtr<ID3D10Blob> root_signature_error_blob;
    ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
        &root_signature_blob, &root_signature_error_blob));

    ComPtr<ID3D12RootSignature> root_signature;
    ThrowIfFailed(d3d12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = root_signature_.Get();
    pipeline_state_desc.CS = cs_bytecode;

    ThrowIfFailed(d3d12_device->CreateComputePipelineState(&pipeline_state_desc,
        IID_PPV_ARGS(&pipeline_state_)));
}

} // namespace gpu
