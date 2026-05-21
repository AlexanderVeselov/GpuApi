#include "gpu_acceleration_structure.hpp"

#include <utility>

namespace gpu
{

AccelerationStructure::AccelerationStructure(AccelerationStructureType type, BufferPtr storage_buffer)
    : type_(type), storage_buffer_(std::move(storage_buffer))
{
}

}  // namespace gpu
