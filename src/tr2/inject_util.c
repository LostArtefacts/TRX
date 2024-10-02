#include "inject_util.h"

#include <libtrx/log.h>

#include <windows.h>

void InjectImpl(bool enable, void (*from)(void), void (*to)(void))
{
    if (from == to) {
        return;
    }

    if (!enable) {
        void (*aux)(void) = from;
        from = to;
        to = aux;
    }

    DWORD tmp;
    LOG_DEBUG("Patching %p to %p", from, to);
    VirtualProtect(from, sizeof(JMP), PAGE_EXECUTE_READWRITE, &tmp);
    HANDLE hCurrentProcess = GetCurrentProcess();
    JMP buf;
    buf.opcode = 0xE9;
    buf.offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
    WriteProcessMemory(hCurrentProcess, from, &buf, sizeof(JMP), &tmp);
    CloseHandle(hCurrentProcess);
}
