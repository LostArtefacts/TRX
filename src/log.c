#include "log.h"

#include "filesystem.h"
#include "memory.h"

#include <stdarg.h>
#include <stdio.h>

FILE *m_LogHandle = NULL;

void Log_Init()
{
    char *full_path = File_GetFullPath("Tomb1Main.log");
    m_LogHandle = fopen(full_path, "w");
    Memory_FreePointer(&full_path);
}

void Log_Message(
    const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    // print to stdout
    printf("%s %d %s ", file, line, func);
    vprintf(fmt, va);
    printf("\n");
    fflush(stdout);

    if (m_LogHandle) {
        // now print the same to the log file
        fprintf(m_LogHandle, "%s %d %s ", file, line, func);
        vfprintf(m_LogHandle, fmt, va);
        fprintf(m_LogHandle, "\n");
        fflush(m_LogHandle);
    }

    va_end(va);
}

void Log_Shutdown()
{
    if (m_LogHandle) {
        fclose(m_LogHandle);
    }
}
