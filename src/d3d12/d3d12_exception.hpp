#ifndef D3D12_EXCEPTION_HPP_
#define D3D12_EXCEPTION_HPP_

#include <exception>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace gpu
{
    class D3D12Exception : public std::exception
    {
    public:
        D3D12Exception(char const* const message, HRESULT hr, std::string file, std::uint32_t line)
            : std::exception(message), hr_(hr), file_(file), line_(line) {}

        char const* what() const override
        {
            static std::string error_info = std::exception::what();
            error_info += " (HRESULT = " + std::to_string(hr_) +
                ", file: " + file_ + ", line: " + std::to_string(line_) + ")";
            return error_info.c_str();
        }

    private:
        HRESULT hr_;
        std::string file_;
        std::uint32_t line_;

    };

} // namespace gpu

#define ThrowIfFailed(x) \
    { HRESULT hr = (x); if (FAILED(hr)) { throw D3D12Exception("D3D12 Error", hr, __FILE__, __LINE__); } }


#endif // D3D12_API_HPP_
