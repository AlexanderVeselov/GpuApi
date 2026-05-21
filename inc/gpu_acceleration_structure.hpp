#pragma once

#include "gpu_types.hpp"

#include <vector>

namespace gpu
{

class AccelerationStructure;

enum class AccelerationStructureType
{
    kBottomLevel,
    kTopLevel
};

struct AccelerationStructureGeometryDesc
{
    BufferPtr vertex_buffer;
    uint64_t vertex_offset = 0;
    uint32_t vertex_stride = 0;
    uint32_t vertex_count = 0;
    ImageFormat vertex_format = ImageFormat::kRGB32_Float;

    BufferPtr index_buffer;
    uint64_t index_offset = 0;
    uint32_t index_count = 0;
    bool opaque = true;
};

struct AccelerationStructureInstanceDesc
{
    AccelerationStructure* bottom_level = nullptr;
    float transform[3][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
    };
    uint32_t instance_id = 0;
    uint32_t instance_mask = 0xff;
};

/// Backend-independent acceleration structure object. The actual AS is backed by a storage buffer, while backends keep
/// their native handles in derived classes.
class AccelerationStructure
{
public:
    virtual ~AccelerationStructure() = default;

    AccelerationStructureType GetType() const { return type_; }
    BufferPtr const& GetStorageBuffer() const { return storage_buffer_; }
    uint64_t GetBuildScratchSize() const { return build_scratch_size_; }

protected:
    AccelerationStructure(AccelerationStructureType type, BufferPtr storage_buffer, uint64_t build_scratch_size);

private:
    AccelerationStructureType type_;
    BufferPtr storage_buffer_;
    uint64_t build_scratch_size_ = 0;
};

}  // namespace gpu
