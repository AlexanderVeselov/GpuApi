#include "gpu_pipeline.hpp"

#include <cassert>
#include <stdexcept>

namespace gpu
{
namespace
{
std::size_t FindBinding(std::vector<Binding> const& bindings, std::uint32_t binding, std::uint32_t space)
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
} // namespace

void Pipeline::BindBuffer(BufferPtr const& buffer, std::uint32_t binding, std::uint32_t space)
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
    if (bindings_[binding_index].type != BindingType::kBuffer)
    {
        static std::string error_message = "Binding (binding = " + std::to_string(binding) + ", space = "
            + std::to_string(space) + ") is not a buffer binding";
        throw std::runtime_error(error_message);
    }

    bindings_[binding_index].buffer = buffer;
}

void Pipeline::BindImage(ImagePtr const& image, std::uint32_t binding, std::uint32_t space)
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
    if (bindings_[binding_index].type != BindingType::kBuffer)
    {
        static std::string error_message = "Binding (binding = " + std::to_string(binding) + ", space = "
            + std::to_string(space) + ") is not an image binding";
        throw std::runtime_error(error_message);
    }

    bindings_[binding_index].image = image;
}

} // namespace gpu
