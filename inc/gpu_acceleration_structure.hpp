#pragma once

#include "gpu_types.hpp"

namespace gpu
{

enum class AccelerationStructureType
{
    kBottomLevel,
    kTopLevel
};

/// Backend-independent acceleration structure object. The actual AS is backed by a storage buffer, while backends keep
/// their native handles in derived classes.
class AccelerationStructure
{
public:
    virtual ~AccelerationStructure() = default;

    AccelerationStructureType GetType() const { return type_; }
    BufferPtr const& GetStorageBuffer() const { return storage_buffer_; }

protected:
    AccelerationStructure(AccelerationStructureType type, BufferPtr storage_buffer);

private:
    AccelerationStructureType type_;
    BufferPtr storage_buffer_;
};

}  // namespace gpu
