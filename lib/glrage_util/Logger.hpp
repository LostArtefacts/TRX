#pragma once

#include <windows.h>
#include <intrin.h>

#include <string>

#define LOG_INFO(...) Logger::printf(__VA_ARGS__)

#ifdef LOG_TRACE_ENABLED
#define LOG_TRACE(...) Logger::tracef(_ReturnAddress(), __FUNCTION__, __VA_ARGS__)
#else
#define LOG_TRACE(...)
#endif

class Logger
{
public:
    static void printf(const char* format, ...);
    static void printf(const std::string& msg);
    static void tracef(void* returnAddress, const char* function, const char* format, ...);
    static void tracef(void* returnAddress, const char* function, const std::string& msg);
};
