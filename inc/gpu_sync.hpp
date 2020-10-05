#ifndef GPU_SYNC_HPP_
#define GPU_SYNC_HPP_

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

#endif // GPU_SYNC_HPP_
