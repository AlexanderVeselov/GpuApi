#pragma once

namespace gpu
{
class Semaphore
{
public:

};

class Fence
{
public:
    virtual void Wait() = 0;

};

} // namespace gpu
