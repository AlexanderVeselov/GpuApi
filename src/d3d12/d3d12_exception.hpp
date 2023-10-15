#ifndef D3D12_EXCEPTION_HPP_
#define D3D12_EXCEPTION_HPP_

#include <exception>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>

namespace gpu
{
    class D3D12Exception : public std::exception
    {
    public:
        D3D12Exception(char const* message, HRESULT hr, std::string file, std::uint32_t line)
            : std::exception(message), hr_(hr), file_(file), line_(line) {}

        char const* what() const override
        {
            _com_error err(hr_);
            std::string hr_string = err.ErrorMessage();
            hr_string.pop_back(); // Remove "." at the end of the string

            static std::string error_info = std::exception::what();
            error_info += " (HRESULT = " + std::to_string(hr_) + ", \"" + hr_string + "\"" +
                ", file: " + file_ + ", line: " + std::to_string(line_) + ")";
            return error_info.c_str();
        }

    private:
        HRESULT hr_;
        std::string file_;
        std::uint32_t line_;

    };

} // namespace gpu

#define ThrowIfFailed(x, msg) \
    { HRESULT hr = (x); if (FAILED(hr)) { throw D3D12Exception(msg, hr, __FILE__, __LINE__); } }

#define ThrowIfFailed(x) \
    { HRESULT hr = (x); if (FAILED(hr)) { static std::string msg = "Failed to execute " + std::string(#x); throw D3D12Exception(msg.c_str(), hr, __FILE__, __LINE__); } }

#endif // D3D12_API_HPP_
