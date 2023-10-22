#ifndef D3D12_COMMON_HPP_
#define D3D12_COMMON_HPP_

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <cstdint>

#undef CreateSemaphore
using Microsoft::WRL::ComPtr;

constexpr std::uint32_t kNullDescriptor = ~0u;

#endif // D3D12_COMMON_HPP_
