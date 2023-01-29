#pragma once

#include <string>
#include <codecvt>

namespace gpu
{
inline std::wstring StringToWstring(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

} // namespace gpu
