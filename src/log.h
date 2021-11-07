#ifndef T1M_LOG_H
#define T1M_LOG_H

#define LOG_INFO(...) T1MLogFunc(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) T1MLogFunc(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) T1MLogFunc(__FILE__, __LINE__, __func__, __VA_ARGS__)

void T1MLogFunc(
    const char *file, int line, const char *func, const char *fmt, ...);
void T1MPrintStackTrace();

#endif
