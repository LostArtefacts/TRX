#include "specific/s_log.h"

#include "log.h"

#include "shared/memory.h"

#include <process.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>

static char *m_MiniDumpPath = NULL;
static char *S_Log_GetMiniDumpPath(const char *log_path);
static void S_Log_CreateMiniDump(EXCEPTION_POINTERS *ex, const char *path);
static void S_Log_LogStackTraces(EXCEPTION_POINTERS *ex);

static char *S_Log_GetMiniDumpPath(const char *const log_path)
{
    char *dot = strrchr(log_path, '.');
    if (dot == NULL) {
        return NULL;
    }

    const size_t index = dot - log_path;
    const char *new_extension = "dmp";
    const size_t new_len = strlen(log_path) - index + strlen(new_extension) + 1;
    char *minidump_path = Memory_Alloc(new_len);
    strncpy(minidump_path, log_path, index);
    strcat(minidump_path, new_extension);
    return minidump_path;
}

static void S_Log_CreateMiniDump(
    EXCEPTION_POINTERS *const ex, const char *const path)
{
    HANDLE handle = CreateFile(
        path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        NULL);
    MINIDUMP_EXCEPTION_INFORMATION dump_info;
    dump_info.ExceptionPointers = ex;
    dump_info.ThreadId = GetCurrentThreadId();
    dump_info.ClientPointers = TRUE;
    MiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(), handle, MiniDumpNormal,
        &dump_info, NULL, NULL);
    CloseHandle(handle);

    LOG_INFO("Crash dump info put in %s", path);
}

static void S_Log_LogStackTraces(EXCEPTION_POINTERS *const ex)
{
    LOG_ERROR("== CRASH REPORT ==");

    HANDLE thread = GetCurrentThread();
    HANDLE process = GetCurrentProcess();

    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;

    if (thread != GetCurrentThread()) {
        SuspendThread(thread);
        if (GetThreadContext(thread, &context) == FALSE) {
            printf("Failed to get context\n");
            return;
        }
        ResumeThread(thread);
    }
    RtlCaptureContext(&context);

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    if (!SymInitialize(process, 0, TRUE)) {
        LOG_ERROR("Failed to call SymInitialize");
        return;
    }

    DWORD image;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    stackframe.AddrPC.Offset = context.Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#endif

    while (StackWalk64(
        image, process, thread, &stackframe, &context, 0,
        SymFunctionTableAccess64, SymGetModuleBase64, 0)) {
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;

        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement64 = 0;
        if (SymFromAddr(
                process, stackframe.AddrPC.Offset, &displacement64, symbol)) {
            IMAGEHLP_LINE64 line;
            DWORD displacement32;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            if (SymGetLineFromAddr64(
                    process, stackframe.AddrPC.Offset, &displacement32,
                    &line)) {
                LOG_INFO(
                    "0x%08llX: %s (%s:%d)", symbol->Address, symbol->Name,
                    line.FileName, line.LineNumber);
            } else {
                LOG_INFO("0x%08llX: %s", symbol->Address, symbol->Name);
            }
        } else {
            LOG_INFO("0x%08llX: ???", symbol->Address);
        }
    }

    SymCleanup(process);
}

LONG WINAPI S_Log_CrashHandler(EXCEPTION_POINTERS *ex)
{
    S_Log_CreateMiniDump(ex, m_MiniDumpPath);
    S_Log_LogStackTraces(ex);
    return EXCEPTION_EXECUTE_HANDLER;
}

void S_Log_Init(const char *log_path)
{
    if (log_path != NULL) {
        m_MiniDumpPath = S_Log_GetMiniDumpPath(log_path);
        SetUnhandledExceptionFilter(S_Log_CrashHandler);
    }
}

void S_Log_Shutdown(void)
{
    Memory_FreePointer(&m_MiniDumpPath);
}
