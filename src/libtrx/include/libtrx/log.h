#pragma once

#define LOG_INFO(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_VAR(var)                                                           \
    _Generic(                                                                  \
        (var),                                                                 \
        int: LOG_DEBUG(#var ": %d", var),                                      \
        int8_t: LOG_DEBUG(#var ": %d", var),                                   \
        int16_t: LOG_DEBUG(#var ": %d", var),                                  \
        uint8_t: LOG_DEBUG(#var ": %d", var),                                  \
        uint16_t: LOG_DEBUG(#var ": %d", var),                                 \
        uint32_t: LOG_DEBUG(#var ": %d", var),                                 \
        float: LOG_DEBUG(#var ": %f", var),                                    \
        double: LOG_DEBUG(#var ": %f", var),                                   \
        char *: LOG_DEBUG(#var ": %s", var),                                   \
        default: LOG_DEBUG(#var ": %p", var))

void Log_Init(const char *path);
void Log_Shutdown(void);
void Log_Message(
    const char *file, int line, const char *func, const char *fmt, ...);

// platform-specific implementations
void Log_Init_Extra(const char *path);
void Log_Shutdown_Extra(void);
