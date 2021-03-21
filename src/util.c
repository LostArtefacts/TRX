#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#include <dbghelp.h>

void T1MTraceFunc(
    const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    printf("%s %d %s ", file, line, func);
    vprintf(fmt, va);
    printf("\n");
    fflush(stdout);
}

void T1MInjectFunc(void (*from)(void), void (*to)(void))
{
    if (from == to) {
        return;
    }
    DWORD tmp;
    TRACE("Patching %p to %p", from, to);
    VirtualProtect(from, sizeof(JMP), PAGE_EXECUTE_READWRITE, &tmp);
    HANDLE hCurrentProcess = GetCurrentProcess();
    JMP buf;
    buf.opcode = 0xE9;
    buf.offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
    WriteProcessMemory(hCurrentProcess, from, &buf, sizeof(JMP), &tmp);
    CloseHandle(hCurrentProcess);
    // arsunt style - doesn't work because of GLRage calling VirtualProtect
    //((JMP*)(from))->opcode = 0xE9;
    //((JMP*)(from))->offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
}

void T1MPrintStackTrace()
{
    const size_t MaxNameLen = 255;
    BOOL result;
    HANDLE process;
    HANDLE thread;
    CONTEXT context;
    STACKFRAME64 stack;
    ULONG frame;
    DWORD64 displacement;
    IMAGEHLP_SYMBOL64 *pSymbol =
        malloc(sizeof(IMAGEHLP_SYMBOL64) + (MaxNameLen + 1) * sizeof(TCHAR));
    char *name = malloc(MaxNameLen + 1);

    RtlCaptureContext(&context);
    memset(&stack, 0, sizeof(STACKFRAME64));

    process = GetCurrentProcess();
    thread = GetCurrentThread();
    displacement = 0;
    stack.AddrPC.Offset = context.Eip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = context.Esp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = context.Ebp;
    stack.AddrFrame.Mode = AddrModeFlat;

    for (frame = 0;; frame++) {
        result = StackWalk64(
            IMAGE_FILE_MACHINE_I386, process, thread, &stack, &context, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL);

        pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        pSymbol->MaxNameLength = MaxNameLen;

        SymGetSymFromAddr64(
            process, (ULONG64)stack.AddrPC.Offset, &displacement, pSymbol);
        UnDecorateSymbolName(
            pSymbol->Name, (PSTR)name, MaxNameLen, UNDNAME_COMPLETE);

        TRACE(
            "Frame %lu:\n"
            "    Symbol name:    %s\n"
            "    PC address:     0x%08LX\n"
            "    Stack address:  0x%08LX\n"
            "    Frame address:  0x%08LX\n"
            "\n",
            frame, pSymbol->Name, (ULONG64)stack.AddrPC.Offset,
            (ULONG64)stack.AddrStack.Offset, (ULONG64)stack.AddrFrame.Offset);

        if (!result) {
            break;
        }
    }
    free(pSymbol);
    free(name);
}
