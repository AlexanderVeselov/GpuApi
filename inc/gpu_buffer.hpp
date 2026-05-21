#pragma once

#include <cstddef>
#include <cstdint>

namespace gpu
{
/// Declares how a buffer may be used by the CPU and GPU.
enum class BufferFlags : uint32_t
{
    kNone = 0,
    /// Buffer can be mapped by the CPU.
    kCpuAccess = 1 << 0,
    /// Buffer can be bound as a constant/uniform buffer.
    kConstant = 1 << 1,
    /// Buffer can be read by shaders.
    kShaderResource = 1 << 2,
    /// Buffer can be read and written by shaders.
    kStorage = 1 << 3,
    /// Buffer can be used as geometry input when building an acceleration structure.
    kAccelerationStructureBuildInput = 1 << 4,
    /// Buffer can store an acceleration structure.
    kAccelerationStructureStorage = 1 << 5
};

inline BufferFlags operator|(BufferFlags lhs, BufferFlags rhs)
{
    return static_cast<BufferFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline BufferFlags operator&(BufferFlags lhs, BufferFlags rhs)
{
    return static_cast<BufferFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline BufferFlags& operator|=(BufferFlags& lhs, BufferFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline bool HasFlag(BufferFlags flags, BufferFlags flag)
{
    return (static_cast<uint32_t>(flags & flag) != 0);
}

/// Backend-independent buffer resource.
class Buffer
{
public:
    explicit Buffer(uint64_t size) : size_(size) {}

    virtual ~Buffer() = default;

    uint64_t GetSize() const { return size_; }

    /// Maps the buffer for CPU access. The buffer must have BufferFlags::kCpuAccess.
    virtual void* Map() = 0;

    /// Unmaps a previously mapped buffer.
    virtual void Unmap() = 0;

    /// Returns a backend GPU/device address for APIs that use explicit addresses.
    virtual uint64_t GetGpuAddress() const = 0;

protected:
    uint64_t size_ = 0;
};

}  // namespace gpu
