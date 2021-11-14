#include "StringUtils.hpp"

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <codecvt>

namespace glrage {

void StringUtils::format(std::string& str, std::string fmt, ...)
{
    va_list vl;

    va_start(vl, fmt);
    formatResize(str, fmt, vl);
    va_end(vl);

    va_start(vl, fmt);
    formatImpl(str, fmt, vl);
    va_end(vl);
}

std::string StringUtils::format(std::string fmt, ...)
{
    std::string result;
    va_list vl;

    va_start(vl, fmt);
    formatResize(result, fmt, vl);
    va_end(vl);

    va_start(vl, fmt);
    formatImpl(result, fmt, vl);
    va_end(vl);

    return result;
}

void StringUtils::formatResize(std::string& str, std::string& fmt, va_list& vl)
{
    str.resize((std::min)(_vscprintf(fmt.c_str(), vl), 0x4000));
}

void StringUtils::formatImpl(std::string& str, std::string& fmt, va_list& vl)
{
    vsnprintf_s(&str[0], str.capacity(), str.capacity(), fmt.c_str(), vl);
}

std::string StringUtils::bytesToHex(const std::vector<uint8_t>& data)
{
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    for (uint32_t byte : data) {
        ss << std::setw(2) << byte << ' ';
    }

    // remove last space
    std::string str = ss.str();
    if (!str.empty()) {
        str.pop_back();
    }

    return str;
}

std::vector<uint8_t> StringUtils::hexToBytes(const std::string& str)
{
    std::vector<uint8_t> data;
    std::istringstream ss(str);
    ss >> std::hex >> std::setw(2);

    for (uint32_t byte; ss >> byte;) {
        data.push_back(byte);
    }

    return data;
}

} // namespace glrage
