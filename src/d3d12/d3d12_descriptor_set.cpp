#include "d3d12_descriptor_set.hpp"
#include "d3d12_acceleration_structure.hpp"
#include "d3d12_buffer.hpp"
#include "d3d12_device.hpp"
#include "d3d12_image.hpp"
#include "d3d12_sampler.hpp"

#include <cassert>
#include <algorithm>
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
}  // namespace

template <class TResource, class TBase>
TResource& CastResource(TBase& resource, char const* function_name)
{
    TResource* d3d12_resource = dynamic_cast<TResource*>(&resource);
    if (!d3d12_resource)
    {
        throw std::runtime_error(std::string(function_name) + ": resource was not created by the D3D12 backend");
    }

    return *d3d12_resource;
}

D3D12DescriptorSet::D3D12DescriptorSet(D3D12Device& device, D3D12PipelineLayout const& layout)
    : descriptor_manager_(device.GetDescriptorManager()), layout_(layout)
{
}

D3D12DescriptorSet::~D3D12DescriptorSet()
{
    Clear();
}

void D3D12DescriptorSet::BindBuffer(Buffer& buffer, uint32_t binding, uint32_t space)
{
    BindBuffer(CastResource<D3D12Buffer>(buffer, "D3D12DescriptorSet::BindBuffer"), binding, space);
}

void D3D12DescriptorSet::BindImage(Image& image, uint32_t binding, uint32_t space)
{
    BindImage(CastResource<D3D12Image>(image, "D3D12DescriptorSet::BindImage"), binding, space);
}

void D3D12DescriptorSet::BindImage(Image& image, ImageView const& view, uint32_t binding, uint32_t space)
{
    BindImage(CastResource<D3D12Image>(image, "D3D12DescriptorSet::BindImage"), view, binding, space);
}

void D3D12DescriptorSet::BindSampler(Sampler& sampler, uint32_t binding, uint32_t space)
{
    BindSampler(CastResource<D3D12Sampler>(sampler, "D3D12DescriptorSet::BindSampler"), binding, space);
}

void D3D12DescriptorSet::BindAccelerationStructure(AccelerationStructure& acceleration_structure, uint32_t binding,
    uint32_t space)
{
    BindAccelerationStructure(CastResource<D3D12AccelerationStructure>(acceleration_structure,
                                  "D3D12DescriptorSet::BindAccelerationStructure"),
        binding, space);
}

void D3D12DescriptorSet::BindBuffer(D3D12Buffer& buffer, uint32_t binding, uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kBuffer)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindBuffer: pipeline binding " + BindingName(binding, space)
            + " is not a buffer binding");
    }

    BoundDescriptor& descriptor = FindOrCreateBoundDescriptor(d3d12_binding);
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

    if (descriptor.buffers.size() == 1 && descriptor.buffers.front() == &buffer)
    {
        return;
    }

    UnregisterBuffers(descriptor);
    descriptor.buffers = {&buffer};
    buffer.RegisterDescriptorSet(*this);
}

void D3D12DescriptorSet::OnBufferResized(D3D12Buffer& buffer)
{
    for (BoundDescriptor const& descriptor : descriptors_)
    {
        for (D3D12Buffer* bound_buffer : descriptor.buffers)
        {
            if (bound_buffer == &buffer)
            {
                RefreshDescriptors(descriptor);
                break;
            }
        }
    }
}

void D3D12DescriptorSet::OnBufferDestroyed(D3D12Buffer& buffer)
{
    for (BoundDescriptor& descriptor : descriptors_)
    {
        descriptor.buffers.erase(std::remove(descriptor.buffers.begin(), descriptor.buffers.end(), &buffer), descriptor.buffers.end());
    }
}

void D3D12DescriptorSet::BindImage(D3D12Image& image, uint32_t binding, uint32_t space)
{
    BindImage(image, ImageView{}, binding, space);
}

void D3D12DescriptorSet::BindImage(D3D12Image& image, ImageView const& view, uint32_t binding, uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kImage)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindImage: pipeline binding " + BindingName(binding, space)
            + " is not an image binding");
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

void D3D12DescriptorSet::BindImageArray(std::vector<ImageDescriptor> const& images, uint32_t binding, uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kImage)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindImageArray: pipeline binding " + BindingName(binding, space)
            + " is not an image binding");
    }

    std::vector<D3D12Descriptor> descriptors;
    descriptors.reserve(images.size());
    for (ImageDescriptor const& image_descriptor : images)
    {
        if (!image_descriptor.image)
        {
            throw std::runtime_error("D3D12DescriptorSet::BindImageArray: image descriptor is null");
        }

        D3D12Image& image = CastResource<D3D12Image>(*image_descriptor.image, "D3D12DescriptorSet::BindImageArray");
        if (d3d12_binding.range_type == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
        {
            descriptors.push_back(image.GetUAV(image_descriptor.view));
        }
        else
        {
            descriptors.push_back(image.GetView(image_descriptor.view));
        }
    }

    BindDescriptors(d3d12_binding, std::move(descriptors));
}

void D3D12DescriptorSet::BindSampler(D3D12Sampler& sampler, uint32_t binding, uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kSampler)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindSampler: pipeline binding " + BindingName(binding, space)
            + " is not a sampler binding");
    }

    BindDescriptor(d3d12_binding, sampler.GetDescriptor());
}

