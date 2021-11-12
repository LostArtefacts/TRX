#include "Logger.hpp"

#include <cstdio>
#include <string>

static const size_t bufferSize = 1024;

void Logger::printf(const char* format, ...)
{
    char output[bufferSize + 2]; // reserve 2 chars for \r\n
    va_list list;
    va_start(list, format);
    vsnprintf_s(output, bufferSize, _TRUNCATE, &format[0], list);
    va_end(list);

    auto len = strnlen_s(output, bufferSize);
    auto tmp = &output[len];
    *tmp++ = '\r';
    *tmp++ = '\n';
    *tmp++ = 0;

    OutputDebugStringA(output);
}

void Logger::printf(const std::string& msg)
{
    printf(msg.c_str());
}

void Logger::tracef(void* returnAddress, const char* function, const char* format, ...)
{
    if (strlen(format) == 0) {
        printf("%p %s", returnAddress, function);
        return;
    }

    char output[bufferSize];
    va_list list;
    va_start(list, format);
    vsnprintf_s(output, sizeof(output), _TRUNCATE, format, list);
    va_end(list);

    printf("%p %s: %s", returnAddress, function, output);
}

void Logger::tracef(void* returnAddress, const char* function, const std::string& msg)
{
    tracef(returnAddress, function, msg.c_str());
}