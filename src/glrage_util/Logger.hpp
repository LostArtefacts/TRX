#pragma once

#include <intrin.h>
#include <string>
#include <windows.h>

#define LOG_INFO(...) Logger::printf(__VA_ARGS__)

class Logger {
public:
    static void printf(const char *format, ...);
    static void printf(const std::string &msg);
    static void tracef(
        void *returnAddress, const char *function, const char *format, ...);
    static void tracef(
        void *returnAddress, const char *function, const std::string &msg);
};
