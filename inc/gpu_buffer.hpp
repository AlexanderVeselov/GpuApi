#pragma once

#include <cstddef>
#include <cstdint>

namespace gpu
{
enum class BufferFlags : uint32_t
{
    kNone = 0,
    kCpuAccess = 1 << 0,
    kConstant = 1 << 1,
    kShaderResource = 1 << 2,
    kStorage = 1 << 3
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

class Buffer
{
public:
    Buffer(uint64_t size)
        : size_(size)
    {}

    virtual ~Buffer() = default;

    uint64_t GetSize() const { return size_; }
    virtual void* Map() = 0;
    virtual void Unmap() = 0;

protected:
    uint64_t size_ = 0;
};

} // namespace gpu
