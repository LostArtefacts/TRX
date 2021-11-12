#pragma once

#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>

namespace glrage {

class StringUtils
{
public:
    static void format(std::string& str, std::string fmt, ...);
    static std::string format(std::string fmt, ...);
    static std::string bytesToHex(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> hexToBytes(const std::string& str);
    static std::wstring utf8ToWide(const std::string& str);
    static std::string wideToUtf8(const std::wstring& str);

private:
    static void formatResize(std::string& str, std::string& fmt, va_list& va);
    static void formatImpl(std::string& str, std::string& fmt, va_list& va);
};

} // namespace glrage