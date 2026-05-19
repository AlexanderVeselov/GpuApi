#pragma once

#include "gpu_types.hpp"

#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#undef CreateSemaphore
using Microsoft::WRL::ComPtr;

namespace gpu
{
constexpr uint32_t kNullDescriptor = ~0u;

inline DXGI_FORMAT ImageToDXGIFormat(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::kUnknown:
        return DXGI_FORMAT_UNKNOWN;
    case ImageFormat::kRGBA32_Float:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case ImageFormat::kRGBA32_UInt:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case ImageFormat::kRGBA32_SInt:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case ImageFormat::kRGBA16_Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case ImageFormat::kRGBA8_SInt:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case ImageFormat::kRGBA8_UInt:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case ImageFormat::kRGBA8_UNorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat::kRGBA8_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case ImageFormat::kBGRA8_UNorm:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case ImageFormat::kBGRA8_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case ImageFormat::kRGB32_Float:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case ImageFormat::kRGB32_UInt:
        return DXGI_FORMAT_R32G32B32_UINT;
    case ImageFormat::kRGB32_SInt:
        return DXGI_FORMAT_R32G32B32_SINT;
    case ImageFormat::kRG32_Float:
        return DXGI_FORMAT_R32G32_FLOAT;
    case ImageFormat::kRG32_UInt:
        return DXGI_FORMAT_R32G32_UINT;
    case ImageFormat::kRG32_SInt:
        return DXGI_FORMAT_R32G32_SINT;
    case ImageFormat::kRG16_Float:
        return DXGI_FORMAT_R16G16_FLOAT;
    case ImageFormat::kR32_Float:
        return DXGI_FORMAT_R32_FLOAT;
    case ImageFormat::kR32_UInt:
        return DXGI_FORMAT_R32_UINT;
    case ImageFormat::kR32_SInt:
        return DXGI_FORMAT_R32_SINT;
    case ImageFormat::kD32_Float:
        return DXGI_FORMAT_D32_FLOAT;
    case ImageFormat::kR32_Typeless:
        return DXGI_FORMAT_R32_TYPELESS;
    case ImageFormat::kR16_Float:
        return DXGI_FORMAT_R16_FLOAT;
    default:
        assert(!"ImageToDXGIFormat: Unknown image format");
        return DXGI_FORMAT_UNKNOWN;
    }
}

}  // namespace gpu
