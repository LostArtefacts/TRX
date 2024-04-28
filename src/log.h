#pragma once

#define LOG_INFO(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)

void Log_Init(const char *path);
void Log_Shutdown(void);
void Log_Message(
    const char *file, int line, const char *func, const char *fmt, ...);
