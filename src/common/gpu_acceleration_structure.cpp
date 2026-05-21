#include "gpu_acceleration_structure.hpp"

#include <utility>

namespace gpu
{

AccelerationStructure::AccelerationStructure(AccelerationStructureType type, BufferPtr storage_buffer,
    uint64_t build_scratch_size)
    : type_(type), storage_buffer_(std::move(storage_buffer)), build_scratch_size_(build_scratch_size)
{
}

}  // namespace gpu
