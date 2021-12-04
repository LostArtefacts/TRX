#ifndef T1M_LOG_H
#define T1M_LOG_H

#define LOG_INFO(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) Log_Message(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

void Log_Init();
void Log_Message(
    const char *file, int line, const char *func, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
