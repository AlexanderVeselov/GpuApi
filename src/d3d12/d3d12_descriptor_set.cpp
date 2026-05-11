#include "d3d12_descriptor_set.hpp"
#include "d3d12_buffer.hpp"
#include "d3d12_image.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>

namespace gpu
{
namespace
{
std::string BindingName(uint32_t binding, uint32_t space)
{
    return "(binding = " + std::to_string(binding) + ", space = " + std::to_string(space) + ")";
}
} // namespace

template <class TResource, class TBase>
TResource& CastResource(TBase& resource, char const* function_name)
{
    TResource* d3d12_resource = dynamic_cast<TResource*>(&resource);
    if (!d3d12_resource)
    {
        throw std::runtime_error(
            std::string(function_name) + ": resource was not created by the D3D12 backend");
    }

    return *d3d12_resource;
}

D3D12DescriptorSet::D3D12DescriptorSet(D3D12PipelineLayout const& layout) : layout_(layout)
{
}

D3D12DescriptorSet::~D3D12DescriptorSet()
{
}

D3D12DescriptorSet::D3D12DescriptorSet(D3D12DescriptorSet&& other) noexcept
    : layout_(other.layout_), descriptors_(std::move(other.descriptors_))
{
}

D3D12DescriptorSet& D3D12DescriptorSet::operator=(D3D12DescriptorSet&& other) noexcept
{
    assert(&layout_ == &other.layout_ &&
           "D3D12DescriptorSet::operator=: descriptor sets must use the same pipeline layout");

    if (this != &other)
    {
        Clear();
        descriptors_ = std::move(other.descriptors_);
    }

    return *this;
}

void D3D12DescriptorSet::BindBuffer(Buffer& buffer, uint32_t binding, uint32_t space)
{
    BindBuffer(CastResource<D3D12Buffer>(buffer, "D3D12DescriptorSet::BindBuffer"), binding, space);
}

void D3D12DescriptorSet::BindImage(Image& image, uint32_t binding, uint32_t space)
{
    BindImage(CastResource<D3D12Image>(image, "D3D12DescriptorSet::BindImage"), binding, space);
}

void D3D12DescriptorSet::BindImage(
    Image& image, ImageView const& view, uint32_t binding, uint32_t space)
{
    BindImage(
        CastResource<D3D12Image>(image, "D3D12DescriptorSet::BindImage"), view, binding, space);
}

void D3D12DescriptorSet::BindBuffer(D3D12Buffer& buffer, uint32_t binding, uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kBuffer)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindBuffer: pipeline binding " +
                                 BindingName(binding, space) + " is not a buffer binding");
    }

    if (d3d12_binding.range_type == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
    {
        BindDescriptor(d3d12_binding, buffer.GetSRV());
    }
    else if (d3d12_binding.range_type == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
    {
        BindDescriptor(d3d12_binding, buffer.GetUAV());
    }
    else
    {
        BindDescriptor(d3d12_binding, buffer.GetCBV());
    }
}

void D3D12DescriptorSet::BindImage(D3D12Image& image, uint32_t binding, uint32_t space)
{
    BindImage(image, ImageView{}, binding, space);
}

void D3D12DescriptorSet::BindImage(
    D3D12Image& image, ImageView const& view, uint32_t binding, uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kImage)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindImage: pipeline binding " +
                                 BindingName(binding, space) + " is not an image binding");
    }

    if (d3d12_binding.range_type == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
    {
        BindDescriptor(d3d12_binding, image.GetUAV(view));
    }
    else
    {
        BindDescriptor(d3d12_binding, image.GetView(view));
    }
}

void D3D12DescriptorSet::Clear()
{
    descriptors_.clear();
}

D3D12Binding const& D3D12DescriptorSet::FindBinding(uint32_t binding, uint32_t space) const
{
    for (D3D12Binding const& d3d12_binding : layout_.GetBindings())
    {
        if (d3d12_binding.binding == binding && d3d12_binding.space == space)
        {
            return d3d12_binding;
        }
    }

    throw std::runtime_error("D3D12DescriptorSet: pipeline layout binding " +
                             BindingName(binding, space) + " was not found");
}

D3D12DescriptorSet::BoundDescriptor& D3D12DescriptorSet::FindOrCreateBoundDescriptor(
    D3D12Binding const& binding)
{
    for (BoundDescriptor& descriptor : descriptors_)
    {
        if (descriptor.binding == binding.binding && descriptor.space == binding.space)
        {
            return descriptor;
        }
    }

    BoundDescriptor descriptor;
    descriptor.binding = binding.binding;
    descriptor.space = binding.space;
    descriptor.root_parameter_index = binding.root_parameter_index;
    descriptors_.push_back(descriptor);
    return descriptors_.back();
}

void D3D12DescriptorSet::BindDescriptor(D3D12Binding const& binding, D3D12Descriptor cpu_descriptor)
{
    assert(cpu_descriptor.IsValid() &&
           "D3D12DescriptorSet::BindDescriptor: CPU descriptor is invalid");
    assert(binding.descriptor_type == D3D12Binding::DescriptorType::kDescriptorTable &&
           "D3D12DescriptorSet::BindDescriptor: only descriptor-table bindings are supported");

    BoundDescriptor& descriptor = FindOrCreateBoundDescriptor(binding);

    descriptor.cpu_descriptor = cpu_descriptor;
}

} // namespace gpu
