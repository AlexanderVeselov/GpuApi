#pragma once

#include <cstddef>

namespace gpu
{
class Buffer
{
public:
    Buffer(std::size_t size) : size_(size) {}
    std::size_t GetSize() const { return size_; }
    virtual void* Map() = 0;
    virtual void Unmap() = 0;

protected:
    std::size_t size_;

};
} // namespace gpu