void D3D12DescriptorSet::BindAccelerationStructure(D3D12AccelerationStructure& acceleration_structure, uint32_t binding,
    uint32_t space)
{
    D3D12Binding const& d3d12_binding = FindBinding(binding, space);
    if (d3d12_binding.type != D3D12Binding::ResourceType::kAccelerationStructure)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindAccelerationStructure: pipeline binding "
            + BindingName(binding, space) + " is not an acceleration-structure binding");
    }

    if (d3d12_binding.range_type != D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
    {
        throw std::runtime_error("D3D12DescriptorSet::BindAccelerationStructure: pipeline binding "
            + BindingName(binding, space) + " is not an SRV binding");
    }

    BindDescriptor(d3d12_binding, acceleration_structure.GetSRV());
}

void D3D12DescriptorSet::Clear()
{
    for (BoundDescriptor& descriptor : descriptors_)
    {
        UnregisterBuffers(descriptor);
        FreeGpuDescriptors(descriptor);
    }
    descriptors_.clear();
}

void D3D12DescriptorSet::UnregisterBuffers(BoundDescriptor& descriptor)
{
    for (D3D12Buffer* buffer : descriptor.buffers)
    {
        if (buffer)
        {
            bool still_bound = false;
            for (BoundDescriptor const& other_descriptor : descriptors_)
            {
                if (&other_descriptor == &descriptor)
                {
                    continue;
                }

                for (D3D12Buffer* other_buffer : other_descriptor.buffers)
                {
                    if (other_buffer == buffer)
                    {
                        still_bound = true;
                        break;
                    }
                }

                if (still_bound)
                {
                    break;
                }
            }

            if (!still_bound)
            {
                buffer->UnregisterDescriptorSet(*this);
            }
        }
    }
    descriptor.buffers.clear();
}

void D3D12DescriptorSet::FreeGpuDescriptors(BoundDescriptor& descriptor)
{
    for (D3D12Descriptor gpu_descriptor : descriptor.gpu_descriptors)
    {
        descriptor_manager_.Free(gpu_descriptor);
    }
    descriptor.gpu_descriptors.clear();
}

D3D12Binding const& D3D12DescriptorSet::FindBinding(uint32_t binding, uint32_t space) const
{
    for (D3D12Binding const& d3d12_binding : layout_.GetBindings())
    {
        if (d3d12_binding.binding == binding && d3d12_binding.space == space
            && d3d12_binding.descriptor_type == D3D12Binding::DescriptorType::kDescriptorTable)
        {
            return d3d12_binding;
        }
    }

    throw std::runtime_error("D3D12DescriptorSet: pipeline layout binding " + BindingName(binding, space)
        + " was not found");
}

D3D12DescriptorSet::BoundDescriptor& D3D12DescriptorSet::FindOrCreateBoundDescriptor(D3D12Binding const& binding)
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
    BindDescriptors(binding, {cpu_descriptor});
}

void D3D12DescriptorSet::BindDescriptors(D3D12Binding const& binding, std::vector<D3D12Descriptor> cpu_descriptors)
{
    if (cpu_descriptors.empty())
    {
        throw std::runtime_error("D3D12DescriptorSet::BindDescriptors: descriptor array is empty");
    }

    for (D3D12Descriptor descriptor : cpu_descriptors)
    {
        assert(descriptor.IsValid() && "D3D12DescriptorSet::BindDescriptors: CPU descriptor is invalid");
    }

    if (cpu_descriptors.size() != binding.descriptor_count)
    {
        throw std::runtime_error(
            "D3D12DescriptorSet::BindDescriptors: descriptor count does not match pipeline "
            "layout binding count");
    }

    assert(binding.descriptor_type == D3D12Binding::DescriptorType::kDescriptorTable
        && "D3D12DescriptorSet::BindDescriptors: only descriptor-table bindings are supported");

    std::vector<D3D12Descriptor> gpu_descriptors;
    switch (cpu_descriptors.front().heap)
    {
    case D3D12DescriptorHeapId::CPU_CBV_SRV_UAV:
        gpu_descriptors = descriptor_manager_.CopyToGPUCBVSRVUAV(cpu_descriptors);
        break;
    case D3D12DescriptorHeapId::CPU_SAMPLER:
        gpu_descriptors = descriptor_manager_.CopyToGPUSampler(cpu_descriptors);
        break;
    default:
        assert(false && "D3D12DescriptorSet::BindDescriptors: source descriptor heap is not shader-visible-copyable");
        break;
    }

    BoundDescriptor& descriptor = FindOrCreateBoundDescriptor(binding);
    FreeGpuDescriptors(descriptor);
    descriptor.cpu_descriptors = std::move(cpu_descriptors);
    descriptor.gpu_descriptors = std::move(gpu_descriptors);
}

void D3D12DescriptorSet::RefreshDescriptors(BoundDescriptor const& descriptor)
{
    if (descriptor.cpu_descriptors.empty() || descriptor.gpu_descriptors.empty())
    {
        return;
    }

    switch (descriptor.cpu_descriptors.front().heap)
    {
    case D3D12DescriptorHeapId::CPU_CBV_SRV_UAV:
        descriptor_manager_.CopyToExistingGPUCBVSRVUAV(descriptor.gpu_descriptors, descriptor.cpu_descriptors);
        break;
    case D3D12DescriptorHeapId::CPU_SAMPLER:
        descriptor_manager_.CopyToExistingGPUSampler(descriptor.gpu_descriptors, descriptor.cpu_descriptors);
        break;
    default:
        assert(false && "D3D12DescriptorSet::RefreshDescriptors: unsupported descriptor heap");
        break;
    }
}

}  // namespace gpu
